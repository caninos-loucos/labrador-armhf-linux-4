#ifndef __PINCTRL_COMMON_OWL_H__ 
#define __PINCTRL_COMMON_OWL_H__

#include "pinctrl_data-owl.h"

#define PINCTRL_DBG(format, ...) \
    pr_debug(format, ## __VA_ARGS__)

#define PINCTRL_ERR(format, ...) \
	printk(KERN_ERR "caninos pinctrl: " format, ## __VA_ARGS__)

/**
 * struct owl_pinctrl_soc_info - Actions SOC pin controller per-SoC configuration
 * @gpio_ranges: An array of GPIO ranges for this SoC
 * @gpio_num_ranges: The number of GPIO ranges for this SoC
 * @pins:	An array describing all pins the pin controller affects.
 *		All pins which are also GPIOs must be listed first within the
 *		array, and be numbered identically to the GPIO controller's
 *		numbering.
 * @npins:	The number of entries in @pins.
 * @functions:	The functions supported on this SoC.
 * @nfunction:	The number of entries in @functions.
 * @groups:	An array describing all pin groups the pin SoC supports.
 * @ngroups:	The number of entries in @groups.
 */
struct owl_pinctrl_soc_info {
	struct device *dev;

	const struct owl_pinconf_pad_info *padinfo;
	
	const struct pinctrl_pin_desc *pins;
	unsigned npins;
	
	const struct owl_pinmux_func *functions;
	unsigned nfunctions;
	
	const struct owl_group *groups;
	unsigned ngroups;
	
	struct owl_gpio_pad_data *owl_gpio_pad_data;
};



#endif /* __PINCTRL_COMMON_OWL_H__ */
