
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/kobject.h>
#include <linux/timekeeping.h>

#include <asm/suspend.h>
#include <mach/hardware.h>
#include <mach/power.h>

#include "pmic_regs.h"

/* Hibernation and suspend events */
#define PM_HIBERNATION_PREPARE	0x0001 /* Going to hibernate */
#define PM_POST_HIBERNATION	0x0002 /* Hibernation finished */
#define PM_SUSPEND_PREPARE	0x0003 /* Going to suspend the system */

#define MASK_2_BITS					(0x3)

//Low power state enter S4 voltage
#define SYS_CTL1_LB_S4_2_9V 		(0x0)
#define SYS_CTL1_LB_S4_3_0V 		(0x1 << 3)
#define SYS_CTL1_LB_S4_3_1V 		(0x2 << 3)
#define SYS_CTL1_LB_S4_3_3V 		(0x3 << 3)


//Wakeup sources definitions
//VBUS wakeup enable
#define SYS_CTL0_USB_WK_EN					(0x1 << 15)
#define SYS_CTL0_WALL_WK_EN					(0x1 << 14)
#define SYS_CTL0_ONOFF_LONG_WK_EN			(0x1 << 13)
#define SYS_CTL0_ONOFF_SHORT_WK_EN			(0x1 << 12)
#define SYS_CTL0_SGPIOIRQ_WK_EN				(0x1 << 11)
#define SYS_CTL0_RESTART_EN					(0x1 << 10)
#define SYS_CTL0_REM_CON_WK_EN				(0x1 << 9)
#define SYS_CTL0_ALARM_WK_EN				(0x1 << 8)
#define SYS_CTL0_HDSW_WK_EN					(0x1 << 7)
#define SYS_CTL0_RESET_WK_EN				(0x1 << 6)
#define SYS_CTL0_IR_WK_EN					(0x1 << 5)
#define SYS_CTL0_VBUS_WK_TH_4_05V			(0x0 << 3)
#define SYS_CTL0_VBUS_WK_TH_4_20V			(0x1 << 3)
#define SYS_CTL0_VBUS_WK_TH_4_35V			(0x2 << 3)
#define SYS_CTL0_VBUS_WK_TH_4_50V			(0x3 << 3)
#define SYS_CTL0_WALL_WK_TH_4_05V			(0x0 << 1)
#define SYS_CTL0_WALL_WK_TH_4_20V			(0x1 << 1)
#define SYS_CTL0_WALL_WK_TH_4_35V			(0x2 << 1)
#define SYS_CTL0_WALL_WK_TH_4_50V			(0x3 << 1)
#define SYS_CTL0_ONOFF_MUXKEY_EN			(0x1 << 0)

//all wakeup enable mask
#define SYS_CTL0_WK_ALL						(0xFBE0)

#define SYS_CTL3_S2S3TOS1TIMER_EN			(0x1 << 9)


#define SYS_CTL1_EN_S1						(0x1 << 0)
#define SYS_CTL3_EN_S2						(0x1 << 15)
#define SYS_CTL3_EN_S3						(0x1 << 14)


#define SYS_CTL2_ONOFF_PRESS				(0x1 << 15)
#define SYS_CTL2_ONOFF_SHORT_PRESS			(0x1 << 14)
#define SYS_CTL2_ONOFF_LONG_PRESS			(0x1 << 13)
#define SYS_CTL2_ONOFF_INT_EN				(0x1 << 12)
#define ONOFF_PRESS_TIME_0_5S_IS_SHORT		(0x0 << 10)
#define ONOFF_PRESS_TIME_1S_IS_SHORT		(0x1 << 10)
#define ONOFF_PRESS_TIME_2S_IS_SHORT		(0x2 << 10)
#define ONOFF_PRESS_TIME_4S_IS_SHORT		(0x3 << 10)
#define SYS_CTL2_ONOFF_PRESS_RESET_EN		(0x1 << 9)		
#define ONOFF_RESET_TIME_SEL_6S_N			(0x3 << 7)
#define ONOFF_RESET_TIME_SEL_8S				(0x1 << 7)
#define ONOFF_RESET_TIME_SEL_10S			(0x2 << 7)
#define ONOFF_RESET_TIME_SEL_12S			(0x3 << 7)
#define SYS_CTL2_S2_TIMER_EN				(0x1 << 6)
#define S2TIMER_6M_N						(0x7 << 3)
#define S2TIMER_16M							(0x1 << 3)
#define S2TIMER_31M							(0x2 << 3)
#define S2TIMER_61M							(0x3 << 3)
#define S2TIMER_91M							(0x4 << 3)
#define S2TIMER_121M						(0x5 << 3)
#define S2TIMER_151M						(0x6 << 3)
#define S2TIMER_181M						(0x7 << 3)
#define SYS_CTL2_ONOFF_PRESS_PD				(0x1 << 2)
#define SYS_CTL2_ONOFF_PRESS_INT_EN			(0x1 << 1)
#define SYS_CTL2_PMU_A_EN					(0x1 << 0)

#define PMIC_I2C_ADDR  		(0x65)

#define ATC2603C_INT_ONOFF			(0x1 << 0)			
#define ATC2603C_INT_ONOFF_PRESS_EN	(0x1 << 1)

#define ATC2603C_S1			(0x1)
#define ATC2603C_S2			(0x2)
#define ATC2603C_S3			(0x3)
#define ATC2603C_S4			(0x4)

struct atc2603C_pm_dev {
	struct device *dev;
	int irq;
	struct notifier_block pm_nb;
	uint active_wakeup_srcs;
};

static int i2c0_write_reg(u8 bus_addr, u8 reg, u16 data)
{
	u32 val = 0;
	// reset all stats
	act_writel(0xff, I2C0_STAT);
	
	// disable interrupt
	act_writel(0xc0, I2C0_CTL);
	
	// enable i2c without interrupt
	act_writel(0x80, I2C0_CTL);
	
	// write data count
	act_writel(2, I2C0_DATCNT);
	
	// write slave addr
	act_writel(bus_addr << 1, I2C0_TXDAT);
	
	// write register addr
	act_writel(reg, I2C0_TXDAT);
	
	// write data
	act_writel((data >> 8) & 0xff, I2C0_TXDAT);
	act_writel(data & 0xff, I2C0_TXDAT);
	
	// write fifo command
	act_writel(0x8d05, I2C0_CMD);
	// wait for the command to complete
	while (1)
	{
		val = act_readl(I2C0_FIFOSTAT);
		
		if (val & (0x1 << 1)) // nack
		{
		    // clear error bit
            act_writel((0x1 << 1), I2C0_FIFOSTAT);
		    
		    // reset fifo
		    act_writel(0x06, I2C0_FIFOCTL);
		    act_readl(I2C0_FIFOCTL);
		    
		    // disable adapter
	        act_writel(0, I2C0_CTL);
			return -EIO;
		}
		
		// execute complete
		if (val & (0x1 << 0)) {
			break;
		}
	}
	
	// disable adapter
	act_writel(0, I2C0_CTL);
	return 0;
}

static int i2c0_read_reg(u8 bus_addr, u8 reg, u16 * data)
{
    u32 val = 0;
    // reset all stats
	act_writel(0xff, I2C0_STAT);
	// disable interrupt
	act_writel(0xc0, I2C0_CTL);
	
	// enable i2c without interrupt
	act_writel(0x80, I2C0_CTL);
	
	// write data count
	act_writel(2, I2C0_DATCNT);
	
	// write slave addr
	act_writel(bus_addr << 1, I2C0_TXDAT);
	
	// write register addr
	act_writel(reg, I2C0_TXDAT);
	
	// write (slave addr | read_flag)
	act_writel((bus_addr << 1) | 0x1, I2C0_TXDAT);
	
	// write fifo command
	act_writel(0x8f35, I2C0_CMD);
	// wait command complete
	while (1)
	{
		val = act_readl(I2C0_FIFOSTAT);
		if (val & (0x1 << 1)) // nack
		{
		    // clear error bit
		    act_writel((0x1 << 1), I2C0_FIFOSTAT);
		    
		    // reset fifo
		    act_writel(0x06, I2C0_FIFOCTL);
		    act_readl(I2C0_FIFOCTL);
		    
		    // disable adapter
	        act_writel(0, I2C0_CTL);
			return -EIO;
		}
		
		// execute complete
		if (val & (0x1 << 0)) {
			break;
		}
	}
	
	// read data from rxdata
	*data = (act_readl(I2C0_RXDAT) & 0xff) << 8;
	*data |= act_readl(I2C0_RXDAT) & 0xff;
	
	// disable adapter
	act_writel(0, I2C0_CTL);
	return 0;
}


static inline int atc2603C_reg_read(u8 reg, u16 * data) {
    return i2c0_read_reg(PMIC_I2C_ADDR, reg, data);
}

static inline int atc2603C_reg_write(u8 reg, u16 data) {
    return i2c0_write_reg(PMIC_I2C_ADDR, reg, data);
}

static int atc2603C_reg_setbits(u8 reg, u16 mask, u16 val)
{
    u16 tmp;
	u16 orig;
	int ret;
	ret = atc2603C_reg_read(reg, &orig);
	if (ret) {
		return ret;
	}
	tmp = orig & ~mask;
	tmp |= val & mask;
	if (tmp != orig) {
	    ret = atc2603C_reg_write(reg, tmp);
		if(ret){
		}
	}
	
	return ret;
}

/*control the enable/desable of interrupts of atc2603C 
@EN should be true or EN to enable or false to Desable the interrups required at interrupts
@interrupts are the interrups required to be enable or disabled
	values: 
*/
static int atc2603C_interrupt_en(bool EN, unsigned int interrupts){
	int ret = 0;
	u16 set;
	if(ATC2603C_INT_ONOFF & interrupts){
		if(!EN){
			 set = 0;
		}else{ 
			set = SYS_CTL2_ONOFF_INT_EN;
		}
		ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2,
							SYS_CTL2_ONOFF_INT_EN,
							set);
		if(ret){
			pr_info("fail to enable/disable ONOFF interrupt");
			return ret;
		}
	}
	if(ATC2603C_INT_ONOFF_PRESS_EN & interrupts){
		if(!EN){
			 set = 0;
		}else{ 
			set = SYS_CTL2_ONOFF_PRESS_INT_EN;
		}
		ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2,
							SYS_CTL2_ONOFF_PRESS_INT_EN,
							set);
		if(ret){
			pr_info("fail to enable ONOFF_PRESS_INT_EN interrupt");
			return ret;
		}
	}
	return ret;
}


/* Prepare the system to shutdown,
i2c process is changed to direct access at this point
*as only direct acces is used at this point do nothing yet
*/
static int _atc2603C_pm_prepare_powerdown(void)
{
	/* switch to direct access,
	 * avoid using standard SPI/I2C interface that maybe halt in pwrdwn process */
	//atc260x_set_reg_direct_access(_get_curr_atc260x_pm_obj()->atc260x, true);
	pr_info("[PM]: prepare powerdown");
	return 0;
}

/*Do shutdown
 */
static int _atc2603C_pm_powerdown(uint deep_pwrdn)
{
	//struct atc2603C_pm_dev *atc2603C_pm;
	uint reg_mask, reg_val;
	int ret;
	u16 temp;

	ret = atc2603C_reg_read(ATC2603C_PMU_SYS_CTL0, &temp);

	//befone clean EN_S1 is necessary Set EN_S2 and EN_S3
	reg_mask = SYS_CTL3_EN_S2 | SYS_CTL3_EN_S3;
	reg_val = deep_pwrdn ? 0 : SYS_CTL3_EN_S3;
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL3, reg_mask, reg_val);

	if (ret) {
		pr_err("%s() %u err\n", __func__, __LINE__);
		return ret;
	}

	pr_alert("%s() : powerdown.......................\n", __func__);

	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL1, SYS_CTL1_EN_S1, 0);
	while (ret == 0) { /* wait for powerdown if success. */
		mdelay(200); /* DO NOT use msleep() here! non-schedulable contex! */
		pr_warn("%s() wait for powerdown...\n", __func__);
	}
	return ret;
}

/*Do reboot now
 */
static int _atc2603C_pm_do_reboot(void)
{
	int ret;
	/* clear all wakeup source except on/off and p_reset */
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL0, SYS_CTL0_WK_ALL, 
								SYS_CTL0_RESET_WK_EN | SYS_CTL0_ONOFF_SHORT_WK_EN | 
								SYS_CTL0_ONOFF_LONG_WK_EN
	);						  
	if (ret) {
		pr_err("%s() clear setup wakeup err, ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* set S3, clear S2 */
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL3, 
							  (SYS_CTL3_EN_S2 | SYS_CTL3_EN_S3),
							  (SYS_CTL3_EN_S3)
	);
	if (ret) {
		pr_err("%s() %u err\n", __func__, __LINE__);
		return ret;
	}

	pr_alert("%s() : reboot.......................\n", __func__);
	//do reset
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL0,
							  (SYS_CTL0_RESTART_EN),
							  (SYS_CTL0_RESTART_EN)
	);
	while (ret == 0) { /* wait for reboot if success. */
		mdelay(200); /* DO NOT use msleep() here! non-schedulable contex! */
		pr_warn("%s() wait for reboot...\n", __func__);
	}
	return ret;
}

/*provides any reboot wanted using tgt, but only simple reboot is done
	@tgt: target reboot.
*/
static int _atc2603C_pm_reboot(uint tgt)
{
	int ret;
	ret = _atc2603C_pm_do_reboot();
	return ret;
}

/*not implemented yet */
static int _atc2603C_pm_finish_wakeup(void){
	//pr_info("suspend_finish");
	return 0;
}

/*not implemented yet */
static int _atc2603C_pm_wakeup(void){
	//pr_info("suspend_wake");
	return 0;
}

/*set wakeup sources
	@wakeup_mask :wakeup sources to be clean
	@wakeup_src: wakeup sources to be set
	#values should be SYS_CTL0_(name of bit to set), such as SYS_CTL0_ONOFF_LONG_WK_EN
	to clean or set all wakeup sources at once use SYS_CTL0_WK_ALL
*/
static int _atc2603C_pm_set_wakeup_src(uint wakeup_mask, uint wakeup_src){
	return atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL0,
						(wakeup_mask),
						(wakeup_src)
	); 
}

/*not implemented yet */
static int _atc2603C_pm_enter_suspend(void){
	//pr_info("set_wakeup_src");
	return 0;
}

static struct owl_pmic_pm_ops atc2603C_pm_pmic_ops = {
	.set_wakeup_src = _atc2603C_pm_set_wakeup_src,
	//.get_wakeup_src = _atc260x_pm_get_wakeup_src,
	//.get_wakeup_flag = _atc260x_pm_get_wakeup_flag,

	.shutdown_prepare = _atc2603C_pm_prepare_powerdown,
	.powerdown = _atc2603C_pm_powerdown,
	.reboot = _atc2603C_pm_reboot,

	//.suspend_prepare = _atc260x_pm_prepare_suspend,
	.suspend_enter = _atc2603C_pm_enter_suspend,
	.suspend_wake = _atc2603C_pm_wakeup,
	.suspend_finish = _atc2603C_pm_finish_wakeup,

	//.get_bus_info = _atc260x_pm_get_bus_info,
};


static int atc2603C_pm_notifier_callback(struct notifier_block *nb, unsigned long event, void *dummy)
{
	//struct atc2603C_pm_dev *atc2603C_pm = _get_curr_atc2603C_pm_obj();
	switch (event) {
	case PM_POST_HIBERNATION:    /* Hibernation finished */
		//_clear_status(atc2603C_pm);
		break;
	}
	printk("Notifier ok!");
	return NOTIFY_OK;
}


/*Should return the state of atc2603C
	return ATC2603C_S1 if state is S1, ATC2603C_S2 if state is S2
	and so on...
 */
static int atc2603C_get_state(void){
	u16 reg; 
	atc2603C_reg_read(ATC2603C_PMU_SYS_CTL1, &reg);
	if(reg & SYS_CTL1_EN_S1){
		return ATC2603C_S1;
	}
	atc2603C_reg_read(ATC2603C_PMU_SYS_CTL3, &reg);
	if(reg & SYS_CTL3_EN_S2){
		return ATC2603C_S2;
	}
	if(reg & SYS_CTL3_EN_S3){
		return ATC2603C_S3;
	}
	return ATC2603C_S4;
}

/*Interrupt handle
 */
static irqreturn_t atc2603C_onoff_irq_handle(int irq, void *dev_id){
	u16 reg;
	unsigned int pmic_state = atc2603C_get_state();
	//get the list of interrupt pending 
	atc2603C_reg_read(ATC2603C_INTS_PD, &reg);
	if(reg & (0x1 << 6)){//there is a ONOFF interrupt pending
		atc2603C_reg_read(ATC2603C_PMU_SYS_CTL2, &reg);
		//short press happen
		if(SYS_CTL2_ONOFF_PRESS_PD & reg){
			if(SYS_CTL2_ONOFF_SHORT_PRESS & reg){
				//pmic is enable and should shutdown
				pr_info("Short press happens!");
				if(pmic_state == ATC2603C_S1){
					//do nothing
					pr_info("do nothing to avoid acidental shutdown\n");
				}else//pmic is in sleep or shutdown state
				{
					//do_wakeup 
					_atc2603C_pm_reboot(0);
					pr_info("fail on reboot should never happen!");
					return IRQ_HANDLED;
				}
				//clean pending
				atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2, 
									SYS_CTL2_ONOFF_SHORT_PRESS, SYS_CTL2_ONOFF_SHORT_PRESS);
			}else{
				if(SYS_CTL2_ONOFF_LONG_PRESS & reg){
					//pmic is enable and should shutdown
					pr_info("Long press happens!");
					if(pmic_state == ATC2603C_S1){
						//do shutdown
						_atc2603C_pm_set_wakeup_src(SYS_CTL0_WK_ALL, SYS_CTL0_ONOFF_LONG_WK_EN | SYS_CTL0_ONOFF_SHORT_WK_EN);
						pr_info("Shutdown Now!");
						_atc2603C_pm_powerdown(0);
						return IRQ_HANDLED;
					}else//pmic is in sleep or shutdown state
					{
						//do_wakeup
						pr_info("reboot to wakeup");
						_atc2603C_pm_reboot(0);
						pr_info("fail on reboot should never happen!");
						return IRQ_HANDLED;
					}
					//clean pending
					atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2, 
										SYS_CTL2_ONOFF_LONG_PRESS, SYS_CTL2_ONOFF_LONG_PRESS);
					//pr_info("unespected ONOFF interrupt PMU_SYS_CTL2:%X", reg);
				}
			}
			//clean pending
			atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2, 
									SYS_CTL2_ONOFF_PRESS_PD, 0);
			//atc2603C_reg_setbits(ATC2603C_INTS_PD, 
			//						(0x1 << 6), 0);
		}else{
			pr_info("no pending in ONOFF");
		}						
	}
	return IRQ_HANDLED;
}

/*config ONOFF key press time
 @time should be  ONOFF_PRESS_TIME_0_5S_IS_SHORT or 
 				  ONOFF_PRESS_TIME_1S_IS_SHORT or
			      ONOFF_PRESS_TIME_2S_IS_SHORT or 
				  ONOFF_PRESS_TIME_4S_IS_SHORT
*/
static int atc2603C_pm_config_onoff_time(unsigned const int time){
	return atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2, 
							   (MASK_2_BITS << 10),
							   (time) 
	);
}

/*Do basics configurations to onoff to work
 */
static int atc2603C_pm_config_onoff(void){
	int ret = 0;
	//enable the interrupt mask of ONOFF
	ret = atc2603C_reg_setbits(ATC2603C_INTS_MSK,
							(0x1 << 6),
							(0x1 << 6));
	if(ret){
			pr_info("fail to enable/disable interrupt MSK ONOFF");
			return ret;
	} 
	//enable interrupt of ONOFF
	ret = atc2603C_interrupt_en(true, ATC2603C_INT_ONOFF);
	if(ret){
		pr_info("fail to configure interrupt ONOFF");
		return ret;
	}
	//config timing of the press ONOFF, until 2second is short after is long
	ret = atc2603C_pm_config_onoff_time(ONOFF_PRESS_TIME_2S_IS_SHORT);
	if(ret){
		pr_info("fail to configure ONOFF times of press");
	}

	//enable the ONOFF long press Reset and and time of 6s to reset
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL2,
							  (SYS_CTL2_ONOFF_PRESS_RESET_EN |
							  ONOFF_RESET_TIME_SEL_6S_N),
							  (SYS_CTL2_ONOFF_PRESS_RESET_EN)
	);
	if(ret){
		pr_info("fail to configure long press reset");
	}
	return ret;
}

static ssize_t adc_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buffer)
{
    int ret = -EINVAL;
    ktime_t timestamp;
    u16 value;
    
    if (strcmp(attr->attr.name, "adc0") == 0) {
        ret = atc2603C_reg_read(ATC2603C_PMU_AUXADC0, &value);
    }
    else if (strcmp(attr->attr.name, "adc1") == 0) {
        ret = atc2603C_reg_read(ATC2603C_PMU_AUXADC1, &value);
    }
    else if (strcmp(attr->attr.name, "adc2") == 0) {
        ret = atc2603C_reg_read(ATC2603C_PMU_AUXADC2, &value);
    }
    
    if (!ret) {
        ret = value;
    }
    
    timestamp = ktime_get();
    
    return sprintf(buffer, "%llu %d\n", ktime_to_us(timestamp), value);
}

static ssize_t adc_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buffer, size_t count)
{
    return count;
}

static struct kobj_attribute adc0_attr = 
    __ATTR(adc0, 0664, adc_show, adc_store);
    
static struct kobj_attribute adc1_attr =
    __ATTR(adc1, 0664, adc_show, adc_store);
    
static struct kobj_attribute adc2_attr =
    __ATTR(adc2, 0664, adc_show, adc_store);

static struct attribute *adc_attrs[] = {
    &adc0_attr.attr,
    &adc1_attr.attr,
    &adc2_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = adc_attrs,
};

// PMIC pin 23 AUXIN0 corresponds to ADC input on base board debug connector

static struct kobject *adc_kobj;

static int init_auxadc_sysfs_group(void)
{
    int ret;
    
    // start all adcs
    atc2603C_reg_write(ATC2603C_PMU_AUXADC_CTL0, 0xffffU);
    
    // create directory auxadc at /sys/kernel/
	adc_kobj = kobject_create_and_add("auxadc", kernel_kobj);
	
	if (!adc_kobj) {
		return -ENOMEM;
	}
	
	ret = sysfs_create_group(adc_kobj, &attr_group);
	
	if(ret) {
	    kobject_put(adc_kobj);
	}
	
	return ret;
}

static void exit_auxadc_sysfs_group(void)
{
	kobject_put(adc_kobj);
}

static int atc2603C_pm_probe(struct platform_device *pdev)
{
	struct atc2603C_pm_dev *atc2603C_pm;
	struct device *dev = &pdev->dev;
	int ret, irq;

	dev_info(&pdev->dev, "Probing...\n");

	atc2603C_pm = devm_kzalloc(&pdev->dev, sizeof(struct atc2603C_pm_dev),GFP_KERNEL);
	if (!atc2603C_pm) {
		dev_err(&pdev->dev, "failed to alloc memory to atc2603C_pm dev!");
		return -ENOMEM;
	}
	atc2603C_pm->dev = &pdev->dev;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
	{
		pr_err("Could not get irq\n");
		ret = -ENODEV;
		goto free;
	}

	platform_set_drvdata(pdev, atc2603C_pm);

	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL1, (MASK_2_BITS<<3), SYS_CTL1_LB_S4_3_1V); 
	if (ret) {
		dev_err(dev, "error to configure LB_S4 of SYS_CTL1 Register!");
	}
	
	//setting the inner wakeup sources, at this point the mask is full
	atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL0, SYS_CTL0_WK_ALL, 
						SYS_CTL0_HDSW_WK_EN | SYS_CTL0_ONOFF_LONG_WK_EN
	);

	//Alarm will be disabled

	//delay some time from s2/s3 to s1 to avoid ddr init fail in bootloader
	ret = atc2603C_reg_setbits(ATC2603C_PMU_SYS_CTL0, SYS_CTL3_S2S3TOS1TIMER_EN, SYS_CTL3_S2S3TOS1TIMER_EN);
	if (ret) {
		dev_err(atc2603C_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}

	//setup wall vbus wakeup functions

	/* get get_wakeup_flag */

	/* register PM notify */
	atc2603C_pm->pm_nb.notifier_call = atc2603C_pm_notifier_callback;
	ret = register_pm_notifier(&(atc2603C_pm->pm_nb));
	if (ret) {
		dev_err(atc2603C_pm->dev, "failed to register pm_notifier, ret=%d\n", ret);
		goto label_err_lv3;
	}

	atc2603C_pm_config_onoff();

	owl_pmic_set_pm_ops(&atc2603C_pm_pmic_ops);

	atc2603C_pm->irq = irq;

	ret = devm_request_irq(dev, atc2603C_pm->irq, atc2603C_onoff_irq_handle,
				IRQF_TRIGGER_HIGH,
				"pmic-onoff", atc2603C_pm);
	if(ret != 0){
		dev_err(dev, "failed to request ONOFF device irq(%d)\n", irq);
		goto label_err_lv3;
	}
	
	ret = init_auxadc_sysfs_group();
	
	if (ret) {
	    dev_err(atc2603C_pm->dev, "failed to init adc, ret=%d\n", ret);
	    goto label_err_lv3;
	}
	
	pr_info("%s: Probe over", __func__);
	return 0;

	label_err_lv3:
	unregister_pm_notifier(&(atc2603C_pm->pm_nb));
	owl_pmic_set_pm_ops(NULL);
	platform_set_drvdata(pdev, NULL);
	free_irq(atc2603C_pm->irq, pdev);
	free:
		devm_kfree(dev, atc2603C_pm);

	return ret;
}

static int atc2603C_pm_remove(struct platform_device *pdev)
{
	struct atc2603C_pm_dev *atc2603C_pm = platform_get_drvdata(pdev);
	exit_auxadc_sysfs_group();

	unregister_pm_notifier(&(atc2603C_pm->pm_nb));
	owl_pmic_set_pm_ops(NULL);
	platform_set_drvdata(pdev, NULL);
	return 0;
}


static const struct of_device_id atc2603C_pm_match[] = {
	{ .compatible = "caninos,atc2603c-pm", },
	{},
};
MODULE_DEVICE_TABLE(of, atc2603C_pm_match);

static struct platform_driver atc2603C_pm_driver = {
	.probe = atc2603C_pm_probe,
	.remove = atc2603C_pm_remove,
	.driver = {
		.name   = "atc2603C-pm",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(atc2603C_pm_match),
	},
};

static int __init atc2603C_pm_init(void)
{
	int ret;
	ret = platform_driver_register(&atc2603C_pm_driver);
	if (ret != 0)
		pr_err("Failed to register ATC2603C PM driver: %d\n", ret);
	return 0;
}
subsys_initcall(atc2603C_pm_init);
//module_init(atc2603C_pm_init); 

static void __exit atc2603C_pm_exit(void)
{
	platform_driver_unregister(&atc2603C_pm_driver);
}
module_exit(atc2603C_pm_exit);

/* Module information */
MODULE_AUTHOR("LSITEC-CITI");
MODULE_DESCRIPTION("ATC26003C PM driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc2603C-pm");
