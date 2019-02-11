#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>

#include <mach/hardware.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>

#define MAX_RESET_ID_SIZE 6
#define NO_ACK_ID 0xffffffff

static struct mutex powergate_mutex;

static unsigned int sps_pg_ctl;

struct owl_powergate_info {
	char name[16];             /* name of powergate */
	unsigned int reset_id[MAX_RESET_ID_SIZE];
	unsigned int pwr_id;
	unsigned int ack_id;
	int count;                 /* power on count */
	int init_power_off;        /* power off at boot init stage */
	int use_mutex;             /* need use mutex? */
};

static struct owl_powergate_info powergate_info[] = {
	[OWL_POWERGATE_CPU2] = {
		.name = "cpu2",
		.pwr_id = 5,
		.ack_id = 21,
		/* for CPU2, init_power_off shoud be 0 */
		.init_power_off = 0,
		.use_mutex = 0,
		.count = 0,
	},

	[OWL_POWERGATE_CPU3] = {
		.name = "cpu3",
		.pwr_id = 6,
		.ack_id = 22,
		/* for CPU3, init_power_off shoud be 0 */
		.init_power_off = 0,
		.use_mutex = 0,
		.count = 0,
	},

	[OWL_POWERGATE_GPU3D] = {
		.name = "gpu3d",
		.reset_id = {MOD_ID_GPU3D},
		.pwr_id = 3,
		.ack_id = NO_ACK_ID,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	[OWL_POWERGATE_VCE_BISP] = {
		.name = "vce/bisp",
		.reset_id = {MOD_ID_VCE, MOD_ID_BISP},
		.pwr_id = 1,
		.ack_id = 17,
		/* for VCE_BISP, init_power_off shoud be 1 */
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	[OWL_POWERGATE_VDE] = {
		.name = "vde",
		.reset_id = {MOD_ID_VDE},
		.pwr_id = 0,
		.ack_id = 16,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	[OWL_POWERGATE_USB2_0] = {
		.name = "usb2_0",
		.reset_id = {MOD_ID_USB2_0},
		.pwr_id = 11,
		.ack_id = 15,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_USB2_1] = {
		.name = "usb2_1",
		.reset_id = {MOD_ID_USB2_1},
		.pwr_id = 2,
		.ack_id = 18,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_USB3] = {
		.name = "usb3",
		.reset_id = {MOD_ID_USB3},
		.pwr_id = 10,
		.ack_id = 14,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_DS] = {
		.name = "ds",
		.reset_id = {MOD_ID_DE, MOD_ID_LCD, MOD_ID_HDMI, MOD_ID_TVOUT, MOD_ID_DSI},
		.pwr_id = 9,
		.ack_id = 13,
		.init_power_off = 0,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_DMA] = {
		.name = "dma",
		.reset_id = {MOD_ID_DMAC},
		.pwr_id = 8,
		.ack_id = 12,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
};

static unsigned int owl_cpu_domains[] = {
	0xffffffff,
	0xffffffff,
	OWL_POWERGATE_CPU2,
	OWL_POWERGATE_CPU3,
};

static DEFINE_SPINLOCK(owl_powergate_lock);

static int owl_powergate_set(enum owl_powergate_id id, bool on)
{
	struct owl_powergate_info *pgi = &powergate_info[id];
	bool ack_is_on;
	unsigned long val, flags;
	int timeout, i, reset_id;
	


	spin_lock_irqsave(&owl_powergate_lock, flags);
		
	if (pgi->ack_id != NO_ACK_ID) {
		ack_is_on = (act_readl(SPS_PG_CTL) & (1 << pgi->ack_id));

		if (ack_is_on == on) {
			spin_unlock_irqrestore(&owl_powergate_lock, flags);
			return 0;
		}
	}

	/* assert modules reset before poweron */
	if (on) {
		if (id == OWL_POWERGATE_CPU2) {
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val &= ~(1 << 6);
			act_writel(val, CMU_CORECTL);
		} else if (id == OWL_POWERGATE_CPU3) {
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val &= ~(1 << 7);
			act_writel(val, CMU_CORECTL);
		} else {
			for (i = 0; i < MAX_RESET_ID_SIZE; i++) {
				reset_id = pgi->reset_id[i];
				if (reset_id != MOD_ID_ROOT)
					owl_module_reset_assert(reset_id);
			}
		}
	}

	val = act_readl(SPS_PG_CTL);
	if (on)
		val |= (1 << pgi->pwr_id);
	else
		val &= ~(1 << pgi->pwr_id);
	act_writel(val, SPS_PG_CTL);

	if (on) {
		timeout = 5000;  /* 5ms */
		while (timeout > 0 && !owl_powergate_is_powered(id)) {
			udelay(50);
			timeout -= 50;
		}
		if (timeout <= 0) {
			pr_err("[PowerGate] enable power for id %d timeout\n",
			       id);
		}
		udelay(10);
		
		/* deasert modules reset after poweron */
		if (id == OWL_POWERGATE_CPU2) {
			/* clk en */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 2);
			act_writel(val, CMU_CORECTL);
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 6);
			act_writel(val, CMU_CORECTL);
		} else if (id == OWL_POWERGATE_CPU3) {
			/* clk en */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 3);
			act_writel(val, CMU_CORECTL);
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 7);
			act_writel(val, CMU_CORECTL);
		} else {
			for (i = 0; i < MAX_RESET_ID_SIZE; i++) {
				reset_id = pgi->reset_id[i];
				if (reset_id != MOD_ID_ROOT) {
					module_clk_enable(reset_id);
					owl_module_reset_deassert(reset_id);
				}
			}
		}
	}

	spin_unlock_irqrestore(&owl_powergate_lock, flags);



	return 0;
}

int owl_powergate_power_on(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	int ret;

	if (id < 0 || id >= OWL_POWERGATE_MAXID)
		return -EINVAL;

	pgi = &powergate_info[id];



	if (pgi->use_mutex)
		mutex_lock(&powergate_mutex);
	pgi->count++;

	if (pgi->ack_id != NO_ACK_ID &&
		owl_powergate_is_powered(id) > 0) {
		if (pgi->use_mutex) {
			pr_err("[PowerGate] '%s', skip power on, count %d\n",
				pgi->name, pgi->count);

			mutex_unlock(&powergate_mutex);
		}
		return 0;
	}

	ret = owl_powergate_set(id, true);

	if (pgi->use_mutex)
		mutex_unlock(&powergate_mutex);

	return ret;
}
EXPORT_SYMBOL(owl_powergate_power_on);

int owl_powergate_power_off(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	int ret = 0;

	if (id < 0 || id >= OWL_POWERGATE_MAXID)
		return -EINVAL;

	pgi = &powergate_info[id];



	if (pgi->use_mutex)
		mutex_lock(&powergate_mutex);

	if (WARN(pgi->count <= 0,
		"unbalanced power off for %s\n", pgi->name)) {
		if (pgi->use_mutex)
			mutex_unlock(&powergate_mutex);
		return -EIO;
	}

	pgi->count--;
	if (pgi->count == 0) {


		ret = owl_powergate_set(id, false);
	}

	if (pgi->use_mutex)
		mutex_unlock(&powergate_mutex);

	return ret;
}
EXPORT_SYMBOL(owl_powergate_power_off);

int owl_powergate_is_powered(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	u32 status;

	if (id < 0 || id >= OWL_POWERGATE_MAXID)
		return -EINVAL;

	pgi = &powergate_info[id];
	if (pgi->ack_id == NO_ACK_ID)
		return 1;

	status = act_readl(SPS_PG_CTL) & (1 << pgi->ack_id);



	return !!status;
}
EXPORT_SYMBOL(owl_powergate_is_powered);

int owl_powergate_suspend(void)
{
	struct owl_powergate_info *pgi;
	int i;


	
	sps_pg_ctl = act_readl(SPS_PG_CTL);
	
	for (i = 0; i < OWL_POWERGATE_MAXID; i++) {
		pgi = &powergate_info[i];

		if (owl_powergate_is_powered(i) > 0) {
			pgi->init_power_off = 0;
		} else {
			pgi->init_power_off = 1;
		}
	}

	return 0;
}

int owl_powergate_resume(void)
{
	struct owl_powergate_info *pgi;
	int i;



	for (i = 0; i < OWL_POWERGATE_MAXID; i++) {
		pgi = &powergate_info[i];

		if (pgi->init_power_off == 0 && owl_powergate_is_powered(i) == 0) {
			owl_powergate_set(i, true);
		}
	}
	
	act_writel(sps_pg_ctl, SPS_PG_CTL);

	return 0;
}

int owl_cpu_powergate_id(int cpuid)
{
	if (cpuid > 1 && cpuid < ARRAY_SIZE(owl_cpu_domains))
		return owl_cpu_domains[cpuid];

	return -EINVAL;
}

int owl_powergate_init(void)
{
	struct owl_powergate_info *pgi;
	int i;
	
	for (i = 0; i < OWL_POWERGATE_MAXID; i++) {
		pgi = &powergate_info[i];

		if (owl_powergate_is_powered(i) > 0) {
			if (pgi->init_power_off) {
				pr_debug("[PowerGate] %s(): '%s', init off\n",
					__func__, pgi->name);
				/* power off */
				owl_powergate_set(i, false);
				pgi->count = 0;
			} else {
				pgi->count = 1;
			}
		} else {
			pgi->count = 0;
		}


	}
	
	mutex_init(&powergate_mutex);

	return 0;
}

arch_initcall(owl_powergate_init);

