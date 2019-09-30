/*
 * 
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <asm/system_misc.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <mach/power.h>
#include <mach/powergate.h>
#include <linux/of_gpio.h>
#include <atc260x/atc260x.h>
#include <linux/highmem.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>


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

//all wakeup enable mask
#define SYS_CTL0_WK_ALL						(0xFBE0)

int owl_powergate_suspend(void);
int owl_powergate_resume(void);

static int _pmic_warn_null_cb(void)
{
	pr_err("[PM] owl_pmic_pm_ops not registered!\n");
	return -ENODEV;
}
static int _pmic_warn_null_cb_1ui(uint) __attribute__((alias("_pmic_warn_null_cb")));
static int _pmic_warn_null_cb_2ui(uint, uint) __attribute__((alias("_pmic_warn_null_cb")));
static int _pmic_warn_null_cb_3uip(uint*, uint*, uint*) __attribute__((alias("_pmic_warn_null_cb")));
static struct owl_pmic_pm_ops s_pmic_fallback_pm_ops = {
	.set_wakeup_src    = _pmic_warn_null_cb_2ui,
	.get_wakeup_src    = _pmic_warn_null_cb,
	.get_wakeup_flag   = _pmic_warn_null_cb,
	.shutdown_prepare  = _pmic_warn_null_cb,
	.powerdown         = _pmic_warn_null_cb_1ui,
	.reboot            = _pmic_warn_null_cb_1ui,
	.suspend_prepare   = _pmic_warn_null_cb,
	.suspend_enter     = _pmic_warn_null_cb,
	.suspend_wake      = _pmic_warn_null_cb,
	.suspend_finish    = _pmic_warn_null_cb,
	.get_bus_info      = _pmic_warn_null_cb_3uip,
};
static struct owl_pmic_pm_ops *s_pmic_pm_ops = &s_pmic_fallback_pm_ops;

/*void system_powerdown_withgpio(void)
{
}
EXPORT_SYMBOL_GPL(system_powerdown_withgpio);
*/

/*bool labrador_pm_is_battery_connected(void)
{
	int ret;
	struct power_supply *psy;
	union power_supply_propval val;

	// Check if battery is known 
	psy = power_supply_get_by_name("battery");
	if (psy) {
		ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &val);
		if (!ret && val.intval) {
			pr_info("find battery connect\n");
			return true;
		}
	}
	
	pr_info("can not find battery connect\n");
	return false;
}*/

static void labrador_pm_halt(void)
{
	int deep_pwrdn, wakeup_src;

	pr_info("[PM] %s %d:\n", __func__, __LINE__);

	s_pmic_pm_ops->shutdown_prepare();

	// default sleep mode and wakeup source 
	// DO NOT add HARD_SWITCH source, we are here just because no batt switch. 
	wakeup_src = SYS_CTL0_RESET_WK_EN | SYS_CTL0_ONOFF_LONG_WK_EN;
	
	//if(owl_pm_is_battery_connected())
	//	wakeup_src |= OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_VBUS_IN;

	// if wall/usb is connect, cannot enter S4, only can enter S3 
	deep_pwrdn = 1;
	//if (!power_supply_is_system_supplied())
	//	deep_pwrdn = 1;

	// Power off system 
	pr_info("Powering off (wakesrc: 0x%x, deep_pwrdn:%d)\n",
		wakeup_src, deep_pwrdn);

	//disable all wakeup sources and enable only Long or short to wakeup
	s_pmic_pm_ops->set_wakeup_src(SYS_CTL0_WK_ALL, SYS_CTL0_ONOFF_LONG_WK_EN | SYS_CTL0_ONOFF_SHORT_WK_EN);
	//s_pmic_pm_ops->set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ALL, wakeup_src);
	s_pmic_pm_ops->powerdown(!deep_pwrdn);

	// never return to here
	pr_err("[PM] %s() failed\n", __func__);
}
EXPORT_SYMBOL_GPL(labrador_pm_halt);


static void labrador_pm_restart(enum reboot_mode mode, const char *cmd)
{
	pr_info("[PM] %s() cmd: %s\n", __func__, cmd ? cmd : "<null>");

	s_pmic_pm_ops->shutdown_prepare();

	/*if (cmd) {
		if (!strcmp(cmd, "recovery")) {
			pr_info("cmd:%s----restart------\n", cmd);
			s_pmic_pm_ops->reboot(REBOOT_TGT_RECOVERY);
		} 			
	}*/
	s_pmic_pm_ops->reboot(0);

	pr_err("[PM] %s() failed\n", __func__);
	return;
}

/*
 *      pmic_suspend_set_ops - Set the global suspend method table.
 *      @ops:   Pointer to ops structure.
 */
void owl_pmic_set_pm_ops(struct owl_pmic_pm_ops *ops)
{
	pr_info("labrador-pm: set pmic suspend ops 0x%lx\n", (ulong)ops);
	if(ops == NULL || IS_ERR(ops)) {
		s_pmic_pm_ops = &s_pmic_fallback_pm_ops;
		pr_err("pmic_set_pm_ops fail");
	} else {
		s_pmic_pm_ops = ops;
		pr_err("pmic_set_pm_ops success");
	}
}
EXPORT_SYMBOL_GPL(owl_pmic_set_pm_ops);

static int labrador_pm_prepare_late(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	return s_pmic_pm_ops->suspend_prepare();
}

static int labrador_pm_enter(suspend_state_t state)
{
	int ret;

	//save pwm status

	//save core clock

	ret = s_pmic_pm_ops->suspend_enter();
	if(ret) {
		pr_err("[PM] %s() PMIC enter suspend failed, ret=%d", __func__, ret);
		return ret;
	}

	//set wakeup sources

	owl_powergate_suspend();

	//suspend cpu

	//restore datas of clock pwm and so on
	owl_powergate_resume();

	// DO NOT call s_pmic_pm_ops here, it's not ready at this moment! 

	return 0;
}

static void labrador_pm_wake(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	s_pmic_pm_ops->suspend_wake();
	//s_pmic_pm_ops->set_wakeup_src(0,0);
}

static void labrador_pm_finish(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	s_pmic_pm_ops->suspend_finish();
}


static const struct platform_suspend_ops labrador_pm_ops = {
	.enter = labrador_pm_enter,
	.prepare_late = labrador_pm_prepare_late,
	.wake = labrador_pm_wake,
	.finish = labrador_pm_finish,
	.valid = suspend_valid_only_mem,
};

static int __init labrador_pm_probe(struct platform_device * pdev){

	pr_info("labrador-pm: Probe begin");
	suspend_set_ops(&labrador_pm_ops);
	arm_pm_restart = labrador_pm_restart;
	pm_power_off = labrador_pm_halt;
	pr_info("pm_power_off was successed pointered!!");
	return 0;
} 

static void __exit labrador_pm_remove(void)
{
	//platform_driver_unregister(pdev);
	return;
}

static const struct of_device_id labrador_pm_match[] = {
	{ .compatible = "caninos,labrador-pm", },
	{},
};
MODULE_DEVICE_TABLE(of, labrador_pm_match);

static struct platform_driver __refdata labrador_pm_driver = {
	.driver     = {
		.name   = "labrador-pm",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(labrador_pm_match),
	},
	.probe = labrador_pm_probe,
	//.remove = labrador_pm_remove
};

int __init labrador_pm_init(void)
{
	printk("labrador-pm: %s() %d\n", __func__, __LINE__);
	return platform_driver_register(&labrador_pm_driver);
}
late_initcall(labrador_pm_init);


