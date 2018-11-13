#include "i2c.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

/// \addtogroup i2c
/// @{

/*!
 This function provides a simple interface to communicate over I2C using a Beagle Bone Black (BBB).
 @param dev_addr i2c address of the chip (default 0x20)
 @param op Operation - there are four possible operations:
 - 'r': Reads n_bytes_data bytes starting from register address reg_addr.
 - 'w': Writes n_bytes_data into the registers starting at register address reg_addr.
 - 's': Sends a software reset to all connected epc660 devices.
 - 'd': Scans the I2C bus for devices. The found I2C addresses will be put into the
 integer array at p_data.
 @param reg_addr First register address for reading or writing
 @param n_bytes_data Number of data bytes to read or write
 @param p_data Pointer to array of unsigned integers, which should contain the data bytes to send
 or will contain the data bytes read or the I2c addresses found. In case of a device scan,
 the list of I2C addresses will be terminated with the address 0xFF.
 @returns On success, 0, on error -1
 */
int i2c(unsigned int dev_addr, char op, unsigned int reg_addr, unsigned int n_bytes_data, unsigned char** p_data) {
	int i, k;
	int fd_i2c;
	char *filename = "/dev/i2c-2";

	unsigned char buf_wr[n_bytes_data + 1];
	unsigned char buf_rd[n_bytes_data + 1];

	int rd_len;
	int wr_len;

	if (dev_addr > 127) {
		printf("Device address is out of range.\n");
		return -1;
	}
	if (reg_addr > 255) {
		printf("Register address is out of range.\n");
		return -1;
	}

	if (op == 'w') {
		wr_len = n_bytes_data + 1;
	} else {
		wr_len = 1;
	}

	if (op == 'r') {
		rd_len = n_bytes_data;
	} else if (op == 'd') {
		rd_len = 1;
	} else {
		rd_len = 0;
	}

	struct i2c_msg rdwr_msgs[2] = { { .addr = dev_addr, //
			.flags = 0, // write
			.len = wr_len, //
			.buf = buf_wr }, { // Read buffer
			.addr = dev_addr, //
					.flags = 1, // read
					.len = rd_len, //
					.buf = buf_rd } };

	struct i2c_rdwr_ioctl_data rdwr_data = { .msgs = rdwr_msgs, .nmsgs = 2 };

	/* Setup: */

	if ((fd_i2c = open(filename, O_RDWR)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		perror("Failed to open the i2c bus.");
		return -1;
	}

	buf_wr[0] = reg_addr;

	/* Actual I2C communication: */

	switch (op) {
	case 'w':
		if (ioctl(fd_i2c, I2C_SLAVE, dev_addr) < 0) {
			printf("0 2\nFailed to acquire bus access and/or talk to slave.\n");
			close(fd_i2c); //
			return -1;
		}
		for (i = 1; i <= n_bytes_data; i++) {
			buf_wr[i] = (*p_data)[i - 1];
		}
		if (write(fd_i2c, buf_wr, wr_len) < 0) {
			printf("0 2\nWriting failed.\n");
			close(fd_i2c); //
			return -1;
		}
		break;
	case 'r':
		if (ioctl(fd_i2c, I2C_RDWR, &rdwr_data) < 0) {
			perror("rdwr ioctl error");
			close(fd_i2c); //
			return -1;
		} else {
			for (i = 0; i < n_bytes_data; ++i) {
				(*p_data)[i] = buf_rd[i];
			}
		}
		break;
	case 'd':
		dev_addr = 0x02;
		rdwr_data.msgs[0].addr = dev_addr;
		rdwr_data.msgs[1].addr = dev_addr;
		k = 0;

		for (i = 0; i < 0x75; i++) {
			dev_addr += 1;
			rdwr_data.msgs[0].addr = dev_addr;
			rdwr_data.msgs[1].addr = dev_addr;
			if (ioctl(fd_i2c, I2C_RDWR, &rdwr_data) >= 0) {
				(*p_data)[k] = dev_addr;
				k += 1;
			}
		}
		(*p_data)[k] = 0xFF; // put invalid address at the end of the list
		break;
	case 's':
		dev_addr = 0x00;
		if (ioctl(fd_i2c, I2C_SLAVE, dev_addr) < 0) {
			printf("0 2\nFailed to acquire bus access and/or talk to slave.\n");
			close(fd_i2c); //
			return -1;
		}
		rdwr_data.msgs[0].addr = 0x00; // device address for general call reset
		buf_wr[0] = 0x06; // register address for general call reset

		write(fd_i2c, buf_wr, 1);
		break;
	case 'l':
		dev_addr = 0x00;
		if (ioctl(fd_i2c, I2C_SLAVE, dev_addr) < 0) {
			printf("0 2\nFailed to acquire bus access and/or talk to slave.\n");
			close(fd_i2c); //
			return -1;
		}
		rdwr_data.msgs[0].addr = 0x00; // device address for general call i2c address reload
		buf_wr[0] = 0x04; // register address for general call i2c address reload
		write(fd_i2c, buf_wr, 1);
		break;
	default:
		printf("Invalid operation: Only 'd' (devices), 'r' (read), 'w' (write) and 's' (software reset) are allowed.\n");
		close(fd_i2c); //
		return -1;
	}

	close(fd_i2c);

	return 0;

}

/// @}
