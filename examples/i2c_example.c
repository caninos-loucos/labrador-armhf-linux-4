/*@About this example:
    >Name: i2c_example.c
    >Author: Igor Ruschi Andrade e Lima <igor.lima@lsitec.org.br>
    >About: This is a simple example to use the I2C interface available on Labrador.
    >What it does?: In this example the I2C-2 is used to read the chip-id of sensor Bmp180.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>


#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

typedef unsigned char   u8;

// Global file descriptor used to talk to the I2C bus:
int i2c_fd = -1;

// Choose the I2C-x to use
const char *i2c_fname = "/dev/i2c-2";

// Returns a new file descriptor for communicating with the I2C bus:
int i2c_init(void) {
    if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0) {
        char err[200];
        sprintf(err, "open('%s') in i2c_init", i2c_fname);
        perror(err);
        return -1;
    }

    // NOTE we do not call ioctl with I2C_SLAVE here because we always use the I2C_RDWR ioctl operation to do
    // writes, reads, and combined write-reads. I2C_SLAVE would be used to set the I2C slave address to communicate
    // with. With I2C_RDWR operation, you specify the slave address every time. There is no need to use normal write()
    // or read() syscalls with an I2C device which does not support SMBUS protocol. I2C_RDWR is much better especially
    // for reading device registers which requires a write first before reading the response.

    return i2c_fd;
}

void i2c_close(void) {
    close(i2c_fd);
}

// Write to an I2C slave device's register:
int i2c_write(u8 slave_addr, u8 reg, u8 data) {
    int retval;
    u8 outbuf[2];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_write");
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(u8 slave_addr, u8 reg, u8 *result) {
    int retval;
    u8 outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    *result = 0;
    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return -1;
    }

    *result = inbuf[0];
    return 0;
}

void main(void){
	u8 slave_addr = 0x77; //i2c address of Bmp180
	u8 reg = 0xD0; //register address of Bmp180 with chip-id 0x55
	u8 res; //the result is stored in res
	int ret; //return of operation

    //start I2c
	i2c_init(); 

    //Use read operation of I2C
	ret = i2c_read(slave_addr, reg, &res);
    if(ret){
        printf("Error with i2c read operation!\n");
    }else{
        printf("I2C_READ = 0x%x\n",res);
    }
}
