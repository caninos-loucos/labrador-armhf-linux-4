/*
 * Authors: Bruna Bazaluk M V, Edgar Righi
 *
 * sx127x's lora driver 
 * (some functions under construction and others not working)
*/


#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>
#include <linux/math64.h>
#include <regs.h>


extern int reset_lora_module(void); /* from board.c */

int sx127x_go_lora_rx(struct sx127x_priv);

static struct class *devclass;
static int devmajor, devminor;

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

struct lora_packet
{
    char data[512];
};


struct sx127x_priv
{
    struct spi_device *spi;
    
    struct list_head device_entry;
    bool opened;
    
    struct delayed_work rx_work;
    
    struct device *chardevice;
    dev_t devt; 
   
    atomic_t device_state;
    
    DECLARE_KFIFO(rx_fifo, struct lora_packet, FIFO_SIZE);
    DECLARE_KFIFO(tx_fifo, struct lora_packet, FIFO_SIZE);
    
    struct lora_packet tmp;
};

int sx127x_reg_read_noretry(struct sx127x_priv *sx127x, int reg)
{
	/* 0x7F most significant bit is set to 0 */
	u8 addr = reg & 0x7F;
	u8 result = 0;
	int ret;
	
	ret = spi_write_then_read(sx127x->spi, &addr, 1, &result, 1);
	
	/* Casting in case most significant bits are not 0 */
	return (ret < 0) ? ret : ((int)(result)) & 0xFF;
}

int sx127x_reg_write_noretry(struct sx127x_priv *sx127x, int reg, u8 value)
{
	u8 addr = reg & 0x7F;
	u8 buff[2];
	
	buff[0] = addr | (0x1 << 7);
	buff[1] = value;
	
	return spi_write(sx127x->spi, buff, 2);
}




//----------------------------write----n-bytes---------------------------------

int sx127x_reg_write_n(struct sx127x_priv *sx127x, int reg, u32 value)
{
	u8 addr = reg & 0x7F;
	u8 buff[255];
	int i;

	for(i; i<255; i--)
	{
		buff[i] = value & 0xFF; /* send last least-significant byte */
		value >>= 8;
	
	}
	/* convert value to big-endian */
	
	
	buff[0] = addr | (0x1 << 7);

	return spi_write(sx127x->spi, buff, sizeof(buff));
}

//------------------------------------------------------------------------------


int sx127x_reg_write24(struct sx127x_priv *sx127x, int reg, u32 value)
{
	u8 addr = reg & 0x7F;
	u8 buff[4];
	
	/* convert value to big-endian */
	buff[3] = value & 0xFF; /* send last least-significant byte */
	value >>= 8;
	buff[2] = value & 0xFF;
	value >>= 8;
	buff[1] = value & 0xFF; /* send first most-significant byte */
	
	buff[0] = addr | (0x1 << 7);
	
	return spi_write(sx127x->spi, buff, sizeof(buff));
}

int sx127x_reg_read24(struct sx127x_priv *sx127x, int reg)
{
	/* 0x7F bit mais significativo eh setado pra 0, indicando leitura */
	u8 addr = reg & 0x7F;
	u8 buff[3];
	int ret;
	
	ret = spi_write_then_read(sx127x->spi, &addr, 1, buff, sizeof(buff));
	
	if (ret < 0) {
	    return ret;
	}
	
	/* convert value from big-endian to little-endian */
	
	ret = buff[0] & 0xFF; /* most-significant byte is th first one */
	ret <<= 8;
	ret |= buff[1] & 0xFF;
	ret <<= 8;
	ret |= buff[2] & 0xFF; /* least-significant byte is the last one*/
	
	return ret;
}


//-------------------------read-com-n-bytes-------------------------------------

int sx127x_reg_read_n(struct sx127x_priv *sx127x, int reg, int n)
{
	/* 0x7F bit mais significativo eh setado pra 0, indicando leitura */
	u8 addr = reg & 0x7F;
	u8 buff[n];
	int ret,i;
	
	ret = spi_write_then_read(sx127x->spi, &addr, 1, buff, sizeof(buff));
	
	if (ret < 0) {
	    return ret;
	}
	
	/* convert value from big-endian to little-endian */
	for(i=0; i<n; i++)
	{
		ret = buff[i] & 0xFF; /* most-significant byte is th first one */
		ret <<= 8;
	
	}
	/* least-significant byte is the last one*/
	
	return ret;
}

//------------------------------------------------------------------------------


int sx127x_reg_read(struct sx127x_priv *sx127x, int reg)
{
    int ret, retry = RW_RETRY;
    
    do {
        ret = sx127x_reg_read_noretry(sx127x, reg);
        retry--;
        
    } while ((retry > 0) && (ret < 0));
    
    return ret;
}

int sx127x_reg_write(struct sx127x_priv *sx127x, int reg, u8 value)
{
    int ret, retry = RW_RETRY;
    
    do {
        ret = sx127x_reg_write_noretry(sx127x, reg, value);
        retry--;
        
    } while ((retry > 0) && (ret < 0));
    
    return ret;
}

int sx127x_reg_clear_set(struct sx127x_priv *sx127x, int reg, u8 mask, u8 value)
{
    /*Used many times to change modes and register values*/

    /* This function reads the value in the register, applies the mask     */
    /* and set value, reads again and compare values to check if it worked */
    
    u8 old_value;
    int ret;
    
    ret = sx127x_reg_read(sx127x, reg);
    
    if (ret < 0) {
        return ret;
    }
    
    old_value = ret & 0xFF;
    
    value &= mask;
    value |= old_value & ~mask;
    
    if (value == old_value) {
        return 0;
    }
    
    ret = sx127x_reg_write(sx127x, reg, value);
    
    if (ret < 0) {
        return ret;
    }
    
    old_value = value;
    
    ret = sx127x_reg_read(sx127x, reg);
    
    if (ret < 0) {
        return ret;
    }
    
    value = ret & 0xFF;
    
    /*If value does not change,value is ! from old_value, the operation failed*/
    return (value == old_value) ? 0 : -EINVAL;
}

int sx127x_go_sleep(struct sx127x_priv *sx127x)
{
    int ret;
    
    ret = sx127x_reg_clear_set(sx127x, REG_OPMODE,
                               REG_OPMODE_FSKOOK_MODE_MSK,
                               REG_OPMODE_FSKOOK_MODE_SLEEP);
    return ret;
}

int sx127x_is_lora_mode(struct sx127x_priv *sx127x)
{
    int ret = -EINVAL;
    
    ret = sx127x_reg_read(sx127x, REG_OPMODE);
    
    if (ret >= 0)
    {
        ret &= ~REG_OPMODE_FSKOOK_LORAMODE_MSK;
        return (ret == REG_OPMODE_FSKOOK_LORAMODE_LORA);
    }
    
    return ret;
}

int sx127x_get_version(struct sx127x_priv *sx127x)
{
    int ret;
    
    ret = sx127x_reg_read(sx127x, REG_VERSION);
    
    return ret;
}

int sx127x_go_lora(struct sx127x_priv *sx127x)
{
    int ret;
    
    ret = sx127x_is_lora_mode(sx127x);
    
    if (ret < 0) {
        return ret;
    }
    
    if (ret > 0) { /* already at Lora mode, nothing to do */
        return 0;
    }
	
	/* To configure RF carrier frequency to 915MHz (must be in sleep mode): */
	/* Operation especified in datasheet */
	u64 aux = (u64)DEF_LORA_RF_CARRIER_FREQ << 19;
	
	aux = div_u64(aux, DEF_CRYSTAL_OSC_FREQ);
	
	
	ret = sx127x_reg_write(sx127x, REG_FR_MSB, aux & 0xFFFFFF);
	
	
	/* Setting gain*/
	ret = sx127x_reg_clear_set(sx127x, REG_LNA,
       							REG_LORA_LNA_GAIN_MASK,
 								LNA_GAIN_G1);



	/* Turn PA boost to 1*/
	ret = sx127x_reg_clear_set(sx127x, REG_PA_CONFIG,
                                   REG_PA_CONFIG_SELECT_MSK,
                                   REG_PA_CONFIG_SELECT_PA_BOOST);

 
	/*Configuring bandwidth to 125kHz*/
	ret = sx127x_reg_clear_set(sx127x, REG_LORA_CONF1,
                                   REG_LORA_CONF1_BW_MSK,
                                   REG_LORA_CONF1_BW_125);

	
	/*Configuring spreading factor to 12*/
	ret = sx127x_reg_clear_set(sx127x, REG_LORA_CONF2,
								   REG_LORA_CONF2_SPREADING_FACTOR_MSK, 
								   REG_LORA_CONF2_SPREADING_FACTOR);

    
    return sx127x_reg_clear_set(sx127x, REG_OPMODE,
                                   REG_OPMODE_FSKOOK_LORAMODE_MSK,
                                   REG_OPMODE_FSKOOK_LORAMODE_LORA);
}





int sx127x_go_lora_tx(struct sx127x_priv *sx127x)
{ 
	int ret = -EINVAL;
	
//----------------------------------------------------------------------------->
    
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGSMASK, 0x0);
	/* Disable all interruptions, clean flags*/    
    
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
    
    /*set TxDoneMask (bit 3) */
    
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGSMASK, BIT(3));
    
//<-----------------------------------------------------------------------------
        
	//set tx mode
	ret = sx127x_reg_clear_set(sx127x, REG_OPMODE,
							   REG_OPMODE_LORA_MSK,
							   REG_OPMODE_LORA_TX);


	// set AccesSharedReg to 1
	ret = sx127x_reg_clear_set(sx127x, 
							   REG_OPMODE,REG_OPMODE_LORA_SHARED_ACCESS_MSK, 
							   REG_OPMODE_LORA_SHARED_ACCESS_ON);

	/* It does not happen on LORA's registers, but FSK/OOK */
	if(sx127x_reg_read(sx127x, REG_IRQ_FLAGS1_MODEREADY) < 0 )
		return dev_err(sx127x_go_lora_rx, "could not enter in rx mode");
	// set AccesSharedReg back to 0
	
	ret = sx127x_reg_clear_set(sx127x, REG_OPMODE,REG_OPMODE_LORA_SHARED_ACCESS_MSK, 0);

}




int sx127x_go_lora_rx(struct sx127x_priv *sx127x)
{ 
	int ret = -EINVAL;
	
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGSMASK, 0x0);
    
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
    
    /* RxDoneMask (bit 6) | ValidHeaderMask (bit 4)*/
    
    ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGSMASK, BIT(6) | BIT(4));
    
	ret = sx127x_reg_clear_set(sx127x, REG_OPMODE,
							   REG_OPMODE_LORA_MSK,
							   REG_OPMODE_LORA_RX_CONTINUOUS);


	// set AccesSharedReg para 1

	ret = sx127x_reg_clear_set(sx127x, REG_OPMODE,
							   REG_OPMODE_LORA_SHARED_ACCESS_MSK,
							   REG_OPMODE_LORA_SHARED_ACCESS_ON);

		if(sx127x_reg_read(sx127x, REG_IRQ_FLAGS1_MODEREADY) < 0 )
		return dev_err(sx127x_go_lora_rx, "could not enter in rx mode");

	// set AccesSharedReg para 0
	
	ret = sx127x_reg_clear_set(sx127x, 
							   REG_OPMODE,REG_OPMODE_LORA_SHARED_ACCESS_MSK, 0);


}

static void rx_worker(struct work_struct *work)
{
    struct delayed_work * dwork = to_delayed_work(work);
    struct sx127x_priv *sx127x =
        container_of(dwork, struct sx127x_priv, rx_work);
    int ret;
	int lastError;
    
    if (atomic_sub_and_test(0, &sx127x->device_state))
    {
        ret = sx127x_go_lora_rx(sx127x);
        
        if (!ret) {
            atomic_inc(&sx127x->device_state);
        }
        else
        {
            lastError = dev_err(spi->dev, "rx_worker failed");
        }
    }
    else
    {
        u8 irq_flags; 
        u8 fifo_add;
		u8 last_pkg_add;
		u8 bytes_read;

		ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGSMASK, 0x0);

		irq_flags = sx127x_reg_read(sx127x, REG_IRQ_FLAGS);
		
		//Clean device's regs
    	ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
	

       	/* If irq_flags is RxDone, verify ValidHeaderMask. */
		/* If true, read device's fifo copyng to a local buffer */
        
		if ((irq_flags & BIT(6))) //rxdone
		{
			if((irq_flags & BIT(4)))
			{
				/* RegFifoRxByteAddr = where fifo ends after receiving n bytes
				 * RegFifoRxCurrentAddr = where the last package starts*/

				int i;

				fifo_add = sx127x_reg_read(sx127x,REG_LORA_FIFO_RX_BYTE_ADDRES);

				last_pkg_add = sx127x_reg_read(sx127x,REG_LORA_FIFO_RX_CURR_ADDRESS);
		
        		// n bytes = (payload + header)
				bytes_read = fifo_add - last_pkg_add ;
        
				sx127x_reg_clear_set(sx127x, REG_LORA_FIFO_RX_BASE_ADDRESS,
										   (0xFF), last_pkg_add);

				sx127x->tmp.data = sx127x_reg_read_n(sx127x, REG_FIFO, bytes_read);

				ret = kfifo_put(&sx127x->rx_fifo, sx127x->tmp);
				
				if(REG_OPMODE_FSKOOK_MODE_STDBY)
				{
					lastError = dev_err(spi->dev, "standby in rx mode");
				}
				else
				{
					ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
				}
							if (!ret) // fifo full
				{
					lastError = dev_err(spi->dev, "fifo full");
				}
				
	
				ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
				// set last error but keeps on rx mode
		
			}
		}
        
        if (1) 
        {
            atomic_sub(&sx127x->device_state,0); // return to state = 0
            
            lastError = dev_err(spi->dev, "rx_worker failed");
        }

//------------------------------------TXWORKER----------------------------------
        
        else
		{
            if (!kfifo_is_empty(tx_fifo))
            {
				ret = sx127x_go_lora_tx(sx127x);

				ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);
	

				irq_flags = sx127x_reg_read(sx127x, REG_IRQ_FLAGS);
		
				ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);

				sx127x_reg_write_n(sx127x, REG_FIFO, sx127x->tmp.data[i]);

				/* when it receives txdone it goes automatically to standby */

			}
		}
        
        if (1) 
        {
            atomic_sub(&sx127x->device_state,0); 
            
            lastError = dev_err(spi->dev, "tx_worker failed");
 
				/* Check tx */
                if(irq_flags & BIT(3))
				{
            		lastError = dev_err(spi->dev, "tx_worker failed");
				}

				ret = sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xFF);

				ret = sx127x_reg_clear_set(sx127x, REG_OPMODE, 
					  REG_OPMODE_FSKOOK_MODE_MSK,REG_OPMODE_FSKOOK_MODE_STDBY);
            }
            
        	/*Change imediatelly to state 0*/    
            atomic_sub(&sx127x->device_state,0);
			
			/* schedule one jiffies from now (current jiffies) */
            schedule_delayed_work(&sx127x->rx_work, jiffies + 1);
            return;
        }
        
        
    }
    
//---------------------------------TXWORKER-------------------------------------




    schedule_delayed_work(&sx127x->rx_work,
                          msecs_to_jiffies(RX_WORK_POOLING_PERIOD));
}

#define SX127X_DRIVERNAME "sx127x"
#define SX127X_CLASSNAME  "sx127x"

static int sx127x_probe(struct spi_device *spi)
{
    struct sx127x_priv *sx127x;
    struct device *dev = &spi->dev;
    int ret, value;
    
    /* devm_kzalloc frees automaticaly */
    sx127x = devm_kzalloc(dev, sizeof(*sx127x), GFP_KERNEL);
	
	if(!sx127x)
	{
		dev_err(dev, "could not allocate driver private data.\n");
		return -ENOMEM;
	}
	
	spi_set_drvdata(spi, sx127x);
	
	sx127x->spi = spi;
	
	if (reset_lora_module())
	{
		dev_err(dev, "could not reset lora module.\n");
		return -ENODEV;
	}
	
	
	/* After reset, the module is in both standby mode and in FSK/OOK mode */
	/* Check the module version (also checks if spi is working)            */
	
	value = sx127x_reg_read(sx127x, REG_VERSION);
	
	if (value < 0)
	{
	    dev_err(dev, "could not read lora module version.\n");
		return value;
	}
	
	if ((value & 0xFF) != 0x22)
	{
	    dev_err(dev, "invalid module version 0x%x != 0x22.\n", value & 0xFF);
		return -EINVAL;
	}
	
	/* We can only enter Lora Mode from Sleep Mode, so go to Sleep Mode. */
	/* First, enter Sleep Mode */
	
	ret = sx127x_go_sleep(sx127x);
	
	if (ret)
	{
	    dev_err(dev, "could not enter sleep mode.\n");
	    return ret;
	}
	
	/* Finally, go to Lora Mode */
	
	ret = sx127x_go_lora(sx127x);
	
	if (ret)
	{
	    dev_err(dev, "could not enter lora mode.\n");
	    return ret;
	}
	
	INIT_DELAYED_WORK(sx127x->rx_work, rx_worker);
	
	ret = schedule_delayed_work(&sx127x->rx_work,
	                            msecs_to_jiffies(RX_WORK_POOLING_PERIOD));
	
	if (ret)
	{
	    dev_err(dev, "could not start rx delayed work.\n");
	    return ret;
	}
	
	INIT_LIST_HEAD(&sx127x->device_entry);
	
	mutex_lock(&device_list_lock);
	
	sx127x->devt = MKDEV(devmajor, devminor);
	
	sx127x->chardevice = device_create(devclass, dev, sx127x->devt, sx127x,
	                                   "%s%d", SX127X_CLASSNAME, devminor);
	
	if (IS_ERR(sx127x->chardevice))
	{
	     cancel_delayed_work(&sx127x->rx_work);
	     mutex_unlock(&device_list_lock);
	     return -ENOMEM;
	}
	
	list_add(&sx127x->device_entry, &device_list);
	
	devminor++;
	mutex_unlock(&device_list_lock);
    return 0;
}

static int sx127x_remove(struct spi_device *spi)
{
    struct sx127x_priv *sx127x = spi_get_drvdata(spi);
    
    device_destroy(devclass, sx127x->devt);
    
    mutex_lock(&device_list_lock);
    
    list_del(&sx127x->device_entry);
    
    mutex_unlock(&device_list_lock);
    
    cancel_delayed_work(&sx127x->rx_work);
    
    return 0;
}

static int sx127x_dev_open(struct inode *inode, struct file *file)
{
    struct sx127x_priv *sx127x;
	int status = -ENXIO;
	int minor = iminor(inode);
	
	mutex_lock(&device_list_lock);
	
	list_for_each_entry(sx127x, &device_list, device_entry)
	{
		if (sx127x->devt == inode->i_rdev)
		{
			status = 0;
			break;
		}
	}
	
	if(status)
	{
		mutex_unlock(&device_list_lock);
		return status;
	}
	
	if(sx127x->opened)
	{
		dev_err(sx127x->chardevice, "device file already open.\n");
		mutex_unlock(&device_list_lock);
		return -EBUSY;
	}
	
	sx127x->opened = true;
	file->private_data = sx127x;
	
	mutex_unlock(&device_list_lock);
	return 0;
}
//----------------------------------------dev_read------------------------->>>>>

/* This function is under construction */
static ssize_t sx127x_dev_read
	(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct sx127x_priv *sx127x;
	int ret = 0;
	
	mutex_lock(&device_list_lock);

	if (!REG_OPMODE_LORA_RX_CONTINUOUS)
	{
		dev_err(struct sx127x_priv *sx127x, "device is not in receive mode.\n");
		mutex_unlock(&device_list_lock);
		return -EINVAL;
	}
	
	sx127x_reg_write(sx127x, REG_IRQ_FLAGS, 0xff);
	
	sx127x_reg_write(sx127x, REG_IRQ_FLAGSMSK, 
		~(REG_IRQ_FLAGS_RXDONE | REG_IRQ_FLAGS_PAYLOAD_CRCERROR));
	
	mutex_unlock(&device_list_lock);

	ret = kfifo_to_user(rx_fifo, buf, count, &copied);

	if(!ret && copied > 0){
		ret = copied;
	}
	mutex_unlock(&device_list_lock);
	return ret;
}


//<<<<<-------------------------------------------------------------------------


//---------------------------------dev_write------------------------------>>>>>>

sx127x_dev_write
{
	struct sx127x *data = filp->private_data;
	size_t packetsz, offset, maxpkt = 256;
	unsigned long n;
	bool transmit;
	u8 kbuf[256];

	if (!REG_OPMODE_LORA_TX)
	{
		dev_err(data->chardevice, "device is not in transmit mode.\n");
		return -EINVAL;
	}
	
	for(offset = 0; offset < count; offset += maxpkt)
	{
		packetsz = min((count - offset), maxpkt);
		
		mutex_lock(&data->mutex);
		n = copy_from_user(kbuf, buf + offset, packetsz);
		if( n!= 0 ){
			pr_err("failed copying data from user");
		} 
		
		sx127x_reg_clear_set(sx127x, REG_OPMODE,REG_OPMODE_FSKOOK_MODE_MSK,
							REG_OPMODE_FSKOOK_MODE_STDBY);

		sx127x_fifo_writepkt(data->spidevice, kbuf, packetsz);

		data->transmitted = false;
		sx127x_setopmode(data, data->opmode);
		
		sx127x_reg_write(data->spidevice, REG_IRQFLAGS, 0xff);
		
		sx127x_reg_write(data->spidevice, REG_IRQFLAGSMSK, 
			~(REG_IRQFLAGS_TXDONE));
		

		mutex_unlock(&data->mutex);
		
		schedule_delayed_work(&data->tx_work, usecs_to_jiffies(POOLING_DELAY));
		
		wait_event_interruptible_timeout(data->writewq, data->transmitted, 60 * HZ);

		
	}
	mutex_lock(&data->mutex);
	data->opmode = sx127x_getopmode(data);
	mutex_unlock(&data->mutex);

	return count;
}
//<<<<<<<-----------------------------------------------------------------------

	
struct file_operations fops = {
	.open = sx127x_dev_open,
	.read = sx127x_dev_read,
	.write = sx127x_dev_write,
	//.release = sx127x_dev_release,
	//.unlocked_ioctl = sx127x_dev_ioctl,
};

static const struct of_device_id sx127x_of_match[] = {
	{ .compatible = "semtech, sx127x", },
	{ },
};

MODULE_DEVICE_TABLE(of, sx127x_of_match);

static struct spi_driver sx127x_driver = {
	.probe  = sx127x_probe,
	.remove = sx127x_remove,
	.driver = {
		.name = SX127X_DRIVERNAME,
		.of_match_table = of_match_ptr(sx127x_of_match),
		.owner = THIS_MODULE,
	},
};

static int __init sx127x_init(void)
{
    int ret;
    
    ret = register_chrdev(0, SX127X_DRIVERNAME, &fops);
	
	if(ret < 0)
	{
		pr_err("failed to register sx127x char device\n");
		return ret;
	}
    
    devmajor = ret;
    devminor = 0;
	devclass = class_create(THIS_MODULE, SX127X_CLASSNAME);
	
	if(!devclass)
	{
		pr_err("failed to register class\n");
		unregister_chrdev(devmajor, SX127X_DRIVERNAME);
		return -ENOMEM;
	}
    
    ret = spi_register_driver(&sx127x_driver);
	
	if(ret)
	{
	    unregister_chrdev(devmajor, SX127X_DRIVERNAME);
	    class_destroy(devclass);
		return ret;
	}
	
	return 0;
}

module_init(sx127x_init);

static void __exit sx127x_exit(void)
{
	spi_unregister_driver(&sx127x_driver);
	unregister_chrdev(devmajor, SX127X_DRIVERNAME);
	class_destroy(devclass);
}

module_exit(sx127x_exit);

MODULE_LICENSE("GPL");
