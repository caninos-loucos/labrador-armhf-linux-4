#ifndef _LORA_DRIVER_H_
#define _LORA_DRIVER_H_

#define POOLING_DELAY (1000) /* wait 1ms */ 

#define SX127X_DRIVERNAME "sx127x"
#define SX127X_CLASSNAME  "sx127x"
#define SX127X_DEVICENAME "sx127x%d"

#define pr_fmt(fmt) SX127X_DRIVERNAME ": " fmt

#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/uaccess.h>

#include "sx127x.h"

struct sx127x
{
	struct device *chardevice;
	struct spi_device *spidevice;
	
	enum sx127x_opmode opmode;
	
	struct delayed_work tx_work;
	struct delayed_work rx_work;
	struct delayed_work cad_work;
	
	struct mutex mutex;
	u32 fosc;
	
	struct list_head device_entry;
	dev_t devt;
	bool open;
	
	/* device state */
	bool loraregmap;
	
	/* tx */
	wait_queue_head_t writewq;
	int transmitted;
	
	/* rx */
	wait_queue_head_t readwq;
	struct kfifo out;
};

#endif

