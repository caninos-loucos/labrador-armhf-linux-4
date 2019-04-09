#define DSS_SUBSYS_NAME "CORE"
#include <mach/bootdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <linux/suspend.h>
#include <linux/sysfs.h>

#include <video/owldss.h>

#include <mach/powergate.h>

#include "dss.h"
#include "dss_features.h"
#include "de.h"

struct {
	struct platform_device *pdev;
} core;

/* Board specific data */
struct owl_dss_board_info {
	int num_devices;
	struct owl_dss_device **devices;
};

static int dss_initialize_debugfs(void)
{
	return 0;
}
static void dss_uninitialize_debugfs(void)
{
	return;
}

static void dss_bus_release(struct device *dev)
{
	DSSDBG("bus_release\n");
}

/* 
 * dss bus device
 */
static struct device dss_bus = {
	.release = dss_bus_release,
};

/* 
 * dss bus
 */
static int dss_bus_match(struct device *dev, struct device_driver *driver)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);

	DSSDBG("bus_match. dev %s/%s, drv %s\n",
			dev_name(dev), dssdev->driver_name, driver->name);

	return strcmp(dssdev->driver_name, driver->name) == 0;
}

static ssize_t dss_dev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);
	return snprintf(buf, PAGE_SIZE, "%s\n",
			dssdev->name ?
			dssdev->name : "");
}

static DEVICE_ATTR_RO(dss_dev);

static struct attribute *dss_dev_attrs[] = {
	&dev_attr_dss_dev.attr,
	NULL
};

static struct attribute_group dss_dev_attr_group = {
	.attrs	= dss_dev_attrs,
};

static const struct attribute_group *dss_dev_attr_groups[] = {
	&dss_dev_attr_group,
	NULL
};

static ssize_t dss_drv_show(struct device_driver *drv, char *buf)
{
	struct owl_dss_driver *dssdrv = to_dss_driver(drv);
	return snprintf(buf, PAGE_SIZE, "%s\n",
			dssdrv->driver.name ?
			dssdrv->driver.name : "");
}

static DRIVER_ATTR_RO(dss_drv);

static struct attribute *dss_drv_attrs[] = {
	&driver_attr_dss_drv.attr,
	NULL
};

static struct attribute_group dss_drv_attr_group = {
	.attrs	= dss_drv_attrs,
};

static const struct attribute_group *dss_drv_attr_groups[] = {
	&dss_drv_attr_group,
	NULL
};

static struct bus_type dss_bus_type = {
	.name = "caninos-dss",
	.match = dss_bus_match,
	.dev_groups = dss_dev_attr_groups,
	.drv_groups = dss_drv_attr_groups,
};


/* register bus and its device */
static int owl_dss_bus_register(void)
{
	int r;

	r = bus_register(&dss_bus_type);
	if (r) {
		DSSERR("bus register failed\n");
		return r;
	}

	dev_set_name(&dss_bus, "caninos-dss");
	r = device_register(&dss_bus);
	if (r) {
		DSSERR("bus driver register failed\n");
		bus_unregister(&dss_bus_type);
		return r;
	}

	return 0;
}

/* unregister bus and its device */
static void owl_dss_bus_unregister(void)
{
	device_unregister(&dss_bus);

	bus_unregister(&dss_bus_type);
}

/*
 * bus operations to others
 */

struct bus_type *dss_get_bus(void)
{
	return &dss_bus_type;
}

static int dss_driver_probe(struct device *dev)
{
	int r;
	struct owl_dss_driver *dssdrv = to_dss_driver(dev->driver);
	struct owl_dss_device *dssdev = to_dss_device(dev);

	DSSDBG("driver_probe: dev %s/%s, drv %s\n",
				dev_name(dev), dssdev->driver_name,
				dssdrv->driver.name);


	
	dss_init_device(core.pdev, dssdev);
	
	r = dssdrv->probe(dssdev);

	if (r) {
		DSSERR("driver probe failed: %d\n", r);
		dss_uninit_device(core.pdev, dssdev);
		return r;
	}

	DSSDBG("probe done for device %s\n", dev_name(dev));

	dssdev->driver = dssdrv;

	return 0;
}

static int dss_driver_remove(struct device *dev)
{
	struct owl_dss_driver *dssdrv = to_dss_driver(dev->driver);
	struct owl_dss_device *dssdev = to_dss_device(dev);

	DSSDBG("driver_remove: dev %s/%s\n", dev_name(dev),
			dssdev->driver_name);

	dssdrv->remove(dssdev);

	dss_uninit_device(core.pdev, dssdev);

	dssdev->driver = NULL;

	return 0;
}

/* register dss driver to dss bus */
int owl_dss_register_driver(struct owl_dss_driver *dssdriver)
{
	dssdriver->driver.bus = &dss_bus_type;
	dssdriver->driver.probe = dss_driver_probe;
	dssdriver->driver.remove = dss_driver_remove;

	if (dssdriver->get_resolution == NULL)
	{
		dssdriver->get_resolution = owl_default_get_resolution;
	}
		
	if (dssdriver->get_recommended_bpp == NULL)
	{
		dssdriver->get_recommended_bpp = owl_default_get_recommended_bpp;
	}

	return driver_register(&dssdriver->driver);
}
EXPORT_SYMBOL(owl_dss_register_driver);

/* unregister dss driver from dss bus */
void owl_dss_unregister_driver(struct owl_dss_driver *dssdriver)
{
	driver_unregister(&dssdriver->driver);
}
EXPORT_SYMBOL(owl_dss_unregister_driver);

/* DEVICE */
static void reset_device(struct device *dev, int check)
{
	u8 *dev_p = (u8 *)dev;
	u8 *dev_end = dev_p + sizeof(*dev);
	void *saved_pdata;

	saved_pdata = dev->platform_data;
	if (check) {
		/*
		 * Check if there is any other setting than platform_data
		 * in struct device; warn that these will be reset by our
		 * init.
		 */
		dev->platform_data = NULL;
		while (dev_p < dev_end) {
			if (*dev_p) {
				WARN("%s: struct device fields will be "
						"discarded\n",
				     __func__);
				break;
			}
			dev_p++;
		}
	}
	memset(dev, 0, sizeof(*dev));
	dev->platform_data = saved_pdata;
}

static void owl_dss_dev_release(struct device *dev)
{
	reset_device(dev, 0);
}

/* register dss deivce to dss bus */
int owl_dss_register_device(struct owl_dss_device *dssdev)
{
	static int dev_num;

	WARN_ON(!dssdev->driver_name);

	reset_device(&dssdev->dev, 1);
	dssdev->dev.bus = &dss_bus_type;
	dssdev->dev.parent = &dss_bus;
	dssdev->dev.release = owl_dss_dev_release;
	dev_set_name(&dssdev->dev, "display%d", dev_num++);
	return device_register(&dssdev->dev);
}

/* unregister dss deivce from dss bus */
void owl_dss_unregister_device(struct owl_dss_device *dssdev)
{
	device_unregister(&dssdev->dev);
}

/*=================================================================
 *       platform device & driver for display subsystem
 *===============================================================*/
static int __init owl_dss_probe(struct platform_device *pdev)
{
	struct owl_dss_board_info *pdata = pdev->dev.platform_data;
	int r;
	int i;

	core.pdev = pdev;

	DSSDBG("dss_features_init called \n");
	dss_features_init();

	DSSDBG("dss_init_overlay_managers called \n");
	dss_init_overlay_managers(pdev);

	DSSDBG("dss_init_overlays called \n");
	dss_init_overlays(pdev);
	
	DSSDBG("dss_initialize_debugfs called \n");
	r = dss_initialize_debugfs();
	if (r)
		goto err_debugfs;

	/*
	 * enable DS power gate if need,
	 * because powergate will maintain a count for every power,
	 * so we must check if DS power is enabled in U-BOOT,
	 * it will ensure DS power on count 1 after booting
	 *
	 * should not care about the return value,
	 * powergate is self-adation for different platform
	 */
	if (owl_powergate_is_powered(OWL_POWERGATE_DS) <= 0) {
		DSSDBG("DS powergate on called\n");
		owl_powergate_power_on(OWL_POWERGATE_DS);
	}

	DSSDBG("de_init called\n");
	if (r) {
		goto err_de;
	}

	DSSDBG("owl_dss_register_device called \n");
	for (i = 0; i < pdata->num_devices; ++i) {
		struct owl_dss_device *dssdev = pdata->devices[i];

		r = owl_dss_register_device(dssdev);
		if (r) {
			DSSERR("device %d %s register failed %d\n", i,
				dssdev->name ?: "unnamed", r);

			while (--i >= 0)
				owl_dss_unregister_device(pdata->devices[i]);

			goto err_register;
		}
	}

	return 0;

err_register:
	owl_de_exit();

err_de:
	owl_powergate_power_off(OWL_POWERGATE_DS);

	dss_uninitialize_debugfs();

err_debugfs:
	return r;
}

static int owl_dss_remove(struct platform_device *pdev)
{
	struct owl_dss_board_info *pdata = pdev->dev.platform_data;
	int i;

	for (i = 0; i < pdata->num_devices; ++i)
		owl_dss_unregister_device(pdata->devices[i]);

	owl_de_exit();

	owl_powergate_power_off(OWL_POWERGATE_DS);

	dss_uninitialize_debugfs();
	
	dss_uninit_overlays(pdev);
	dss_uninit_overlay_managers(pdev);

	return 0;
}

atomic_t devices_suspended_from_early_suspend = ATOMIC_INIT(false);

#ifdef CONFIG_PM
static int owl_dss_suspend(struct platform_device *pdev, pm_message_t state)
{
	DSSINFO("suspending dss \n");
	
	if(!owl_dss_is_devices_suspended())
	{
		DSSINFO("suspending displays \n");
		dss_suspend_all_devices();
		DSSINFO("suspending displays end\n");
	}
	
	owl_de_suspend();
	
	DSSINFO("suspending dss end\n");
	
	return 0;
}

static int owl_dss_resume(struct platform_device *pdev)
{
	DSSINFO("resuming dss \n");

	/*
	 * should not care about the return value,
	 * powergate is self-adation for different platform
	 */

	owl_de_resume();

	if(owl_dss_is_devices_suspended() && !atomic_read(&devices_suspended_from_early_suspend))
	{
		DSSINFO("resuming displays \n");
		dss_resume_all_devices();
		DSSINFO("resuming displays end \n");
	}
	DSSINFO("resuming dss  end\n");

	return 0;
}
#else
#define owl_dss_suspend    NULL
#define owl_dss_resume     NULL
#endif

static void owl_dss_shutdown(struct platform_device *pdev)
{
	dss_disable_all_devices();
}

static struct owl_dss_device asoc_lcd_device = {
	.name			= "lcd",
	.driver_name	= "generic_lcdc_panel",
	.type			= OWL_DISPLAY_TYPE_LCD,
	.data_lines	= 24,
};
static struct owl_dss_device asoc_edp_device = {
	.name			= "edp",
	.driver_name	= "generic_edp_panel",
	.type			= OWL_DISPLAY_TYPE_EDP,
	.data_lines	= 24,
};

static struct owl_dss_device asoc_dsi_device = {
	.name			= "dsi",
	.driver_name	= "generic_dsi_panel",
	.type			= OWL_DISPLAY_TYPE_DSI,
	.data_lines	= 24,
};
static struct owl_dss_device asoc_hdmi_device = {
	.name			= "hdmi",
	.driver_name	= "hdmi_panel",
	.type			= OWL_DISPLAY_TYPE_HDMI,
	.data_lines	= 16,
};

static struct owl_dss_device asoc_cvbs_device = {
	.name			= "cvbs",
	.driver_name	= "cvbs_panel",
	.type			= OWL_DISPLAY_TYPE_CVBS,
	.data_lines	= 16,
};

static struct owl_dss_device *owl_dss_devices[] = {
	&asoc_lcd_device,
	&asoc_dsi_device,
	&asoc_edp_device,
	&asoc_hdmi_device,
	&asoc_cvbs_device,
};

static struct owl_dss_board_info owl_dss_data = {
	.num_devices	= ARRAY_SIZE(owl_dss_devices),
	.devices	= owl_dss_devices,
};

static struct platform_device owl_dss_device = {
	.name = "caninos-dss",
	.id   = -1,
	.dev  = {
		.platform_data = &owl_dss_data,
	},
};

static struct platform_driver owl_dss_driver = {
	.remove = owl_dss_remove,
	.shutdown = owl_dss_shutdown,
	.suspend = owl_dss_suspend,
	.resume	= owl_dss_resume,
	
	.driver = {
		.name   = "caninos-dss",
		.owner  = THIS_MODULE,
	},
};

static int __init owl_dss_init(void)
{
	int r;

	r = owl_dss_bus_register();
	
	if (r) {
		return r;
	}
	
	r = platform_device_register(&owl_dss_device);
	
	if (r) 
	{
		owl_dss_bus_unregister();
		return r;
	}	

	r = platform_driver_probe(&owl_dss_driver, owl_dss_probe);
	
	if (r) 
	{
		owl_dss_bus_unregister();
		platform_device_unregister(&owl_dss_device);
		return r;
	}	

	return 0;
}

static void owl_dss_exit(void)
{
	platform_driver_unregister(&owl_dss_driver);
	
	platform_device_unregister(&owl_dss_device);
	
	owl_de_exit();
	
	owl_dss_bus_unregister();
}

module_init(owl_dss_init);
module_exit(owl_dss_exit);

