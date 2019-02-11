
 
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <video/owldss.h>
#include <video/owlfb.h>
#include <video/owldisplay.h>

#include "owlfb.h"





static ssize_t show_dc_debug(struct device *dev,struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t store_dc_debug(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	int dc_debug_flags;
	int r;


	r = strtobool(buf, (bool *)&dc_debug_flags);
		
	if (r)
		return r;		
	
	r = count;

	return r;
}

static struct device_attribute owlfb_dc_attrs[] = {
	__ATTR(dc_debug, S_IRUGO | S_IWUSR, show_dc_debug, store_dc_debug),
};


int owlfb_dc_create_debug_sysfs(struct owlfb_device *fbdev)
{
	int i;
	int r;
	DBG("create sysfs for fbs\n");

	for (i = 0; i < fbdev->num_fbs; i++) {
		int t;
		for (t = 0; t < ARRAY_SIZE(owlfb_dc_attrs); t++) {
			r = device_create_file(fbdev->fbs[i]->dev,&owlfb_dc_attrs[t]);

			if (r) {
				dev_err(fbdev->dev, "failed to create sysfs "
						"file\n");
				return r;
			}
		}
	}

	return 0;
}
