#ifndef _EPC660_GPIO_H_
#define _EPC660_GPIO_H_

/**************************************************************************
* Mapping of epc660 pins to BBB GPIOs                                     *
**************************************************************************/

/* Header P8: odd pins */
#define BBB_TRIANG_LED_ENABLE  23   /* P8_13 */
#define BBB_BOOT_DISABLE       22   /* P8_19 */
#define BBB_XSYNC_SAT_CFG      62   /* P8_21 */
#define TESTAOHA_BUFF          36   /* P8_23: no connection to BBB by default */
#define TESTAIHB               32   /* P8_25: no connection to BBB by default */
#define BBB_DATA9              87   /* P8_29 */
#define VFSAB                  10   /* P8_31: no connection to BBB by default */
#define TESTAIO2                9   /* P8_33: no connection to BBB by default */
#define EPC660_TCK              8   /* P8_35 */
#define EPC660_TDO             78   /* P8_37 */
#define BBB_DATA6              76   /* P8_39 */
#define BBB_DATA4              74   /* P8_41 */
#define BBB_DATA2              72   /* P8_43 */
#define BBB_DATA0_FS           70   /* P8_45 */

/* Header P8: even pins */
#define EPC660_RESET_BAR       26   /* P8_14 */ 
#define BBB_V_LED_ENABLE       46   /* P8_16 */ 
#define BBB_HSYNC_A1           63   /* P8_20 */
#define VSEAL                  37   /* P8_22: no connection to BBB by default */ 
#define TESTAOHB_BUFF          33   /* P8_24: no connection to BBB by default */ 
#define TESTAIHA               61   /* P8_26: no connection to BBB by default */ 
#define BBB_DATA10             88   /* P8_28 */ 
#define BBB_DATA11             89   /* P8_30 */ 
#define TESTAI_VFSDG           11   /* P8_32: no connection to BBB by default */ 
#define TESTAIO1               81   /* P8_34: no connection to BBB by default */ 
#define EPC660_TMS             80   /* P8_36 */ 
#define EPC660_TDI             79   /* P8_38 */ 
#define BBB_DATA7              77   /* P8_40 */ 
#define BBB_DATA5              75   /* P8_42 */ 
#define BBB_DATA3              73   /* P8_44 */ 
#define BBB_DATA1              71   /* P8_46 */ 

/* Header P9: odd pins */
#define BBB_5V_10V_ENABLE      30  /* P9_11 */
#define BBB_EPC660_ENABLE      31  /* P9_13 */
#define BBB_EPC660_SHUTTER     48  /* P9_15 */
#define BBB_EPC660_SCL          5  /* P9_17 */
#define EEPROM_SCL             13  /* P9_19 */
#define VSSLED_1                3  /* P9_21: no connection to BBB by default */
#define ILLB_EPC660_LED_1      49  /* P9_23: no connection to BBB by default */
#define BBB_VSYNC_A0             117  /* P9_25 */
#define ILLB_EPC660_LEDFB     115  /* P9_27: no connection to BBB by default */
#define BBB_SPI1_D0           111  /* P9_29 */
#define BBB_SPI1_SCLK         110  /* P9_31 */

/* Header P9: even pins */
#define BBB_3V3_ENABLE         60  /* P9_12 */
#define BBB_MINUS_5V_ENABLE    50  /* P9_14 */
#define EPC660_MODCLK          51  /* P9_16 */
#define BBB_EPC660_SDA          4  /* P9_18 */
#define EEPROM_SDA             12  /* P9_20 */
#define VSSLED_2                2  /* P9_22: no connection to BBB by default */
#define ILLB_EPC660_LED_2      15  /* P9_24: no connection to BBB by default */
#define BBB_DCLK               14  /* P9_26 */
#define BBB_SPI1_CS0          113  /* P9_28 */
#define BBB_SPI1_D1           112  /* P9_30 */
#define BBB_LED_ENABLE	        7  /*P9_42*/

#endif
