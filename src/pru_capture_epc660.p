.origin 0
.entrypoint START

#define PRU1_ARM_INTERRUPT 19
#define AM33XX
//#define PRU0
#define PRU1
	

//#define GPIO1 0x4804c000
//#define GPIO_CLEARDATAOUT 0x190
//#define GPIO_SETDATAOUT 0x194

#define CONST_PRUCFG	     C4
#define CONST_PRUDRAM        C24
#define CONST_PRUSHAREDRAM   C28
#define CONST_DDR            C31

#ifdef PRU0
	// Address for the Constant table Block Index Register 0 (CTBIR0)
	#define CTBIR0        0x22020
	// Address for the Constant table Programmable Pointer Register 0(CTPPR0)	
	#define CTPPR0        0x22028 
	// Address for the Constant table Programmable Pointer Register 1(CTPPR1)	
	#define CTPPR1        0x2202C
#else
	#ifdef PRU1
		// Address for the Constant table Block Index Register 0 (CTBIR0)
		#define CTBIR0        0x24020
		// Address for the Constant table Programmable Pointer Register 0(CTPPR0)	
		#define CTPPR0        0x24028 
		// Address for the Constant table Programmable Pointer Register 1(CTPPR1)	
		#define CTPPR1        0x2402C
	#else
		#error Either PRU0 or PRU1 needs to be defined.
	#endif
#endif

START:
// clear that bit:
	LBCO r0, CONST_PRUCFG, 4, 4 // load data from memory address C4+0x4 into register r0
	CLR  r0, r0, 4  // Enable OCP master ports (1: Initiate standby sequence)
	SBCO r0, CONST_PRUCFG, 4, 4 // store data from r0 into memory at address C4 with offset 0x4.

// enable parallel capture mode for PRU0:	
//	LBCO r0, C4, 0x8, 4 // load data from memory address C4+0x8 into register r0
//	SET  r0, r0, 2  // 1: use negative edge of pru1_r31_status[16]
//	SET  r0, r0, 0  // bx01: 16-bit parallel capture mode
//	SBCO r0, C4, 0x8, 4 // store data from r0 into memory at address C4 with offset 0x8.

// enable parallel capture mode for PRU1:	
	LBCO r0, CONST_PRUCFG, 0xC, 4 // load data from memory address C4+0xC into register r0
	SET  r0, r0, 2  // 1: use negative edge of pru1_r31_status[16]
	SET  r0, r0, 0  // bx01: 16-bit parallel capture mode
	SBCO r0, CONST_PRUCFG, 0xC, 4 // store data from r0 into memory at address C4 with offset 0xC.

// Configure the programmable pointer register for PRU0 by setting c28_pointer[15:0] 
// field to 0x0120.  This will make C28 point to 0x00012000 (PRU shared RAM).
//    MOV  r0, 0x00000120
//    MOV  r1, CTPPR0
//    ST32 r0, r1

// Configure the programmable pointer register for PRU1 by setting C24_BLK_INDEX[7:0] 
// field to 0x01.  This will make C24 point to 0x00000100 (PRU1 local data memory).
//	MOV  r0, 0x0000001
//	MOV  r1, CTBIR0
//	SBBO r0, r1, 0, 4

// Configure the programmable pointer register for PRU1 by setting c31_pointer[15:0] 
// field to 0x0010.  This will make C31 point to 0x80001000 (DDR memory).
//	MOV  r0, 0x00100000
//	MOV  r1, CTPPR1
//	SBBO r0, r1, 0, 4

//// Clear local memory - just for debugging purposes.	
//	MOV  r0, 0x00000100
//	MOV  r1, 0x0000061F		// Initialize final pointer
//	MOV  r3, 0xFFFFFFFF
//CLEAR_MEM_1:	
//	SBBO r3, r0, 0, 4		// Copy 4 bytes from r3 to the memory address r0+0
//	ADD  r0, r0, 4			// Update/increase pointer		
//	QBGT CLEAR_MEM_1, r0, r1 	// Repeat if pointer did not reach the last address yet.
	

	MOV  r11, 0x00000000            // local starting address of configuration section in PRU1 DRAM
	LBBO r0, r11, 4, 4	 	// Initialize pointer to DDR memory.
	LBBO r1, r11, 8, 4		// load final pointer for DDR memory
		
INIT_ROW:
	MOV  r2, 0x00000100		// Initialize start pointer for data in local memory.
	
	// Initialize different starting addresses in order to compensate for the offset limitation in SBBO:
	MOV  r10, 256
	MOV  r3, r2
	ADD  r4, r3, r10
	ADD  r5, r4, r10
	ADD  r6, r5, r10
	ADD  r7, r6, r10
	ADD  r8, r7, r10

	// Load the settings for the ROI from the DDR memory:
	MOV  r11, 0x00000000    // local starting address of configuration section in PRU1 DRAM
	LBBO r13, r11, 0, 4		// load address offset required for jumping to correct address/instruction
	MOV  r12, r13			// multiply value in r13 by 3 for jumping to correct instruction
	ADD  r12, r13, r12		// multiplied by 2
	ADD  r13, r13, r12 		// multiplied by 3
	
	// Depending on the ROI and the binning, jump to the respective instruction:
	MOV  r9, CAPTURE_DATA
	ADD  r9, r9, r13

	// Wait for HSYNC going low:
	WBC  r31, 13
	
	JMP  r9   				// jump to correct instruction with respect to the number of columns	
	
CAPTURE_DATA:	
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r3+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r3+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r3+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r3+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r3+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r3+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r3+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r3+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r3+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r3+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r3+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r3+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r3+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r3+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r3+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r3+30.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 32, 2		// Copy 2 bytes from r31.w0 to the memory address r3+32.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 34, 2		// Copy 2 bytes from r31.w0 to the memory address r3+34.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 36, 2		// Copy 2 bytes from r31.w0 to the memory address r3+36.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 38, 2		// Copy 2 bytes from r31.w0 to the memory address r3+38.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 40, 2		// Copy 2 bytes from r31.w0 to the memory address r3+40.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 42, 2		// Copy 2 bytes from r31.w0 to the memory address r3+42.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 44, 2		// Copy 2 bytes from r31.w0 to the memory address r3+44.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 46, 2		// Copy 2 bytes from r31.w0 to the memory address r3+46.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 48, 2		// Copy 2 bytes from r31.w0 to the memory address r3+48.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 50, 2		// Copy 2 bytes from r31.w0 to the memory address r3+50.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 52, 2		// Copy 2 bytes from r31.w0 to the memory address r3+52.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 54, 2		// Copy 2 bytes from r31.w0 to the memory address r3+54.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 56, 2		// Copy 2 bytes from r31.w0 to the memory address r3+56.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 58, 2		// Copy 2 bytes from r31.w0 to the memory address r3+58.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 60, 2		// Copy 2 bytes from r31.w0 to the memory address r3+60.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 62, 2		// Copy 2 bytes from r31.w0 to the memory address r3+62.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 64, 2		// Copy 2 bytes from r31.w0 to the memory address r3+64.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 66, 2		// Copy 2 bytes from r31.w0 to the memory address r3+66.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 68, 2		// Copy 2 bytes from r31.w0 to the memory address r3+68.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 70, 2		// Copy 2 bytes from r31.w0 to the memory address r3+70.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 72, 2		// Copy 2 bytes from r31.w0 to the memory address r3+72.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 74, 2		// Copy 2 bytes from r31.w0 to the memory address r3+74.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 76, 2		// Copy 2 bytes from r31.w0 to the memory address r3+76.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 78, 2		// Copy 2 bytes from r31.w0 to the memory address r3+78.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 80, 2		// Copy 2 bytes from r31.w0 to the memory address r3+80.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 82, 2		// Copy 2 bytes from r31.w0 to the memory address r3+82.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 84, 2		// Copy 2 bytes from r31.w0 to the memory address r3+84.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 86, 2		// Copy 2 bytes from r31.w0 to the memory address r3+86.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 88, 2		// Copy 2 bytes from r31.w0 to the memory address r3+88.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 90, 2		// Copy 2 bytes from r31.w0 to the memory address r3+90.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 92, 2		// Copy 2 bytes from r31.w0 to the memory address r3+92.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 94, 2		// Copy 2 bytes from r31.w0 to the memory address r3+94.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 96, 2		// Copy 2 bytes from r31.w0 to the memory address r3+96.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 98, 2		// Copy 2 bytes from r31.w0 to the memory address r3+98.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 100, 2		// Copy 2 bytes from r31.w0 to the memory address r3+100.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 102, 2		// Copy 2 bytes from r31.w0 to the memory address r3+102.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 104, 2		// Copy 2 bytes from r31.w0 to the memory address r3+104.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 106, 2		// Copy 2 bytes from r31.w0 to the memory address r3+106.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 108, 2		// Copy 2 bytes from r31.w0 to the memory address r3+108.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 110, 2		// Copy 2 bytes from r31.w0 to the memory address r3+110.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 112, 2		// Copy 2 bytes from r31.w0 to the memory address r3+112.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 114, 2		// Copy 2 bytes from r31.w0 to the memory address r3+114.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 116, 2		// Copy 2 bytes from r31.w0 to the memory address r3+116.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 118, 2		// Copy 2 bytes from r31.w0 to the memory address r3+118.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 120, 2		// Copy 2 bytes from r31.w0 to the memory address r3+120.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 122, 2		// Copy 2 bytes from r31.w0 to the memory address r3+122.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 124, 2		// Copy 2 bytes from r31.w0 to the memory address r3+124.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 126, 2		// Copy 2 bytes from r31.w0 to the memory address r3+126.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 128, 2		// Copy 2 bytes from r31.w0 to the memory address r3+128.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 130, 2		// Copy 2 bytes from r31.w0 to the memory address r3+130.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 132, 2		// Copy 2 bytes from r31.w0 to the memory address r3+132.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 134, 2		// Copy 2 bytes from r31.w0 to the memory address r3+134.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 136, 2		// Copy 2 bytes from r31.w0 to the memory address r3+136.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 138, 2		// Copy 2 bytes from r31.w0 to the memory address r3+138.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 140, 2		// Copy 2 bytes from r31.w0 to the memory address r3+140.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 142, 2		// Copy 2 bytes from r31.w0 to the memory address r3+142.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 144, 2		// Copy 2 bytes from r31.w0 to the memory address r3+144.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 146, 2		// Copy 2 bytes from r31.w0 to the memory address r3+146.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 148, 2		// Copy 2 bytes from r31.w0 to the memory address r3+148.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 150, 2		// Copy 2 bytes from r31.w0 to the memory address r3+150.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 152, 2		// Copy 2 bytes from r31.w0 to the memory address r3+152.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 154, 2		// Copy 2 bytes from r31.w0 to the memory address r3+154.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 156, 2		// Copy 2 bytes from r31.w0 to the memory address r3+156.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 158, 2		// Copy 2 bytes from r31.w0 to the memory address r3+158.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 160, 2		// Copy 2 bytes from r31.w0 to the memory address r3+160.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 162, 2		// Copy 2 bytes from r31.w0 to the memory address r3+162.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 164, 2		// Copy 2 bytes from r31.w0 to the memory address r3+164.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 166, 2		// Copy 2 bytes from r31.w0 to the memory address r3+166.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 168, 2		// Copy 2 bytes from r31.w0 to the memory address r3+168.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 170, 2		// Copy 2 bytes from r31.w0 to the memory address r3+170.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 172, 2		// Copy 2 bytes from r31.w0 to the memory address r3+172.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 174, 2		// Copy 2 bytes from r31.w0 to the memory address r3+174.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 176, 2		// Copy 2 bytes from r31.w0 to the memory address r3+176.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 178, 2		// Copy 2 bytes from r31.w0 to the memory address r3+178.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 180, 2		// Copy 2 bytes from r31.w0 to the memory address r3+180.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 182, 2		// Copy 2 bytes from r31.w0 to the memory address r3+182.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 184, 2		// Copy 2 bytes from r31.w0 to the memory address r3+184.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 186, 2		// Copy 2 bytes from r31.w0 to the memory address r3+186.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 188, 2		// Copy 2 bytes from r31.w0 to the memory address r3+188.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 190, 2		// Copy 2 bytes from r31.w0 to the memory address r3+190.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 192, 2		// Copy 2 bytes from r31.w0 to the memory address r3+192.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 194, 2		// Copy 2 bytes from r31.w0 to the memory address r3+194.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 196, 2		// Copy 2 bytes from r31.w0 to the memory address r3+196.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 198, 2		// Copy 2 bytes from r31.w0 to the memory address r3+198.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 200, 2		// Copy 2 bytes from r31.w0 to the memory address r3+200.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 202, 2		// Copy 2 bytes from r31.w0 to the memory address r3+202.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 204, 2		// Copy 2 bytes from r31.w0 to the memory address r3+204.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 206, 2		// Copy 2 bytes from r31.w0 to the memory address r3+206.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 208, 2		// Copy 2 bytes from r31.w0 to the memory address r3+208.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 210, 2		// Copy 2 bytes from r31.w0 to the memory address r3+210.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 212, 2		// Copy 2 bytes from r31.w0 to the memory address r3+212.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 214, 2		// Copy 2 bytes from r31.w0 to the memory address r3+214.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 216, 2		// Copy 2 bytes from r31.w0 to the memory address r3+216.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 218, 2		// Copy 2 bytes from r31.w0 to the memory address r3+218.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 220, 2		// Copy 2 bytes from r31.w0 to the memory address r3+220.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 222, 2		// Copy 2 bytes from r31.w0 to the memory address r3+222.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 224, 2		// Copy 2 bytes from r31.w0 to the memory address r3+224.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 226, 2		// Copy 2 bytes from r31.w0 to the memory address r3+226.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 228, 2		// Copy 2 bytes from r31.w0 to the memory address r3+228.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 230, 2		// Copy 2 bytes from r31.w0 to the memory address r3+230.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 232, 2		// Copy 2 bytes from r31.w0 to the memory address r3+232.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 234, 2		// Copy 2 bytes from r31.w0 to the memory address r3+234.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 236, 2		// Copy 2 bytes from r31.w0 to the memory address r3+236.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 238, 2		// Copy 2 bytes from r31.w0 to the memory address r3+238.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 240, 2		// Copy 2 bytes from r31.w0 to the memory address r3+240.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 242, 2		// Copy 2 bytes from r31.w0 to the memory address r3+242.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 244, 2		// Copy 2 bytes from r31.w0 to the memory address r3+244.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 246, 2		// Copy 2 bytes from r31.w0 to the memory address r3+246.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 248, 2		// Copy 2 bytes from r31.w0 to the memory address r3+248.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 250, 2		// Copy 2 bytes from r31.w0 to the memory address r3+250.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 252, 2		// Copy 2 bytes from r31.w0 to the memory address r3+252.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r3, 254, 2		// Copy 2 bytes from r31.w0 to the memory address r3+254.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r4+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r4+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r4+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r4+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r4+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r4+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r4+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r4+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r4+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r4+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r4+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r4+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r4+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r4+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r4+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r4+30.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 32, 2		// Copy 2 bytes from r31.w0 to the memory address r4+32.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 34, 2		// Copy 2 bytes from r31.w0 to the memory address r4+34.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 36, 2		// Copy 2 bytes from r31.w0 to the memory address r4+36.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 38, 2		// Copy 2 bytes from r31.w0 to the memory address r4+38.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 40, 2		// Copy 2 bytes from r31.w0 to the memory address r4+40.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 42, 2		// Copy 2 bytes from r31.w0 to the memory address r4+42.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 44, 2		// Copy 2 bytes from r31.w0 to the memory address r4+44.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 46, 2		// Copy 2 bytes from r31.w0 to the memory address r4+46.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 48, 2		// Copy 2 bytes from r31.w0 to the memory address r4+48.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 50, 2		// Copy 2 bytes from r31.w0 to the memory address r4+50.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 52, 2		// Copy 2 bytes from r31.w0 to the memory address r4+52.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 54, 2		// Copy 2 bytes from r31.w0 to the memory address r4+54.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 56, 2		// Copy 2 bytes from r31.w0 to the memory address r4+56.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 58, 2		// Copy 2 bytes from r31.w0 to the memory address r4+58.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 60, 2		// Copy 2 bytes from r31.w0 to the memory address r4+60.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 62, 2		// Copy 2 bytes from r31.w0 to the memory address r4+62.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 64, 2		// Copy 2 bytes from r31.w0 to the memory address r4+64.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 66, 2		// Copy 2 bytes from r31.w0 to the memory address r4+66.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 68, 2		// Copy 2 bytes from r31.w0 to the memory address r4+68.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 70, 2		// Copy 2 bytes from r31.w0 to the memory address r4+70.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 72, 2		// Copy 2 bytes from r31.w0 to the memory address r4+72.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 74, 2		// Copy 2 bytes from r31.w0 to the memory address r4+74.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 76, 2		// Copy 2 bytes from r31.w0 to the memory address r4+76.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 78, 2		// Copy 2 bytes from r31.w0 to the memory address r4+78.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 80, 2		// Copy 2 bytes from r31.w0 to the memory address r4+80.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 82, 2		// Copy 2 bytes from r31.w0 to the memory address r4+82.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 84, 2		// Copy 2 bytes from r31.w0 to the memory address r4+84.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 86, 2		// Copy 2 bytes from r31.w0 to the memory address r4+86.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 88, 2		// Copy 2 bytes from r31.w0 to the memory address r4+88.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 90, 2		// Copy 2 bytes from r31.w0 to the memory address r4+90.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 92, 2		// Copy 2 bytes from r31.w0 to the memory address r4+92.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 94, 2		// Copy 2 bytes from r31.w0 to the memory address r4+94.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 96, 2		// Copy 2 bytes from r31.w0 to the memory address r4+96.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 98, 2		// Copy 2 bytes from r31.w0 to the memory address r4+98.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 100, 2		// Copy 2 bytes from r31.w0 to the memory address r4+100.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 102, 2		// Copy 2 bytes from r31.w0 to the memory address r4+102.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 104, 2		// Copy 2 bytes from r31.w0 to the memory address r4+104.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 106, 2		// Copy 2 bytes from r31.w0 to the memory address r4+106.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 108, 2		// Copy 2 bytes from r31.w0 to the memory address r4+108.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 110, 2		// Copy 2 bytes from r31.w0 to the memory address r4+110.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 112, 2		// Copy 2 bytes from r31.w0 to the memory address r4+112.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 114, 2		// Copy 2 bytes from r31.w0 to the memory address r4+114.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 116, 2		// Copy 2 bytes from r31.w0 to the memory address r4+116.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 118, 2		// Copy 2 bytes from r31.w0 to the memory address r4+118.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 120, 2		// Copy 2 bytes from r31.w0 to the memory address r4+120.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 122, 2		// Copy 2 bytes from r31.w0 to the memory address r4+122.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 124, 2		// Copy 2 bytes from r31.w0 to the memory address r4+124.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 126, 2		// Copy 2 bytes from r31.w0 to the memory address r4+126.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 128, 2		// Copy 2 bytes from r31.w0 to the memory address r4+128.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 130, 2		// Copy 2 bytes from r31.w0 to the memory address r4+130.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 132, 2		// Copy 2 bytes from r31.w0 to the memory address r4+132.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 134, 2		// Copy 2 bytes from r31.w0 to the memory address r4+134.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 136, 2		// Copy 2 bytes from r31.w0 to the memory address r4+136.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 138, 2		// Copy 2 bytes from r31.w0 to the memory address r4+138.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 140, 2		// Copy 2 bytes from r31.w0 to the memory address r4+140.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 142, 2		// Copy 2 bytes from r31.w0 to the memory address r4+142.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 144, 2		// Copy 2 bytes from r31.w0 to the memory address r4+144.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 146, 2		// Copy 2 bytes from r31.w0 to the memory address r4+146.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 148, 2		// Copy 2 bytes from r31.w0 to the memory address r4+148.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 150, 2		// Copy 2 bytes from r31.w0 to the memory address r4+150.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 152, 2		// Copy 2 bytes from r31.w0 to the memory address r4+152.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 154, 2		// Copy 2 bytes from r31.w0 to the memory address r4+154.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 156, 2		// Copy 2 bytes from r31.w0 to the memory address r4+156.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 158, 2		// Copy 2 bytes from r31.w0 to the memory address r4+158.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 160, 2		// Copy 2 bytes from r31.w0 to the memory address r4+160.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 162, 2		// Copy 2 bytes from r31.w0 to the memory address r4+162.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 164, 2		// Copy 2 bytes from r31.w0 to the memory address r4+164.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 166, 2		// Copy 2 bytes from r31.w0 to the memory address r4+166.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 168, 2		// Copy 2 bytes from r31.w0 to the memory address r4+168.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 170, 2		// Copy 2 bytes from r31.w0 to the memory address r4+170.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 172, 2		// Copy 2 bytes from r31.w0 to the memory address r4+172.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 174, 2		// Copy 2 bytes from r31.w0 to the memory address r4+174.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 176, 2		// Copy 2 bytes from r31.w0 to the memory address r4+176.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 178, 2		// Copy 2 bytes from r31.w0 to the memory address r4+178.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 180, 2		// Copy 2 bytes from r31.w0 to the memory address r4+180.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 182, 2		// Copy 2 bytes from r31.w0 to the memory address r4+182.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 184, 2		// Copy 2 bytes from r31.w0 to the memory address r4+184.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 186, 2		// Copy 2 bytes from r31.w0 to the memory address r4+186.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 188, 2		// Copy 2 bytes from r31.w0 to the memory address r4+188.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 190, 2		// Copy 2 bytes from r31.w0 to the memory address r4+190.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 192, 2		// Copy 2 bytes from r31.w0 to the memory address r4+192.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 194, 2		// Copy 2 bytes from r31.w0 to the memory address r4+194.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 196, 2		// Copy 2 bytes from r31.w0 to the memory address r4+196.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 198, 2		// Copy 2 bytes from r31.w0 to the memory address r4+198.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 200, 2		// Copy 2 bytes from r31.w0 to the memory address r4+200.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 202, 2		// Copy 2 bytes from r31.w0 to the memory address r4+202.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 204, 2		// Copy 2 bytes from r31.w0 to the memory address r4+204.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 206, 2		// Copy 2 bytes from r31.w0 to the memory address r4+206.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 208, 2		// Copy 2 bytes from r31.w0 to the memory address r4+208.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 210, 2		// Copy 2 bytes from r31.w0 to the memory address r4+210.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 212, 2		// Copy 2 bytes from r31.w0 to the memory address r4+212.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 214, 2		// Copy 2 bytes from r31.w0 to the memory address r4+214.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 216, 2		// Copy 2 bytes from r31.w0 to the memory address r4+216.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 218, 2		// Copy 2 bytes from r31.w0 to the memory address r4+218.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 220, 2		// Copy 2 bytes from r31.w0 to the memory address r4+220.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 222, 2		// Copy 2 bytes from r31.w0 to the memory address r4+222.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 224, 2		// Copy 2 bytes from r31.w0 to the memory address r4+224.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 226, 2		// Copy 2 bytes from r31.w0 to the memory address r4+226.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 228, 2		// Copy 2 bytes from r31.w0 to the memory address r4+228.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 230, 2		// Copy 2 bytes from r31.w0 to the memory address r4+230.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 232, 2		// Copy 2 bytes from r31.w0 to the memory address r4+232.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 234, 2		// Copy 2 bytes from r31.w0 to the memory address r4+234.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 236, 2		// Copy 2 bytes from r31.w0 to the memory address r4+236.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 238, 2		// Copy 2 bytes from r31.w0 to the memory address r4+238.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 240, 2		// Copy 2 bytes from r31.w0 to the memory address r4+240.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 242, 2		// Copy 2 bytes from r31.w0 to the memory address r4+242.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 244, 2		// Copy 2 bytes from r31.w0 to the memory address r4+244.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 246, 2		// Copy 2 bytes from r31.w0 to the memory address r4+246.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 248, 2		// Copy 2 bytes from r31.w0 to the memory address r4+248.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 250, 2		// Copy 2 bytes from r31.w0 to the memory address r4+250.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 252, 2		// Copy 2 bytes from r31.w0 to the memory address r4+252.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r4, 254, 2		// Copy 2 bytes from r31.w0 to the memory address r4+254.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r5+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r5+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r5+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r5+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r5+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r5+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r5+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r5+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r5+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r5+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r5+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r5+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r5+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r5+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r5+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r5+30.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 32, 2		// Copy 2 bytes from r31.w0 to the memory address r5+32.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 34, 2		// Copy 2 bytes from r31.w0 to the memory address r5+34.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 36, 2		// Copy 2 bytes from r31.w0 to the memory address r5+36.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 38, 2		// Copy 2 bytes from r31.w0 to the memory address r5+38.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 40, 2		// Copy 2 bytes from r31.w0 to the memory address r5+40.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 42, 2		// Copy 2 bytes from r31.w0 to the memory address r5+42.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 44, 2		// Copy 2 bytes from r31.w0 to the memory address r5+44.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 46, 2		// Copy 2 bytes from r31.w0 to the memory address r5+46.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 48, 2		// Copy 2 bytes from r31.w0 to the memory address r5+48.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 50, 2		// Copy 2 bytes from r31.w0 to the memory address r5+50.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 52, 2		// Copy 2 bytes from r31.w0 to the memory address r5+52.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 54, 2		// Copy 2 bytes from r31.w0 to the memory address r5+54.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 56, 2		// Copy 2 bytes from r31.w0 to the memory address r5+56.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 58, 2		// Copy 2 bytes from r31.w0 to the memory address r5+58.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 60, 2		// Copy 2 bytes from r31.w0 to the memory address r5+60.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 62, 2		// Copy 2 bytes from r31.w0 to the memory address r5+62.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 64, 2		// Copy 2 bytes from r31.w0 to the memory address r5+64.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 66, 2		// Copy 2 bytes from r31.w0 to the memory address r5+66.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 68, 2		// Copy 2 bytes from r31.w0 to the memory address r5+68.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 70, 2		// Copy 2 bytes from r31.w0 to the memory address r5+70.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 72, 2		// Copy 2 bytes from r31.w0 to the memory address r5+72.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 74, 2		// Copy 2 bytes from r31.w0 to the memory address r5+74.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 76, 2		// Copy 2 bytes from r31.w0 to the memory address r5+76.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 78, 2		// Copy 2 bytes from r31.w0 to the memory address r5+78.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 80, 2		// Copy 2 bytes from r31.w0 to the memory address r5+80.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 82, 2		// Copy 2 bytes from r31.w0 to the memory address r5+82.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 84, 2		// Copy 2 bytes from r31.w0 to the memory address r5+84.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 86, 2		// Copy 2 bytes from r31.w0 to the memory address r5+86.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 88, 2		// Copy 2 bytes from r31.w0 to the memory address r5+88.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 90, 2		// Copy 2 bytes from r31.w0 to the memory address r5+90.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 92, 2		// Copy 2 bytes from r31.w0 to the memory address r5+92.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 94, 2		// Copy 2 bytes from r31.w0 to the memory address r5+94.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 96, 2		// Copy 2 bytes from r31.w0 to the memory address r5+96.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 98, 2		// Copy 2 bytes from r31.w0 to the memory address r5+98.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 100, 2		// Copy 2 bytes from r31.w0 to the memory address r5+100.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 102, 2		// Copy 2 bytes from r31.w0 to the memory address r5+102.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 104, 2		// Copy 2 bytes from r31.w0 to the memory address r5+104.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 106, 2		// Copy 2 bytes from r31.w0 to the memory address r5+106.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 108, 2		// Copy 2 bytes from r31.w0 to the memory address r5+108.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 110, 2		// Copy 2 bytes from r31.w0 to the memory address r5+110.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 112, 2		// Copy 2 bytes from r31.w0 to the memory address r5+112.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 114, 2		// Copy 2 bytes from r31.w0 to the memory address r5+114.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 116, 2		// Copy 2 bytes from r31.w0 to the memory address r5+116.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 118, 2		// Copy 2 bytes from r31.w0 to the memory address r5+118.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 120, 2		// Copy 2 bytes from r31.w0 to the memory address r5+120.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 122, 2		// Copy 2 bytes from r31.w0 to the memory address r5+122.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 124, 2		// Copy 2 bytes from r31.w0 to the memory address r5+124.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 126, 2		// Copy 2 bytes from r31.w0 to the memory address r5+126.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 128, 2		// Copy 2 bytes from r31.w0 to the memory address r5+128.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 130, 2		// Copy 2 bytes from r31.w0 to the memory address r5+130.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 132, 2		// Copy 2 bytes from r31.w0 to the memory address r5+132.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 134, 2		// Copy 2 bytes from r31.w0 to the memory address r5+134.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 136, 2		// Copy 2 bytes from r31.w0 to the memory address r5+136.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 138, 2		// Copy 2 bytes from r31.w0 to the memory address r5+138.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 140, 2		// Copy 2 bytes from r31.w0 to the memory address r5+140.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 142, 2		// Copy 2 bytes from r31.w0 to the memory address r5+142.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 144, 2		// Copy 2 bytes from r31.w0 to the memory address r5+144.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 146, 2		// Copy 2 bytes from r31.w0 to the memory address r5+146.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 148, 2		// Copy 2 bytes from r31.w0 to the memory address r5+148.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 150, 2		// Copy 2 bytes from r31.w0 to the memory address r5+150.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 152, 2		// Copy 2 bytes from r31.w0 to the memory address r5+152.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 154, 2		// Copy 2 bytes from r31.w0 to the memory address r5+154.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 156, 2		// Copy 2 bytes from r31.w0 to the memory address r5+156.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 158, 2		// Copy 2 bytes from r31.w0 to the memory address r5+158.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 160, 2		// Copy 2 bytes from r31.w0 to the memory address r5+160.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 162, 2		// Copy 2 bytes from r31.w0 to the memory address r5+162.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 164, 2		// Copy 2 bytes from r31.w0 to the memory address r5+164.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 166, 2		// Copy 2 bytes from r31.w0 to the memory address r5+166.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 168, 2		// Copy 2 bytes from r31.w0 to the memory address r5+168.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 170, 2		// Copy 2 bytes from r31.w0 to the memory address r5+170.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 172, 2		// Copy 2 bytes from r31.w0 to the memory address r5+172.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 174, 2		// Copy 2 bytes from r31.w0 to the memory address r5+174.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 176, 2		// Copy 2 bytes from r31.w0 to the memory address r5+176.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 178, 2		// Copy 2 bytes from r31.w0 to the memory address r5+178.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 180, 2		// Copy 2 bytes from r31.w0 to the memory address r5+180.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 182, 2		// Copy 2 bytes from r31.w0 to the memory address r5+182.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 184, 2		// Copy 2 bytes from r31.w0 to the memory address r5+184.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 186, 2		// Copy 2 bytes from r31.w0 to the memory address r5+186.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 188, 2		// Copy 2 bytes from r31.w0 to the memory address r5+188.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 190, 2		// Copy 2 bytes from r31.w0 to the memory address r5+190.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 192, 2		// Copy 2 bytes from r31.w0 to the memory address r5+192.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 194, 2		// Copy 2 bytes from r31.w0 to the memory address r5+194.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 196, 2		// Copy 2 bytes from r31.w0 to the memory address r5+196.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 198, 2		// Copy 2 bytes from r31.w0 to the memory address r5+198.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 200, 2		// Copy 2 bytes from r31.w0 to the memory address r5+200.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 202, 2		// Copy 2 bytes from r31.w0 to the memory address r5+202.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 204, 2		// Copy 2 bytes from r31.w0 to the memory address r5+204.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 206, 2		// Copy 2 bytes from r31.w0 to the memory address r5+206.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 208, 2		// Copy 2 bytes from r31.w0 to the memory address r5+208.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 210, 2		// Copy 2 bytes from r31.w0 to the memory address r5+210.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 212, 2		// Copy 2 bytes from r31.w0 to the memory address r5+212.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 214, 2		// Copy 2 bytes from r31.w0 to the memory address r5+214.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 216, 2		// Copy 2 bytes from r31.w0 to the memory address r5+216.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 218, 2		// Copy 2 bytes from r31.w0 to the memory address r5+218.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 220, 2		// Copy 2 bytes from r31.w0 to the memory address r5+220.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 222, 2		// Copy 2 bytes from r31.w0 to the memory address r5+222.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 224, 2		// Copy 2 bytes from r31.w0 to the memory address r5+224.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 226, 2		// Copy 2 bytes from r31.w0 to the memory address r5+226.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 228, 2		// Copy 2 bytes from r31.w0 to the memory address r5+228.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 230, 2		// Copy 2 bytes from r31.w0 to the memory address r5+230.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 232, 2		// Copy 2 bytes from r31.w0 to the memory address r5+232.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 234, 2		// Copy 2 bytes from r31.w0 to the memory address r5+234.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 236, 2		// Copy 2 bytes from r31.w0 to the memory address r5+236.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 238, 2		// Copy 2 bytes from r31.w0 to the memory address r5+238.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 240, 2		// Copy 2 bytes from r31.w0 to the memory address r5+240.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 242, 2		// Copy 2 bytes from r31.w0 to the memory address r5+242.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 244, 2		// Copy 2 bytes from r31.w0 to the memory address r5+244.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 246, 2		// Copy 2 bytes from r31.w0 to the memory address r5+246.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 248, 2		// Copy 2 bytes from r31.w0 to the memory address r5+248.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 250, 2		// Copy 2 bytes from r31.w0 to the memory address r5+250.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 252, 2		// Copy 2 bytes from r31.w0 to the memory address r5+252.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r5, 254, 2		// Copy 2 bytes from r31.w0 to the memory address r5+254.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r6+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r6+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r6+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r6+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r6+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r6+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r6+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r6+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r6+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r6+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r6+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r6+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r6+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r6+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r6+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r6+30.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 32, 2		// Copy 2 bytes from r31.w0 to the memory address r6+32.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 34, 2		// Copy 2 bytes from r31.w0 to the memory address r6+34.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 36, 2		// Copy 2 bytes from r31.w0 to the memory address r6+36.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 38, 2		// Copy 2 bytes from r31.w0 to the memory address r6+38.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 40, 2		// Copy 2 bytes from r31.w0 to the memory address r6+40.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 42, 2		// Copy 2 bytes from r31.w0 to the memory address r6+42.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 44, 2		// Copy 2 bytes from r31.w0 to the memory address r6+44.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 46, 2		// Copy 2 bytes from r31.w0 to the memory address r6+46.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 48, 2		// Copy 2 bytes from r31.w0 to the memory address r6+48.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 50, 2		// Copy 2 bytes from r31.w0 to the memory address r6+50.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 52, 2		// Copy 2 bytes from r31.w0 to the memory address r6+52.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 54, 2		// Copy 2 bytes from r31.w0 to the memory address r6+54.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 56, 2		// Copy 2 bytes from r31.w0 to the memory address r6+56.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 58, 2		// Copy 2 bytes from r31.w0 to the memory address r6+58.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 60, 2		// Copy 2 bytes from r31.w0 to the memory address r6+60.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 62, 2		// Copy 2 bytes from r31.w0 to the memory address r6+62.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 64, 2		// Copy 2 bytes from r31.w0 to the memory address r6+64.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 66, 2		// Copy 2 bytes from r31.w0 to the memory address r6+66.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 68, 2		// Copy 2 bytes from r31.w0 to the memory address r6+68.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 70, 2		// Copy 2 bytes from r31.w0 to the memory address r6+70.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 72, 2		// Copy 2 bytes from r31.w0 to the memory address r6+72.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 74, 2		// Copy 2 bytes from r31.w0 to the memory address r6+74.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 76, 2		// Copy 2 bytes from r31.w0 to the memory address r6+76.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 78, 2		// Copy 2 bytes from r31.w0 to the memory address r6+78.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 80, 2		// Copy 2 bytes from r31.w0 to the memory address r6+80.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 82, 2		// Copy 2 bytes from r31.w0 to the memory address r6+82.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 84, 2		// Copy 2 bytes from r31.w0 to the memory address r6+84.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 86, 2		// Copy 2 bytes from r31.w0 to the memory address r6+86.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 88, 2		// Copy 2 bytes from r31.w0 to the memory address r6+88.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 90, 2		// Copy 2 bytes from r31.w0 to the memory address r6+90.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 92, 2		// Copy 2 bytes from r31.w0 to the memory address r6+92.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 94, 2		// Copy 2 bytes from r31.w0 to the memory address r6+94.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 96, 2		// Copy 2 bytes from r31.w0 to the memory address r6+96.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 98, 2		// Copy 2 bytes from r31.w0 to the memory address r6+98.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 100, 2		// Copy 2 bytes from r31.w0 to the memory address r6+100.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 102, 2		// Copy 2 bytes from r31.w0 to the memory address r6+102.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 104, 2		// Copy 2 bytes from r31.w0 to the memory address r6+104.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 106, 2		// Copy 2 bytes from r31.w0 to the memory address r6+106.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 108, 2		// Copy 2 bytes from r31.w0 to the memory address r6+108.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 110, 2		// Copy 2 bytes from r31.w0 to the memory address r6+110.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 112, 2		// Copy 2 bytes from r31.w0 to the memory address r6+112.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 114, 2		// Copy 2 bytes from r31.w0 to the memory address r6+114.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 116, 2		// Copy 2 bytes from r31.w0 to the memory address r6+116.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 118, 2		// Copy 2 bytes from r31.w0 to the memory address r6+118.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 120, 2		// Copy 2 bytes from r31.w0 to the memory address r6+120.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 122, 2		// Copy 2 bytes from r31.w0 to the memory address r6+122.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 124, 2		// Copy 2 bytes from r31.w0 to the memory address r6+124.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 126, 2		// Copy 2 bytes from r31.w0 to the memory address r6+126.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 128, 2		// Copy 2 bytes from r31.w0 to the memory address r6+128.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 130, 2		// Copy 2 bytes from r31.w0 to the memory address r6+130.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 132, 2		// Copy 2 bytes from r31.w0 to the memory address r6+132.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 134, 2		// Copy 2 bytes from r31.w0 to the memory address r6+134.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 136, 2		// Copy 2 bytes from r31.w0 to the memory address r6+136.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 138, 2		// Copy 2 bytes from r31.w0 to the memory address r6+138.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 140, 2		// Copy 2 bytes from r31.w0 to the memory address r6+140.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 142, 2		// Copy 2 bytes from r31.w0 to the memory address r6+142.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 144, 2		// Copy 2 bytes from r31.w0 to the memory address r6+144.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 146, 2		// Copy 2 bytes from r31.w0 to the memory address r6+146.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 148, 2		// Copy 2 bytes from r31.w0 to the memory address r6+148.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 150, 2		// Copy 2 bytes from r31.w0 to the memory address r6+150.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 152, 2		// Copy 2 bytes from r31.w0 to the memory address r6+152.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 154, 2		// Copy 2 bytes from r31.w0 to the memory address r6+154.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 156, 2		// Copy 2 bytes from r31.w0 to the memory address r6+156.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 158, 2		// Copy 2 bytes from r31.w0 to the memory address r6+158.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 160, 2		// Copy 2 bytes from r31.w0 to the memory address r6+160.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 162, 2		// Copy 2 bytes from r31.w0 to the memory address r6+162.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 164, 2		// Copy 2 bytes from r31.w0 to the memory address r6+164.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 166, 2		// Copy 2 bytes from r31.w0 to the memory address r6+166.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 168, 2		// Copy 2 bytes from r31.w0 to the memory address r6+168.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 170, 2		// Copy 2 bytes from r31.w0 to the memory address r6+170.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 172, 2		// Copy 2 bytes from r31.w0 to the memory address r6+172.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 174, 2		// Copy 2 bytes from r31.w0 to the memory address r6+174.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 176, 2		// Copy 2 bytes from r31.w0 to the memory address r6+176.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 178, 2		// Copy 2 bytes from r31.w0 to the memory address r6+178.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 180, 2		// Copy 2 bytes from r31.w0 to the memory address r6+180.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 182, 2		// Copy 2 bytes from r31.w0 to the memory address r6+182.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 184, 2		// Copy 2 bytes from r31.w0 to the memory address r6+184.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 186, 2		// Copy 2 bytes from r31.w0 to the memory address r6+186.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 188, 2		// Copy 2 bytes from r31.w0 to the memory address r6+188.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 190, 2		// Copy 2 bytes from r31.w0 to the memory address r6+190.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 192, 2		// Copy 2 bytes from r31.w0 to the memory address r6+192.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 194, 2		// Copy 2 bytes from r31.w0 to the memory address r6+194.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 196, 2		// Copy 2 bytes from r31.w0 to the memory address r6+196.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 198, 2		// Copy 2 bytes from r31.w0 to the memory address r6+198.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 200, 2		// Copy 2 bytes from r31.w0 to the memory address r6+200.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 202, 2		// Copy 2 bytes from r31.w0 to the memory address r6+202.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 204, 2		// Copy 2 bytes from r31.w0 to the memory address r6+204.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 206, 2		// Copy 2 bytes from r31.w0 to the memory address r6+206.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 208, 2		// Copy 2 bytes from r31.w0 to the memory address r6+208.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 210, 2		// Copy 2 bytes from r31.w0 to the memory address r6+210.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 212, 2		// Copy 2 bytes from r31.w0 to the memory address r6+212.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 214, 2		// Copy 2 bytes from r31.w0 to the memory address r6+214.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 216, 2		// Copy 2 bytes from r31.w0 to the memory address r6+216.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 218, 2		// Copy 2 bytes from r31.w0 to the memory address r6+218.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 220, 2		// Copy 2 bytes from r31.w0 to the memory address r6+220.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 222, 2		// Copy 2 bytes from r31.w0 to the memory address r6+222.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 224, 2		// Copy 2 bytes from r31.w0 to the memory address r6+224.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 226, 2		// Copy 2 bytes from r31.w0 to the memory address r6+226.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 228, 2		// Copy 2 bytes from r31.w0 to the memory address r6+228.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 230, 2		// Copy 2 bytes from r31.w0 to the memory address r6+230.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 232, 2		// Copy 2 bytes from r31.w0 to the memory address r6+232.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 234, 2		// Copy 2 bytes from r31.w0 to the memory address r6+234.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 236, 2		// Copy 2 bytes from r31.w0 to the memory address r6+236.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 238, 2		// Copy 2 bytes from r31.w0 to the memory address r6+238.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 240, 2		// Copy 2 bytes from r31.w0 to the memory address r6+240.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 242, 2		// Copy 2 bytes from r31.w0 to the memory address r6+242.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 244, 2		// Copy 2 bytes from r31.w0 to the memory address r6+244.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 246, 2		// Copy 2 bytes from r31.w0 to the memory address r6+246.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 248, 2		// Copy 2 bytes from r31.w0 to the memory address r6+248.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 250, 2		// Copy 2 bytes from r31.w0 to the memory address r6+250.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 252, 2		// Copy 2 bytes from r31.w0 to the memory address r6+252.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r6, 254, 2		// Copy 2 bytes from r31.w0 to the memory address r6+254.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r7+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r7+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r7+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r7+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r7+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r7+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r7+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r7+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r7+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r7+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r7+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r7+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r7+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r7+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r7+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r7+30.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 32, 2		// Copy 2 bytes from r31.w0 to the memory address r7+32.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 34, 2		// Copy 2 bytes from r31.w0 to the memory address r7+34.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 36, 2		// Copy 2 bytes from r31.w0 to the memory address r7+36.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 38, 2		// Copy 2 bytes from r31.w0 to the memory address r7+38.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 40, 2		// Copy 2 bytes from r31.w0 to the memory address r7+40.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 42, 2		// Copy 2 bytes from r31.w0 to the memory address r7+42.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 44, 2		// Copy 2 bytes from r31.w0 to the memory address r7+44.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 46, 2		// Copy 2 bytes from r31.w0 to the memory address r7+46.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 48, 2		// Copy 2 bytes from r31.w0 to the memory address r7+48.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 50, 2		// Copy 2 bytes from r31.w0 to the memory address r7+50.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 52, 2		// Copy 2 bytes from r31.w0 to the memory address r7+52.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 54, 2		// Copy 2 bytes from r31.w0 to the memory address r7+54.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 56, 2		// Copy 2 bytes from r31.w0 to the memory address r7+56.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 58, 2		// Copy 2 bytes from r31.w0 to the memory address r7+58.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 60, 2		// Copy 2 bytes from r31.w0 to the memory address r7+60.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 62, 2		// Copy 2 bytes from r31.w0 to the memory address r7+62.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 64, 2		// Copy 2 bytes from r31.w0 to the memory address r7+64.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 66, 2		// Copy 2 bytes from r31.w0 to the memory address r7+66.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 68, 2		// Copy 2 bytes from r31.w0 to the memory address r7+68.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 70, 2		// Copy 2 bytes from r31.w0 to the memory address r7+70.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 72, 2		// Copy 2 bytes from r31.w0 to the memory address r7+72.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 74, 2		// Copy 2 bytes from r31.w0 to the memory address r7+74.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 76, 2		// Copy 2 bytes from r31.w0 to the memory address r7+76.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 78, 2		// Copy 2 bytes from r31.w0 to the memory address r7+78.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 80, 2		// Copy 2 bytes from r31.w0 to the memory address r7+80.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 82, 2		// Copy 2 bytes from r31.w0 to the memory address r7+82.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 84, 2		// Copy 2 bytes from r31.w0 to the memory address r7+84.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 86, 2		// Copy 2 bytes from r31.w0 to the memory address r7+86.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 88, 2		// Copy 2 bytes from r31.w0 to the memory address r7+88.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 90, 2		// Copy 2 bytes from r31.w0 to the memory address r7+90.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 92, 2		// Copy 2 bytes from r31.w0 to the memory address r7+92.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 94, 2		// Copy 2 bytes from r31.w0 to the memory address r7+94.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 96, 2		// Copy 2 bytes from r31.w0 to the memory address r7+96.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 98, 2		// Copy 2 bytes from r31.w0 to the memory address r7+98.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 100, 2		// Copy 2 bytes from r31.w0 to the memory address r7+100.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 102, 2		// Copy 2 bytes from r31.w0 to the memory address r7+102.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 104, 2		// Copy 2 bytes from r31.w0 to the memory address r7+104.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 106, 2		// Copy 2 bytes from r31.w0 to the memory address r7+106.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 108, 2		// Copy 2 bytes from r31.w0 to the memory address r7+108.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 110, 2		// Copy 2 bytes from r31.w0 to the memory address r7+110.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 112, 2		// Copy 2 bytes from r31.w0 to the memory address r7+112.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 114, 2		// Copy 2 bytes from r31.w0 to the memory address r7+114.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 116, 2		// Copy 2 bytes from r31.w0 to the memory address r7+116.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 118, 2		// Copy 2 bytes from r31.w0 to the memory address r7+118.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 120, 2		// Copy 2 bytes from r31.w0 to the memory address r7+120.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 122, 2		// Copy 2 bytes from r31.w0 to the memory address r7+122.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 124, 2		// Copy 2 bytes from r31.w0 to the memory address r7+124.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 126, 2		// Copy 2 bytes from r31.w0 to the memory address r7+126.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 128, 2		// Copy 2 bytes from r31.w0 to the memory address r7+128.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 130, 2		// Copy 2 bytes from r31.w0 to the memory address r7+130.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 132, 2		// Copy 2 bytes from r31.w0 to the memory address r7+132.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 134, 2		// Copy 2 bytes from r31.w0 to the memory address r7+134.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 136, 2		// Copy 2 bytes from r31.w0 to the memory address r7+136.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 138, 2		// Copy 2 bytes from r31.w0 to the memory address r7+138.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 140, 2		// Copy 2 bytes from r31.w0 to the memory address r7+140.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 142, 2		// Copy 2 bytes from r31.w0 to the memory address r7+142.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 144, 2		// Copy 2 bytes from r31.w0 to the memory address r7+144.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 146, 2		// Copy 2 bytes from r31.w0 to the memory address r7+146.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 148, 2		// Copy 2 bytes from r31.w0 to the memory address r7+148.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 150, 2		// Copy 2 bytes from r31.w0 to the memory address r7+150.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 152, 2		// Copy 2 bytes from r31.w0 to the memory address r7+152.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 154, 2		// Copy 2 bytes from r31.w0 to the memory address r7+154.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 156, 2		// Copy 2 bytes from r31.w0 to the memory address r7+156.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 158, 2		// Copy 2 bytes from r31.w0 to the memory address r7+158.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 160, 2		// Copy 2 bytes from r31.w0 to the memory address r7+160.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 162, 2		// Copy 2 bytes from r31.w0 to the memory address r7+162.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 164, 2		// Copy 2 bytes from r31.w0 to the memory address r7+164.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 166, 2		// Copy 2 bytes from r31.w0 to the memory address r7+166.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 168, 2		// Copy 2 bytes from r31.w0 to the memory address r7+168.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 170, 2		// Copy 2 bytes from r31.w0 to the memory address r7+170.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 172, 2		// Copy 2 bytes from r31.w0 to the memory address r7+172.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 174, 2		// Copy 2 bytes from r31.w0 to the memory address r7+174.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 176, 2		// Copy 2 bytes from r31.w0 to the memory address r7+176.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 178, 2		// Copy 2 bytes from r31.w0 to the memory address r7+178.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 180, 2		// Copy 2 bytes from r31.w0 to the memory address r7+180.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 182, 2		// Copy 2 bytes from r31.w0 to the memory address r7+182.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 184, 2		// Copy 2 bytes from r31.w0 to the memory address r7+184.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 186, 2		// Copy 2 bytes from r31.w0 to the memory address r7+186.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 188, 2		// Copy 2 bytes from r31.w0 to the memory address r7+188.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 190, 2		// Copy 2 bytes from r31.w0 to the memory address r7+190.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 192, 2		// Copy 2 bytes from r31.w0 to the memory address r7+192.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 194, 2		// Copy 2 bytes from r31.w0 to the memory address r7+194.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 196, 2		// Copy 2 bytes from r31.w0 to the memory address r7+196.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 198, 2		// Copy 2 bytes from r31.w0 to the memory address r7+198.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 200, 2		// Copy 2 bytes from r31.w0 to the memory address r7+200.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 202, 2		// Copy 2 bytes from r31.w0 to the memory address r7+202.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 204, 2		// Copy 2 bytes from r31.w0 to the memory address r7+204.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 206, 2		// Copy 2 bytes from r31.w0 to the memory address r7+206.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 208, 2		// Copy 2 bytes from r31.w0 to the memory address r7+208.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 210, 2		// Copy 2 bytes from r31.w0 to the memory address r7+210.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 212, 2		// Copy 2 bytes from r31.w0 to the memory address r7+212.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 214, 2		// Copy 2 bytes from r31.w0 to the memory address r7+214.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 216, 2		// Copy 2 bytes from r31.w0 to the memory address r7+216.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 218, 2		// Copy 2 bytes from r31.w0 to the memory address r7+218.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 220, 2		// Copy 2 bytes from r31.w0 to the memory address r7+220.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 222, 2		// Copy 2 bytes from r31.w0 to the memory address r7+222.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 224, 2		// Copy 2 bytes from r31.w0 to the memory address r7+224.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 226, 2		// Copy 2 bytes from r31.w0 to the memory address r7+226.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 228, 2		// Copy 2 bytes from r31.w0 to the memory address r7+228.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 230, 2		// Copy 2 bytes from r31.w0 to the memory address r7+230.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 232, 2		// Copy 2 bytes from r31.w0 to the memory address r7+232.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 234, 2		// Copy 2 bytes from r31.w0 to the memory address r7+234.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 236, 2		// Copy 2 bytes from r31.w0 to the memory address r7+236.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 238, 2		// Copy 2 bytes from r31.w0 to the memory address r7+238.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 240, 2		// Copy 2 bytes from r31.w0 to the memory address r7+240.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 242, 2		// Copy 2 bytes from r31.w0 to the memory address r7+242.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 244, 2		// Copy 2 bytes from r31.w0 to the memory address r7+244.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 246, 2		// Copy 2 bytes from r31.w0 to the memory address r7+246.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 248, 2		// Copy 2 bytes from r31.w0 to the memory address r7+248.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 250, 2		// Copy 2 bytes from r31.w0 to the memory address r7+250.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 252, 2		// Copy 2 bytes from r31.w0 to the memory address r7+252.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r7, 254, 2		// Copy 2 bytes from r31.w0 to the memory address r7+254.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 0, 2		// Copy 2 bytes from r31.w0 to the memory address r8+0.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 2, 2		// Copy 2 bytes from r31.w0 to the memory address r8+2.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 4, 2		// Copy 2 bytes from r31.w0 to the memory address r8+4.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 6, 2		// Copy 2 bytes from r31.w0 to the memory address r8+6.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 8, 2		// Copy 2 bytes from r31.w0 to the memory address r8+8.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 10, 2		// Copy 2 bytes from r31.w0 to the memory address r8+10.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 12, 2		// Copy 2 bytes from r31.w0 to the memory address r8+12.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 14, 2		// Copy 2 bytes from r31.w0 to the memory address r8+14.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 16, 2		// Copy 2 bytes from r31.w0 to the memory address r8+16.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 18, 2		// Copy 2 bytes from r31.w0 to the memory address r8+18.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 20, 2		// Copy 2 bytes from r31.w0 to the memory address r8+20.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 22, 2		// Copy 2 bytes from r31.w0 to the memory address r8+22.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 24, 2		// Copy 2 bytes from r31.w0 to the memory address r8+24.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 26, 2		// Copy 2 bytes from r31.w0 to the memory address r8+26.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 28, 2		// Copy 2 bytes from r31.w0 to the memory address r8+28.
	WBS  r31, 16			// Wait for DCLK going high.
	WBC  r31, 16			// Wait for falling edge.
	SBBO r31.w0, r8, 30, 2		// Copy 2 bytes from r31.w0 to the memory address r8+30.
//	WBS  r31, 16			// Wait for DCLK going high.

	MOV r10, 1312			// 2 * 328 * 2 = 1312 (two bytes for each column from top and bottom - including dummy columns)
	ADD r3, r2, r10			// r3 = r2 + 1312
	
// Clear local memory - just for debugging purposes.	
//	MOV  r4, 0x00000100
////	MOV  r5, 0x00000617  		// Initialize final pointer
//	MOV  r5, 0x0000061F
//	MOV  r6, 0xFFFFFFFF
//CLEAR_MEM_2:	
//	SBBO r6, r4, 0, 4		// Copy 4 bytes from r6 to the memory address r4+0
//	ADD  r4, r4, 4			// Update/increase pointer		
//	QBGT CLEAR_MEM_2, r4, r5 	// Repeat if pointer did not reach the last address yet

//	MOV  r4,  0x00000000
//	MOV  r5,  0x00000000
//	MOV  r6,  0x00000000
//	MOV  r7,  0x00000000
//	MOV  r8,  0x00010001
//	MOV  r9,  0x00010001
//	MOV  r10, 0x00010001
//	MOV  r11, 0x00010001
//	MOV  r12, 0x00010001
//	MOV  r13, 0x00010001
//	MOV  r14, 0x00010001
//	MOV  r15, 0x00010001	
//	MOV  r16, 0x00010001

//	MOV r29, 495936
//	SUB r29, r1, r29
//	QBLT COPY_DATA, r29, r0	// quick branch to TERMINATE_FRAME if r0 <= r29

//	MOV  r4,  0x05550555
//	MOV  r5,  0x05550555
//	MOV  r6,  0x05550555
//	MOV  r7,  0x05550555
	
//	MOV  r4,  0x00550055
//	MOV  r5,  0x00550055
//	MOV  r6,  0x00550055
//	MOV  r7,  0x00550055
//	MOV  r8,  0x00100010
//	MOV  r9,  0x00100010
//	MOV  r10, 0x00100010
//	MOV  r11, 0x00100010
//	MOV  r12, 0x00100010
//	MOV  r13, 0x00100010
//	MOV  r14, 0x00100010
//	MOV  r15, 0x00100010	
//	MOV  r16, 0x00100010

//	MOV r29, 330624	
//	SUB r29, r1, r29
//	QBLT COPY_DATA, r29, r0	// quick branch to TERMINATE_FRAME if r0 <= r29

//	MOV  r4,  0x0AAA0AAA
//	MOV  r5,  0x0AAA0AAA
//	MOV  r6,  0x0AAA0AAA
//	MOV  r7,  0x0AAA0AAA	

//	MOV  r4,  0x00FF00FF
//	MOV  r5,  0x00FF00FF
//	MOV  r6,  0x00FF00FF
//	MOV  r7,  0x00FF00FF
//	MOV  r8,  0x01000100
//	MOV  r9,  0x01000100
//	MOV  r10, 0x01000100
//	MOV  r11, 0x01000100
//	MOV  r12, 0x01000100
//	MOV  r13, 0x01000100
//	MOV  r14, 0x01000100
//	MOV  r15, 0x01000100	
//	MOV  r16, 0x01000100

//	MOV r29, 165312
//	SUB r29, r1, r29
//	QBLT COPY_DATA, r29, r0	// quick branch to TERMINATE_FRAME if r0 <= r29

//	MOV  r4,  0x0FFF0FFF
//	MOV  r5,  0x0FFF0FFF
//	MOV  r6,  0x0FFF0FFF
//	MOV  r7,  0x0FFF0FFF		
	
//	MOV  r4,  0x0FFF0FFF
//	MOV  r5,  0x0FFF0FFF
//	MOV  r6,  0x0FFF0FFF
//	MOV  r7,  0x0FFF0FFF
//	MOV  r8,  0x0FFF0FFF
//	MOV  r9,  0x0FFF0FFF
//	MOV  r10, 0x0FFF0FFF
//	MOV  r11, 0x0FFF0FFF
//	MOV  r12, 0x0FFF0FFF
//	MOV  r13, 0x0FFF0FFF
//	MOV  r14, 0x0FFF0FFF
//	MOV  r15, 0x0FFF0FFF	
//	MOV  r16, 0x0FFF0FFF

	ADD  r2, r2, r12			// move memory address to first valid data
        
	SUB  r4, r3, r2
        QBGT COPY_DATA2, r4, 104
 
COPY_DATA1:
	LBBO r4, r2, 0, 104			// Load part of double row from local data memory.
	SBBO r4, r0, 0, 104			// Write the data to DDR memory.	
	ADD  r2, r2, 104			// Update pointer to local memory
	ADD  r0, r0, 104			// Update pointer to DDR memory
	SUB  r4, r3, r2				// Compute number of remaining bytes.
	QBLE COPY_DATA1, r4, 104    // Repeat if there are more than 104 bytes remaining.
	
	QBEQ TERMINATE_ROW, r4, 0	// Terminate row if all bytes have already been copied.
	
COPY_DATA2:
	LBBO r4, r2, 0, 4		// Load part of double row from local data memory.
	SBBO r4, r0, 0, 4		// Write the data to DDR memory.	
	ADD  r2, r2, 4			// Update pointer to local memory
	ADD  r0, r0, 4			// Update pointer to DDR memory
	SUB  r4, r3, r2			// Compute number of remaining bytes.
	QBLE COPY_DATA2, r4, 4  // Repeat if there are more than 4 bytes remaining.
	
//	LBBO r4, r2, 0, 64		// Load last portion of data from the local memory
//	SBBO r4, r0, 0, 64
//	ADD  r2, r2, 64		// Update pointer to local memory
//	ADD  r0, r0, 64		// Update pointer to DDR memory

TERMINATE_ROW:	
	QBGE TERMINATE_FRAME, r1, r0	// quick branch to TERMINATE_FRAME if r0 >= r1
	JMP  INIT_ROW
		
TERMINATE_FRAME:	
//#ifdef AM33XX
    // Send notification to Host for program completion	
	MOV R31.b0, PRU1_ARM_INTERRUPT+16
//#else
//    MOV R31.b0, PRU0_ARM_INTERRUPT
//#endif

// wait for interrupt
WBS r31, #30

JMP START

HALT
