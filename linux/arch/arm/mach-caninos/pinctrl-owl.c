#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/consumer.h>

#include <asm/mach/irq.h>

#include <../drivers/pinctrl/core.h>
#include <../drivers/gpio/gpiolib.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include "pinctrl_common-owl.h"
#include "pinctrl_data-owl.h"
#include "pinctrl.h"

#define DRIVER_NAME "caninos-labrador-pinctrl"

extern spinlock_t owl_gpio_lock;

struct owl_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctl;
	const struct owl_pinctrl_soc_info *info;
};

#define MFP_CTL_REG(i)    (MFP_CTL0 + (MFP_CTL1 - MFP_CTL0) * (i))
#define PAD_ST_REG(i)     (PAD_ST0 + (PAD_ST1 - PAD_ST0) * (i))
#define PAD_PULLCTL_REG(i) (PAD_PULLCTL0 + (PAD_PULLCTL1 - PAD_PULLCTL0) *(i))
#define PAD_DRV_REG(i)    (PAD_DRV0 + (PAD_DRV1 - PAD_DRV0) * (i))
#define GPIO_OUTEN_REG(i) (GPIO_AOUTEN + (GPIO_BOUTEN - GPIO_AOUTEN) * (i))
#define GPIO_INEN_REG(i)  (GPIO_AINEN + (GPIO_BINEN - GPIO_AINEN) * (i))
#define GPIO_DAT_REG(i)   (GPIO_ADAT + (GPIO_BDAT - GPIO_ADAT) * (i))

#define PINCTRL_PULLREGS 3
#define PINCTRL_DRVREGS	 3

struct dupcode {
	u32 regaddr;
	u32 regmask;

	u32 regval;
	int refcnt;
};

static struct dupcode mfp_dupcodes[] = {
	{MFP_CTL0, (0x1 << 2), 0, 0},
	{MFP_CTL1, (0x7 << 29), 0, 0},
	{MFP_CTL1, (0x7 << 26), 0, 0},
};

static int mfp_dupcode_cnt = ARRAY_SIZE(mfp_dupcodes);

/* part 1, pinctrl groups */
static const struct owl_gpio_pad_data *owl_gpio_pad_data = NULL;

static void owl_gpio_pad_set(int gpio, int val)
{
	int i;
	unsigned long irq_flags;
	unsigned int dat;

	if(owl_gpio_pad_data == NULL)
		return;
    
	for(i = 0; i < owl_gpio_pad_data->size; i++) {
		if(owl_gpio_pad_data->gpio_pads[i].gpio == gpio) {
			spin_lock_irqsave(&owl_gpio_lock, irq_flags);
			if(val) {
				owl_gpio_pad_data->gpio_pads[i].ref_count++;
				if(owl_gpio_pad_data->gpio_pads[i].ref_count != 1) {
					spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
					return;
				}
			}
			else {
				owl_gpio_pad_data->gpio_pads[i].ref_count--;
				if(owl_gpio_pad_data->gpio_pads[i].ref_count != 0) {
					spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
					return;
				}
			}
			dat = act_readl(SPS_PWR_CTL) & (~owl_gpio_pad_data->gpio_pads[i].mask);
			dat |= val << owl_gpio_pad_data->gpio_pads[i].bit;
			act_writel(dat, owl_gpio_pad_data->gpio_pads[i].reg);
			spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
			return;
		}
	}
}

static void owl_gpio_pad_enable(int gpio)
{
	owl_gpio_pad_set(gpio, 1);
}

static void owl_gpio_pad_disable(int gpio)
{
	owl_gpio_pad_set(gpio, 0);
}

static int owl_pctlops_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;

	PINCTRL_DBG("%s\n", __FUNCTION__);

	return info->ngroups;
}

static const char *owl_pctlops_get_group_name(struct pinctrl_dev *pctldev,
	unsigned selector)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;

	PINCTRL_DBG("%s(selector:%d)\n", __FUNCTION__, selector);

	return info->groups[selector].name;
}

static int owl_pctlops_get_group_pins(struct pinctrl_dev *pctldev, 
	unsigned selector, const unsigned **pins, unsigned *num_pins)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;

	PINCTRL_DBG("%s(selector:%d)\n", __FUNCTION__, selector);

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->groups[selector].pads;
	*num_pins = info->groups[selector].padcnt;

	return 0;
}

static void owl_pctlops_pin_dbg_show(struct pinctrl_dev *pctldev,
	struct seq_file *s, unsigned offset)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);

	seq_printf(s, "%s", dev_name(apctl->dev));
}

static int reserve_map(struct device *dev, struct pinctrl_map **map,
		       unsigned *reserved_maps, unsigned *num_maps,
		       unsigned reserve)
{
	unsigned old_num = *reserved_maps;
	unsigned new_num = *num_maps + reserve;
	struct pinctrl_map *new_map;

	if (old_num >= new_num)
		return 0;

	new_map = krealloc(*map, sizeof(*new_map) * new_num, GFP_KERNEL);
	if (!new_map) {
		dev_err(dev, "krealloc(map) failed\n");
		return -ENOMEM;
	}

	memset(new_map + old_num, 0, (new_num - old_num) * sizeof(*new_map));

	*map = new_map;
	*reserved_maps = new_num;

	return 0;
}

static int add_map_mux(struct pinctrl_map **map, unsigned *reserved_maps,
		       unsigned *num_maps, const char *group,
		       const char *function)
{
	if (WARN_ON(*num_maps == *reserved_maps))
		return -ENOSPC;

	(*map)[*num_maps].type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)[*num_maps].data.mux.group = group;
	(*map)[*num_maps].data.mux.function = function;
	(*num_maps)++;

	return 0;
}

static int add_map_group_configs(struct device *dev, struct pinctrl_map **map,
			   unsigned *reserved_maps, unsigned *num_maps,
			   const char *group, unsigned long *configs,
			   unsigned num_configs)
{
	unsigned long *dup_configs;

	if (WARN_ON(*num_maps == *reserved_maps))
		return -ENOSPC;

	dup_configs = kmemdup(configs, num_configs * sizeof(*dup_configs),
			      GFP_KERNEL);
	if (!dup_configs) {
		dev_err(dev, "kmemdup(configs) failed\n");
		return -ENOMEM;
	}

	(*map)[*num_maps].type = PIN_MAP_TYPE_CONFIGS_GROUP;
	(*map)[*num_maps].data.configs.group_or_pin = group;
	(*map)[*num_maps].data.configs.configs = dup_configs;
	(*map)[*num_maps].data.configs.num_configs = num_configs;
	(*num_maps)++;

	return 0;
}

static int add_map_pin_configs(struct device *dev, struct pinctrl_map **map,
			   unsigned *reserved_maps, unsigned *num_maps,
			   const char *pin, unsigned long *configs,
			   unsigned num_configs)
{
	unsigned long *dup_configs;

	if (WARN_ON(*num_maps == *reserved_maps))
		return -ENOSPC;

	dup_configs = kmemdup(configs, num_configs * sizeof(*dup_configs),
			      GFP_KERNEL);
	if (!dup_configs) {
		dev_err(dev, "kmemdup(configs) failed\n");
		return -ENOMEM;
	}

	(*map)[*num_maps].type = PIN_MAP_TYPE_CONFIGS_PIN;
	(*map)[*num_maps].data.configs.group_or_pin = pin;
	(*map)[*num_maps].data.configs.configs = dup_configs;
	(*map)[*num_maps].data.configs.num_configs = num_configs;
	(*num_maps)++;

	return 0;
}


static int add_config(struct device *dev, unsigned long **configs,
		      unsigned *num_configs, unsigned long config)
{
	unsigned old_num = *num_configs;
	unsigned new_num = old_num + 1;
	unsigned long *new_configs;

	new_configs = krealloc(*configs, sizeof(*new_configs) * new_num,
			       GFP_KERNEL);
	if (!new_configs) {
		dev_err(dev, "krealloc(configs) failed\n");
		return -ENOMEM;
	}

	new_configs[old_num] = config;

	*configs = new_configs;
	*num_configs = new_num;

	return 0;
}

static void owl_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
				      struct pinctrl_map *map,
				      unsigned num_maps)
{
	int i;

	for (i = 0; i < num_maps; i++)
		if (map[i].type == PIN_MAP_TYPE_CONFIGS_GROUP || map[i].type == PIN_MAP_TYPE_CONFIGS_PIN)
			kfree(map[i].data.configs.configs);

	kfree(map);
}

struct cfg_param {
	const char *property;
	enum owl_pinconf_param param;
};

static const struct cfg_param cfg_params[] = {
	{"caninos,pull",			OWL_PINCONF_PARAM_PULL},
	{"caninos,paddrv",		OWL_PINCONF_PARAM_PADDRV},
};

static int owl_pinctrl_dt_subnode_to_map(struct device *dev,
					   struct device_node *np,
					   struct pinctrl_map **map,
					   unsigned *reserved_maps,
					   unsigned *num_maps)
{
	int ret, i;
	const char *function;
	u32 val;
	unsigned long config;
	unsigned long *configs = NULL;
	unsigned num_configs = 0;
	unsigned reserve;
	struct property *prop;
	int groups_prop_num;
	int pins_prop_num;
	int groups_or_pins_prop_num;

	ret = of_property_read_string(np, "caninos,function", &function);
	if (ret < 0) {
		/* EINVAL=missing, which is fine since it's optional */
		if (ret != -EINVAL)
			dev_err(dev,
				"could not parse property caninos,function\n");
		function = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(cfg_params); i++) {
		ret = of_property_read_u32(np, cfg_params[i].property, &val);
		if (!ret) {
			config = OWL_PINCONF_PACK(cfg_params[i].param, val);
			ret = add_config(dev, &configs, &num_configs, config);
			if (ret < 0)
				goto exit;
		/* EINVAL=missing, which is fine since it's optional */
		} else if (ret != -EINVAL) {
			dev_err(dev, "could not parse property %s\n",
				cfg_params[i].property);
		}
	}

	reserve = 0;
	if (function != NULL)
		reserve++;
	if (num_configs)
		reserve++;

	ret = of_property_count_strings(np, "caninos,pins");
	if (ret < 0) {
		if (ret != -EINVAL)
			dev_err(dev, "could not parse property caninos,pins\n");

		pins_prop_num = 0;
	} else {
		pins_prop_num = ret;
	}

	if(pins_prop_num > 0 && function != NULL) {
		dev_err(dev, "could not assign caninos,pins to function\n");
		goto exit;
	}

	ret = of_property_count_strings(np, "caninos,groups");
	if (ret < 0) {
		if (ret != -EINVAL)
			dev_err(dev, "could not parse property caninos,groups\n");

		groups_prop_num = 0;
	} else {
		groups_prop_num = ret;
	}

	groups_or_pins_prop_num = groups_prop_num + pins_prop_num;
	if(groups_or_pins_prop_num == 0) {
		dev_err(dev, "no property caninos,pins or caninos,groups\n");
		goto exit;
	}
	
	reserve *= groups_or_pins_prop_num;

	ret = reserve_map(dev, map, reserved_maps, num_maps, reserve);
	if (ret < 0)
		goto exit;

	if(groups_prop_num > 0) {
		const char *group;
		of_property_for_each_string(np, "caninos,groups", prop, group) {
			if (function) {
				ret = add_map_mux(map, reserved_maps, num_maps,
						  group, function);
				if (ret < 0)
					goto exit;
			}

			if (num_configs) {
				ret = add_map_group_configs(dev, map, reserved_maps,
						      num_maps, group, configs,
						      num_configs);
				if (ret < 0)
					goto exit;
			}
		}
	}

	if(pins_prop_num > 0) {
		const char *pin;
		of_property_for_each_string(np, "caninos,pins", prop, pin) {
			if (num_configs) {
				ret = add_map_pin_configs(dev, map, reserved_maps,
						      num_maps, pin, configs,
						      num_configs);
				if (ret < 0)
					goto exit;
			}
		}
	}

	ret = 0;

exit:
	kfree(configs);
	return ret;
}

static int owl_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
					struct device_node *np_config,
					struct pinctrl_map **map,
					unsigned *num_maps)
{
	unsigned reserved_maps;
	struct device_node *np;
	int ret;

	reserved_maps = 0;
	*map = NULL;
	*num_maps = 0;

	for_each_child_of_node(np_config, np) {
		ret = owl_pinctrl_dt_subnode_to_map(pctldev->dev, np, map,
						      &reserved_maps, num_maps);
		if (ret < 0) {
			owl_pinctrl_dt_free_map(pctldev, *map, *num_maps);
			return ret;
		}
	}

	return 0;
}



static struct pinctrl_ops owl_pctlops_ops = {
	.get_groups_count = owl_pctlops_get_groups_count,
	.get_group_name = owl_pctlops_get_group_name,
	.get_group_pins = owl_pctlops_get_group_pins,
	.pin_dbg_show = owl_pctlops_pin_dbg_show,
	.dt_node_to_map = owl_pinctrl_dt_node_to_map,
	.dt_free_map = owl_pinctrl_dt_free_map,

};


/* part 2, pinctrl pinmux */

static int inc_dupcode(struct dupcode *code, 
					u32 group_regaddr, u32 group_mask, u32 group_val)
{
	int comaskfound;
	unsigned int comask;
	unsigned int coval;
	
	comaskfound=0;

	if(code->regaddr != group_regaddr)
		return 0;

	PINCTRL_DBG("%s\n", __FUNCTION__);

	comask = (group_mask & code->regmask);
	if (comask) {
		if (comask != code->regmask)  {
			PINCTRL_DBG("dupcode mask error !\n");
			PINCTRL_DBG("group reg<0x%x> mask = 0x%x !\n",group_regaddr, group_mask);
			PINCTRL_DBG("code mask = 0x%x!\n", code->regmask);
			return -1;
		}
		comaskfound=1;
	} else {
		return 0;
	}

	if (comaskfound) {
		if (code->refcnt > 0) {
			coval = (code->regmask & group_val);
 			if (coval != code->regval)  {
				PINCTRL_DBG("dupcode coval error !\n");
				PINCTRL_DBG("group reg<0x%x> val = 0x%x !\n",group_regaddr, group_val);
				PINCTRL_DBG("code val = 0x%x!\n", code->regval);

				return -1;
			}
			code->refcnt++;
		} else {
			coval = (code->regmask & group_val);
 			code->regval = coval;
			code->refcnt = 1;
		}
	}


	return comaskfound;
}

static int lookup_dupcode_idx(struct dupcode *code, int dupcode_num, 
						u32 group_regaddr, u32 group_mask)
{
	int i;

	for(i = 0; i < dupcode_num; i++) {
		if((code[i].regaddr == group_regaddr) && 
			(group_mask & code[i].regmask))
			return i;
	}

	return -1;
}

static inline int get_group_mfp_mask_val(const struct owl_group *g, int function, u32 *mask, u32 *val)
{
	int i;
	u32 option_num;
	u32 option_mask;

	for (i = 0; i < g->nfuncs; i++) {
		if (g->funcs[i] == function)
			break;
	}
	if (WARN_ON(i == g->nfuncs))
		return -EINVAL;

	option_num = (1 << g->mfpctl_width);
	if(i > option_num)
		i -= option_num;

	option_mask = option_num - 1;
 	*mask = (option_mask  << g->mfpctl_shift);
	*val = (i << g->mfpctl_shift);

	return 0;

}

static int pinmux_request_check_gpio(struct pinctrl_dev *pctldev, u32 pin)
{
	struct pin_desc *desc;
	desc = pin_desc_get(pctldev, pin);

	if(desc->gpio_owner){
		PINCTRL_ERR(KERN_ALERT "%s\n", __FUNCTION__);
		PINCTRL_ERR(KERN_ALERT "CHECK PMX:%s has already been requested by %s",
				desc->name, desc->gpio_owner);
	}

	return 0;
}

static int gpio_request_check_pinmux(struct pinctrl_dev *pctldev, u32 pin)
{
	struct pin_desc *desc;
	desc = pin_desc_get(pctldev, pin);

	if(desc->mux_owner){
		PINCTRL_ERR("%s\n", __FUNCTION__);
		PINCTRL_ERR("CHECK PMX:%s has already been requested by %s",
				desc->name, desc->mux_owner);
	}

	return 0;
}

static int owl_pmxops_request(struct pinctrl_dev *pctldev, unsigned pin)
{
	return pinmux_request_check_gpio(pctldev, pin);
}

static int owl_pmxops_enable(struct pinctrl_dev *pctldev,
	unsigned function, unsigned group)
{
	const struct owl_group *g;
	int ret = 0;
	int i;
	int inc;
	int mfpdup_idx;
	int need_schimtt = 0;
	u32 group_schimtt_val[PINCTRL_STREGS] = {0};
	u32 group_schimtt_mask[PINCTRL_STREGS] = {0};

	u32 g_val;
	u32 g_mask;

	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;


 	PINCTRL_DBG("%s function:%d '%s', group:%d '%s'\n", __FUNCTION__,
       		function, info->functions[function].name,
       		group, info->groups[group].name);

	g = &info->groups[group];

	if(g->mfpctl_regnum >= 0) {
		u32 mfpval;

		if(get_group_mfp_mask_val(g, function, &g_mask, &g_val)){
			return -EINVAL;
		}

		mfpdup_idx = lookup_dupcode_idx(mfp_dupcodes, mfp_dupcode_cnt, 
						MFP_CTL_REG(g->mfpctl_regnum), g_mask);

		if(mfpdup_idx >= 0) {
			inc = inc_dupcode(&mfp_dupcodes[mfpdup_idx], 
					MFP_CTL_REG(g->mfpctl_regnum), g_mask, g_val);
			if (inc < 0) {
				ret = -EINVAL;
				goto pmxen_fail;
			}
		}

		/*******we've done all the checkings. From now on ,we will set hardware.*****************/
		/*******No more errors should happen, otherwise it will be hard to roll back***************/
		mfpval = act_readl(MFP_CTL_REG(g->mfpctl_regnum));
		PINCTRL_DBG("read mfpval = 0x%x\n", mfpval);
		mfpval &= (~ g_mask);
		mfpval |= g_val;
		PINCTRL_DBG("write mfpval = 0x%x\n", mfpval);
		act_writel(mfpval, MFP_CTL_REG(g->mfpctl_regnum));
		PINCTRL_DBG("read mfpval again = 0x%x\n",
				act_readl(MFP_CTL_REG(g->mfpctl_regnum)));
	}

	/*check each pad of this group for schimtt info, fill the group_schimtt_mask & group_schimtt_val*/
	for(i = 0; i < g->padcnt; i++) {
		int pad_num;
		const struct owl_pinconf_pad_info *pad_info;
		struct owl_pinconf_schimtt *schimtt_info;

		pad_num = g->pads[i];
		pad_info = info->padinfo;

		owl_gpio_pad_enable(pad_info[pad_num].gpio);
			
		schimtt_info = pad_info[pad_num].schimtt;

		if(schimtt_info && schimtt_info->reg_num >= 0) {
			int j;

			need_schimtt = 1;

			group_schimtt_mask[schimtt_info->reg_num] |= (1 << schimtt_info->shift);

			for(j = 0; j < schimtt_info->num_schimtt_funcs; j++) {
				if(schimtt_info->schimtt_funcs[j] == function) {
					group_schimtt_val[schimtt_info->reg_num] |= (1 << schimtt_info->shift);
					break;
				}
			}
		}
	}

	/*set schimtt val*/
	if(need_schimtt){
		u32 val;
		u32 reg;
		for(i = 0; i < PINCTRL_STREGS; i++) {
			if(group_schimtt_mask[i] != 0) {
				reg = PAD_ST_REG(i);
				val = act_readl(reg);
				val &= (~group_schimtt_mask[i]);
				val |= group_schimtt_val[i];
				act_writel(val, reg);
			}
		}
	}

	return 0;

pmxen_fail:
	return ret;
}

static int owl_pmxops_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;



	return info->nfunctions;
}

static const char *owl_pmxops_get_func_name(struct pinctrl_dev *pctldev,
	unsigned selector)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;



	return info->functions[selector].name;
}

static int owl_pmxops_get_groups(struct pinctrl_dev *pctldev,
	unsigned selector, const char * const **groups,
	unsigned * const num_groups)
{
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;



	*groups = info->functions[selector].groups;
	*num_groups = info->functions[selector].ngroups;

	return 0;
}

static int owl_pmxops_gpio_request_enable (struct pinctrl_dev *pctldev,
	struct pinctrl_gpio_range *range, unsigned offset)
{
	if (range->id == 0 && range->base == 0) {
		u32 gpio_num;
		
		gpio_num = offset - range->pin_base + range->base;
		owl_gpio_pad_enable(gpio_num);
	}
	return gpio_request_check_pinmux(pctldev, offset);
}

static void owl_pmxops_gpio_disable_free (struct pinctrl_dev *pctldev,
	struct pinctrl_gpio_range *range, unsigned offset)
{
	int bank;
	unsigned long tmp;
	unsigned long mask;
	unsigned long flags;
	u32 gpio_num;

	if (range->id == 0 && range->base == 0) {
		gpio_num = offset - range->pin_base + range->base;
		bank = (gpio_num >> 5);
		mask = (1 << (gpio_num & 0x1f));
		if (bank < 5) {
			spin_lock_irqsave(&owl_gpio_lock, flags);

			tmp = act_readl(GPIO_INEN_REG(bank));
			tmp &= ~mask;
			act_writel(tmp, GPIO_INEN_REG(bank));

			tmp = act_readl(GPIO_OUTEN_REG(bank));
			tmp &= ~mask;
			act_writel(tmp, GPIO_OUTEN_REG(bank));

			spin_unlock_irqrestore(&owl_gpio_lock, flags);
		}
		owl_gpio_pad_disable(gpio_num);
	}
}

static struct pinmux_ops owl_pmxops_ops = {
	.get_functions_count = owl_pmxops_get_funcs_count,
	.get_function_name = owl_pmxops_get_func_name,
	.get_function_groups = owl_pmxops_get_groups,
	.set_mux = owl_pmxops_enable,
	.request = owl_pmxops_request,
	.gpio_request_enable = owl_pmxops_gpio_request_enable,
	.gpio_disable_free = owl_pmxops_gpio_disable_free,
};

/* part 3, pinctrl pinconfs */

static int owl_group_pinconf_reg(const struct owl_group *g, 
    enum owl_pinconf_param param, u32 *reg, u32 *bit, u32 *width)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PADDRV:
		if(g->paddrv_regnum < 0)
			return -EINVAL;

		*reg = PAD_DRV_REG(g->paddrv_regnum);
		*bit = g->paddrv_shift;
		*width = g->paddrv_width;
		break;
	default:
		return -EINVAL;
	}

	return 0;
	
}

static int owl_group_pinconf_arg2val(const struct owl_group *g,
									enum owl_pinconf_param param, 
									u32 arg, u32 *val)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PADDRV:
		*val = arg;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_group_pinconf_val2arg(const struct owl_group *g,
									enum owl_pinconf_param param, 
									u32 val, u32 *arg)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PADDRV:
		*arg = val;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_pad_pinconf_reg(const struct owl_pinconf_pad_info *pad,
								enum owl_pinconf_param param,
								u32 *reg, u32 *bit, u32 *width)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PULL:
		if((!pad->pull) || (pad->pull->reg_num < 0))
			return -EINVAL;
		
		*reg = PAD_PULLCTL_REG(pad->pull->reg_num);
		*bit = pad->pull->shift;
		*width = pad->pull->width;
		break;
	case OWL_PINCONF_PARAM_SCHMITT:
		PINCTRL_ERR("Cannot configure pad schmitt yet!\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_pad_pinconf_arg2val(const struct owl_pinconf_pad_info *pad,
									enum owl_pinconf_param param, 
									u32 arg, u32 *val)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PULL:
		switch(arg) {
		case OWL_PINCONF_PULL_NONE:
			*val = 0;
			break;
		case OWL_PINCONF_PULL_DOWN:
			if(pad->pull->pulldown)
				*val = pad->pull->pulldown;
			else
				return -EINVAL;
			break;
		case OWL_PINCONF_PULL_UP:
			if(pad->pull->pullup)
				*val = pad->pull->pullup;
			else
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}

		break;

	case OWL_PINCONF_PARAM_SCHMITT:
		PINCTRL_ERR("Cannot configure pad schmitt yet!\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_pad_pinconf_val2arg(const struct owl_pinconf_pad_info *pad,
    enum owl_pinconf_param param, u32 val, u32 *arg)
{
	switch (param)
	{
	case OWL_PINCONF_PARAM_PULL:
		if(pad->pull->pulldown && (val == pad->pull->pulldown))
			*arg = OWL_PINCONF_PULL_DOWN;
		else if(pad->pull->pullup && (val == pad->pull->pullup))
			*arg = OWL_PINCONF_PULL_UP;
		else if(val == 0)
			*arg = OWL_PINCONF_PULL_NONE;
		else
			return -EINVAL;

		break;

	case OWL_PINCONF_PARAM_SCHMITT:
		PINCTRL_ERR("Cannot configure pad schmitt yet!\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_confops_pin_config_get(struct pinctrl_dev *pctldev,
    unsigned pin, unsigned long *config)
{
	int ret;
	struct owl_pinctrl *apctl;
	const struct owl_pinctrl_soc_info *info;
	const struct owl_pinconf_pad_info *pad_tab;
	u32 reg = 0, bit = 0, width = 0;
	u32 val, mask;
	u32 tmp;
	u32 arg = 0;
	enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(*config);

	apctl = pinctrl_dev_get_drvdata(pctldev);
	info = apctl->info;
	pad_tab = &info->padinfo[pin];
	
	// We get config for those pins we CAN get it for and that's it

	ret = owl_pad_pinconf_reg(pad_tab, param, &reg, &bit, &width);
	
	if(ret)
	{
		return ret;
	}

	tmp = act_readl(reg);
	mask = (1 << width) - 1;
	val = (tmp >> bit) & mask;

	ret = owl_pad_pinconf_val2arg(pad_tab, param, val, &arg);
	
	if(ret)
	{
	    return ret;
	}

	*config = OWL_PINCONF_PACK(param, arg);

	return ret;
}

static int owl_confops_pin_config_set(struct pinctrl_dev *pctldev,
    unsigned pin, unsigned long *configs, unsigned num_configs)
{
	int ret;
	struct owl_pinctrl *apctl;
	const struct owl_pinctrl_soc_info *info;
	const struct owl_pinconf_pad_info *pad_tab;
	u32 reg = 0, bit = 0, width = 0;
	u32 val = 0, mask = 0;
	u32 tmp;
	int i = 0;

	for (i = 0; i < num_configs; i++)
	{
		enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(configs[i]);
		u32 arg = OWL_PINCONF_UNPACK_ARG(configs[i]);


 
		apctl = pinctrl_dev_get_drvdata(pctldev);
		
		info = apctl->info;
		pad_tab = &info->padinfo[pin];

		ret = owl_pad_pinconf_reg(pad_tab, param, &reg, &bit, &width);
		
		if(ret)
		{
			return ret;
		}

		ret = owl_pad_pinconf_arg2val(pad_tab, param, arg, &val);
		
		if(ret)
		{
			return ret;
		}

		// Update register
		mask = (1 << width) - 1;
		mask = mask << bit;
		tmp = act_readl(reg);
		tmp &= ~mask;
		tmp |= val << bit;
		act_writel(tmp, reg);
	}

	return ret;
}

static int owl_confops_group_config_get(struct pinctrl_dev *pctldev,
    unsigned group, unsigned long *config)
{
	int ret = 0;
	const struct owl_group *g;
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;
	u32 reg, bit, width;
	u32 val, mask;
	u32 tmp;
	u32 arg = 0;
	enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(*config);

	g = &info->groups[group];

	ret = owl_group_pinconf_reg(g, param, &reg, &bit, &width);
	
	if(ret)
	{
		return ret;
	}

	tmp = act_readl(reg);
	mask = (1 << width) - 1;
	val = (tmp >> bit) & mask;

	ret = owl_group_pinconf_val2arg(g, param, val, &arg);
	
	if(ret)
	{
		return ret;
	}

	*config = OWL_PINCONF_PACK(param, arg);

	return ret;
}

static int owl_confops_group_config_set(struct pinctrl_dev *pctldev,
    unsigned group, unsigned long *configs,unsigned num_configs)
{
	int ret = 0;
	const struct owl_group *g;
	struct owl_pinctrl *apctl = pinctrl_dev_get_drvdata(pctldev);
	const struct owl_pinctrl_soc_info *info = apctl->info;
	u32 reg, bit, width;
	u32 val, mask;
	u32 tmp;
	int i = 0;

	for (i = 0; i < num_configs; i++)
	{
		enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(configs[i]);
		
		u32 arg = OWL_PINCONF_UNPACK_ARG(configs[i]);

		g = &info->groups[group];
		
		ret = owl_group_pinconf_reg(g, param, &reg, &bit, &width);
		
		if(ret)
		{
			return ret;
		}

		ret = owl_group_pinconf_arg2val(g, param, arg, &val);
		
		if(ret)
		{
			return ret;
		}

		// Update register
		mask = (1 << width) - 1;
		mask = mask << bit;
		tmp = act_readl(reg);
		tmp &= ~mask;
		tmp |= val << bit;
		act_writel(tmp, reg);
	}

	return ret;
}

static struct pinconf_ops owl_confops_ops = {
	.pin_config_get = owl_confops_pin_config_get,
	.pin_config_set = owl_confops_pin_config_set,
	.pin_config_group_get = owl_confops_group_config_get,
	.pin_config_group_set = owl_confops_group_config_set,
};

static struct pinctrl_desc owl_pinctrl_desc = {
	.name = DRIVER_NAME,
	.owner = THIS_MODULE,
	.pins = atm7059_pads,
	.npins = NUM_PADS,
	.pctlops = &owl_pctlops_ops,
	.pmxops = &owl_pmxops_ops,
	.confops = &owl_confops_ops,
};

static struct owl_pinctrl_soc_info owl_pinctrl_info = {
	.padinfo = atm7059_pad_tab,
	.pins = atm7059_pads,
	.functions = atm7059_functions,
	.groups = atm7059_groups,
	.owl_gpio_pad_data = &atm7059_gpio_pad_data,
	.npins = NUM_PADS,
	.ngroups = ARRAY_SIZE(atm7059_groups),
	.nfunctions = ARRAY_SIZE(atm7059_functions),
};

static struct of_device_id owl_pinctrl_of_match[] = {
	{ .compatible = "caninos,labrador-pinctrl" },
	{ },
};

static int owl_pinctrl_probe(struct platform_device *pdev)
{
    struct owl_pinctrl_soc_info * info = &owl_pinctrl_info;
	struct owl_pinctrl * apctl = NULL;
	int err;
	


	info->dev = &pdev->dev;

	owl_gpio_pad_data = info->owl_gpio_pad_data;
    
	apctl = devm_kzalloc(&pdev->dev, sizeof(*apctl), GFP_KERNEL);
	
	if (!apctl)
	{
	    pr_err("Memory allocation failed\n");
	    err = -ENOMEM;
	    goto err_kzalloc;
	}

	apctl->info = info;
	apctl->dev = info->dev;

	pr_info("NFunctions: %d, NGroups: %d\n", info->nfunctions, info->ngroups);

	apctl->pctl = pinctrl_register(&owl_pinctrl_desc, &pdev->dev, apctl);
	
	if (IS_ERR(apctl->pctl))
	{
		pr_err("Could not register pinmux driver\n");
		err = -EINVAL;
		goto err_no_pmx;
	}
	
	platform_set_drvdata(pdev, apctl);
	
 
	return 0;

err_no_pmx:
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, apctl);
	
err_kzalloc:
    pr_info("Driver probe finished with error: %d\n", err);
	return err;
}

static struct platform_driver owl_pinctrl_driver = {
    .probe = owl_pinctrl_probe,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = owl_pinctrl_of_match,
	},
};

static int __init owl_pinctrl_init(void)
{
	return platform_driver_register(&owl_pinctrl_driver);
}

subsys_initcall(owl_pinctrl_init);

