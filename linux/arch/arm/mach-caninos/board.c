/*
    Filename: board.c
    Module  : Caninos Labrador Core Board Driver
    Author  : Edgar Bernardi Righi
    Company : LSITEC
    Date    : January 2019
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/memblock.h>
#include <linux/highmem.h>

#include <asm/system_info.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/smp_twd.h>
#include <asm/cacheflush.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>

#include <asm/hardware/cache-l2x0.h>

#include <mach/hardware.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>
#include <mach/power.h>
#include <mach/irqs.h>

#include "pmic_regs.h"

#define DRIVER_NAME "board-setup"

#define INFO_MSG(fmt,...) pr_info(DRIVER_NAME ": " fmt, ##__VA_ARGS__)
#define ERR_MSG(fmt,...) pr_err(DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define KINFO_SIZE (SZ_1M)
#define FB_SIZE (8 * SZ_1M)

#define BOOT_FLAG (0x55aa)
#define CPU_SHIFT(cpu) (19 + cpu)

#define PMIC_I2C_ADDR  (0x65)
#define CPU_CORE_FREQ  (1104) // 1104 MHz
#define CPU_CORE_VOLT  (1175) // 1175 mV

// CPU Recommended Frequency/Voltage Pairs
//
//  408MHz at  950mV
//  720MHz at  975mV
//  900MHz at 1025mV
// 1104MHz at 1175mV
// 1308MHz at 1250mV
//
// PMIC DCDC1 Regulator Operating Limits
//
// Max Voltage : 1400mV
// Min Voltage :  700mV
// Voltage Step:   25mV
// Min Selector:    0
// Max Selector:   28 
// Stable After:  350us
//

extern phys_addr_t arm_lowmem_limit;

extern void board_secondary_startup(void); // headsmp.S

static phys_addr_t s_phy_mem_size_saved;

static unsigned int owl_kinfo_start;
static unsigned int owl_fb_start;

static DEFINE_RAW_SPINLOCK(boot_lock);

static void __iomem *scu_base_addr(void)
{
	return (void *)IO_ADDRESS(OWL_PA_SCU);
}

void __init board_smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores = scu_base ? scu_get_core_count(scu_base) : 1;

	INFO_MSG("Number of cores %d\n", ncores);
	
	if (ncores > nr_cpu_ids) {
		ncores = nr_cpu_ids;
	}
	
	for (i = 0; i < ncores; i++) {
		set_cpu_possible(i, true);
	}
}

void __init board_smp_prepare_cpus(unsigned int max_cpus)
{
	scu_enable(scu_base_addr());
}

static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

void __init board_secondary_init(unsigned int cpu)
{
	trace_hardirqs_off();
	write_pen_release(-1);
	raw_spin_lock(&boot_lock);
	raw_spin_unlock(&boot_lock);
}

static void wakeup_secondary(unsigned int cpu)
{
	enum owl_powergate_id cpuid;

	cpuid = owl_cpu_powergate_id(cpu);
	
	owl_powergate_power_on(cpuid);
	
	udelay(200);
	
	switch (cpu)
	{
	case 1:
		module_reset(MODULE_RST_DBG1RESET);
		udelay(10);
		
		act_writel(virt_to_phys(board_secondary_startup), CPU1_ADDR);
		act_writel(BOOT_FLAG, CPU1_FLAG);
		break;
	case 2:
		act_writel(virt_to_phys(board_secondary_startup), CPU2_ADDR);
		act_writel(BOOT_FLAG, CPU2_FLAG);
		break;
	case 3:
		act_writel(virt_to_phys(board_secondary_startup), CPU3_ADDR);
		act_writel(BOOT_FLAG, CPU3_FLAG);
		break;
	}
	
	dsb_sev();
	mb();
}

int __init board_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
	
	wakeup_secondary(cpu);
	
	udelay(10);
	
	raw_spin_lock(&boot_lock);
	
	write_pen_release(cpu_logical_map(cpu));
	
	smp_send_reschedule(cpu);

	timeout = jiffies + (1 * HZ);
	
	while (time_before(jiffies, timeout)) {
		if (pen_release == -1)
			break;
	}

	switch (cpu) {
	case 1:
		act_writel(0, CPU1_ADDR);
		act_writel(0, CPU1_FLAG);
		break;
	case 2:
		act_writel(0, CPU2_ADDR);
		act_writel(0, CPU2_FLAG);
		break;
	case 3:
		act_writel(0, CPU3_ADDR);
		act_writel(0, CPU3_FLAG);
		break;
	}
	
	raw_spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

static struct smp_operations board_smp_ops =
{
    .smp_init_cpus = board_smp_init_cpus,
    .smp_prepare_cpus = board_smp_prepare_cpus,
    .smp_secondary_init = board_secondary_init,
    .smp_boot_secondary = board_boot_secondary,
};

static struct map_desc board_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(OWL_PA_REG_BASE),
		.pfn		= __phys_to_pfn(OWL_PA_REG_BASE),
		.length		= OWL_PA_REG_SIZE,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= OWL_VA_BOOT_RAM,
		.pfn		= __phys_to_pfn(OWL_PA_BOOT_RAM),
		.length		= SZ_4K,
		.type		= MT_MEMORY_RWX,
	},
};

static void __init board_map_io(void)
{
	iotable_init(board_io_desc, ARRAY_SIZE(board_io_desc));
}

static void __init board_reserve(void)
{
	phys_addr_t phy_mem_size, phy_mem_end;

	phy_mem_size = memblock_phys_mem_size();
	
	if (phy_mem_size & (phy_mem_size - 1))
	{
		uint _tmp = __fls(phy_mem_size);
		
		if (_tmp > 0 && (phy_mem_size & (1U << (_tmp - 1))))
		{
			// close to next boundary
			_tmp++;
			phy_mem_size = (_tmp >= sizeof(phy_mem_size) * 8) ? phy_mem_size : (1U << _tmp);
		}
		else
		{
			phy_mem_size = 1U << _tmp;
		}
	}
	
	s_phy_mem_size_saved = phy_mem_size;
	
	phy_mem_end = arm_lowmem_limit;

	memblock_reserve(0, 0x4000); // reserve low 16K for DDR dqs training
	
	phy_mem_end -= FB_SIZE;
	
	owl_fb_start = phy_mem_end;
	
	memblock_reserve(owl_fb_start, FB_SIZE);

	phy_mem_end -= KINFO_SIZE;
	
    owl_kinfo_start = phy_mem_end;
    
    memblock_reserve(owl_kinfo_start, KINFO_SIZE);
}

static void __init timer0_setup(void)
{
    // disable the clock of timer
	act_clearl(1 << 27, CMU_DEVCLKEN1);
	
	// enable timer0
	// set its value to zero and disable IRQs

	act_writel(0, T0_VAL);
	act_writel(0, T0_CMP);
	act_writel(4, T0_CTL);
	
	// disable timer1
	act_writel(0, T1_CTL);
	act_writel(0, T1_VAL);
	act_writel(0, T1_CMP);
	
    // enable the clock of timer
	act_setl(1 << 27, CMU_DEVCLKEN1);
	
	INFO_MSG("Timer0 early setup completed\n");
}

static void __init timer0_delay_us(u32 delay)
{
    u32 timer_value;
    
    if (delay == 0)
    {
        ERR_MSG("Invalid timer0 delay value\n");
        return;
    }
    
    // 1us = 24 clock cycles (24MHz)
	timer_value = delay * 24;
    
    // disable the clock of timer
	act_clearl(1 << 27, CMU_DEVCLKEN1);
	
	// set timer0 value to zero
	act_writel(0, T0_VAL);
	
    // enable the clock of timer
	act_setl(1 << 27, CMU_DEVCLKEN1);
	
	// busy wait
	while (act_readl(T0_VAL) < timer_value);
}

static void __init i2c0_setup(void)
{
	act_setl(1U << 0, CMU_ETHERNETPLL);
	
	timer0_delay_us(600);
	
	act_setl(1U << 14, CMU_DEVCLKEN1);
	act_readl(CMU_DEVCLKEN1);
	
	act_clearl(1U << 12, CMU_DEVRST1);
	act_readl(CMU_DEVRST1);
	
	timer0_delay_us(20);
	
	act_setl(1U << 12, CMU_DEVRST1);
	act_readl(CMU_DEVRST1);
	
	timer0_delay_us(50);

	// I2C clock divider (400kHz)
	
	act_writel((5U << 8) | (16U << 0), I2C0_CLKDIV);
	
	INFO_MSG("I2C0 early setup completed\n");
}

static int __init i2c0_write_reg(u8 bus_addr, u8 reg, u16 data)
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

static int __init i2c0_read_reg(u8 bus_addr, u8 reg, u16 * data)
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

static inline int pmic_reg_read(u8 reg, u16 * data) {
    return i2c0_read_reg(PMIC_I2C_ADDR, reg, data);
}

static inline int pmic_reg_write(u8 reg, u16 data) {
    return i2c0_write_reg(PMIC_I2C_ADDR, reg, data);
}

static int __init pmic_reg_setbits(u8 reg, u16 mask, u16 val)
{
    u16 tmp, orig;
	int ret;
	
	ret = pmic_reg_read(reg, &orig);
	
	if (ret) {
		return ret;
	}
	
	tmp = orig & ~mask;
	tmp |= val & mask;
	
	if (tmp != orig) {
	    ret = pmic_reg_write(reg, tmp);
	}
	
	return ret;
}

static int __init pmic_cmu_reset(void)
{
	u16 reg_val;
	int ret;
	
	ret = pmic_reg_read(ATC2603C_CMU_DEVRST, &reg_val);
	
	if (ret) {
		return ret;
	}
	
	ret = pmic_reg_write(ATC2603C_CMU_DEVRST, reg_val & ~(0x1 << 2));
	
	if (ret) {
		return ret;
	}
	
	timer0_delay_us(50);
	
	ret = pmic_reg_write(ATC2603C_CMU_DEVRST, reg_val);
	
	if (ret) {
		return ret;
	}
	
	timer0_delay_us(50);
	
	return 0;
}

static int __init pmic_setup(void)
{
    u16 data;
    int ret;
    
    timer0_setup();
    
    i2c0_setup();
    
    ret = pmic_reg_read(ATC2603C_PMU_OC_INT_EN, &data);
    
    if (ret) {
		return ret;
    }
    
    if (data != 0x1bc0)
    {
        ERR_MSG("Invalid PMIC model\n");
        return -1;
    }
    
    // setup dbg ctl reg
    
    ret = pmic_reg_read(ATC2603C_PMU_BDG_CTL, &data);
    
    if (ret) {
		return ret;
    }
    
	data |= (0x1 << 7);   // dbg enable
	data |= (0x1 << 6);   // dbg filter
	data &= ~(0x1 << 5);  // disable pulldown resistor
	data &= ~(0x1 << 11); // efuse
	
	ret = pmic_reg_write(ATC2603C_PMU_BDG_CTL, data);
	
	if (ret) {
		return ret;
    }
	
	// setup interrupts
	// reset ATC260X INTC
	
	ret = pmic_cmu_reset();    
	
	if (ret) {
		return ret;
    }
    
	// disable all sources
	
	ret = pmic_reg_write(ATC2603C_INTS_MSK, 0);
	
	if (ret) {
		return ret;
    }
    
    // Enable P_EXTIRQ pad
    
	ret = pmic_reg_setbits(ATC2603C_PAD_EN, 0x1, 0x1);
	
	if (ret) {
		return ret;
    }
    
    INFO_MSG("PMIC early setup completed\n");
    return 0;
}

static int __init pmic_dcdc1_set_selector(int selector)
{
    int ret;
    
	ret = pmic_reg_setbits(ATC2603C_PMU_DC1_CTL0, (0x1F << 7), selector << 7);
	
	if (ret < 0)
	{
	    ERR_MSG("Could not set PMIC voltage selector\n");
	    return ret;
	}
    
    timer0_delay_us(350);
    return 0;
}

static int __init cpu_set_clock(u32 freq, u32 voltage)
{
    u32 val, old_freq, locked;
    int selector, ret;
	
	// find the nearest valid voltage and validate it
	// it must round up
	if (voltage % 25) {
	    voltage = ((voltage / 25) + 1) * 25;
	}
	if (voltage < 700 || voltage > 1400) {
	    return -1;
	}
	
	// calculate the voltage selector
	selector = (voltage - 700) / 25;
	
	// find the nearest valid frequency and validate it
	// it must round down
	freq = (freq / 12) * 12;
	if (freq < 408 || freq > 1308) {
	    return -1;
	}
	
	// read current core clock frequency in MHz
	val = act_readl(CMU_COREPLL);
	old_freq = (val & 0xff) * 12;
	
	// if both frequencies are equal, only update the core voltage
	if (freq == old_freq) {
	    return pmic_dcdc1_set_selector(selector);
	}
	
	// if the new frequency is bigger than the current running frequency
	// the core voltage must be increased/updated before upping the clock
	if (freq > old_freq)
	{
	    ret = pmic_dcdc1_set_selector(selector);
	    if (ret < 0) {
	        return ret;
	    }
	}
	
	// update the core clock
	val &= ~(0xff);
	act_writel(val | (freq / 12), CMU_COREPLL);
	
	// wait for the core PLL to lock
	do {
	    locked = (act_readl(CMU_COREPLLDEBUG) >> 11) & 0x1;
	} while(!locked);
	
	// if the new frequency is smaller than the old frequency
	// the core voltage must be decreased/updated now
	if (freq < old_freq)
	{
        ret = pmic_dcdc1_set_selector(selector);
	    if (ret < 0) {
	        return ret;
	    }
    }
    
    INFO_MSG("CPU running at %uMHz and %umV\n", freq, voltage);
    INFO_MSG("CPU early setup completed\n");
    return 0;
}

static void __init board_check_revision(void)
{
	const u64 phys_offset = __pa(PAGE_OFFSET);

	u8 * vddr = kmap_atomic(pfn_to_page(PFN_DOWN(phys_offset)));
	
	memcpy(&system_serial_low, vddr + 0x800, sizeof(system_serial_low));
	memcpy(&system_serial_high, vddr + 0x804, sizeof(system_serial_high));
	
	kunmap_atomic(vddr);
}

/////////////// ATC2603C

#define PMU_SYS_CTL1_EN_S1 (1 << 0)
#define PMU_SYS_CTL3_EN_S2 (1 << 15)
#define PMU_SYS_CTL3_EN_S3 (1 << 14)
/*
static int pmic_set_wakeup_src(uint wakeup_mask, uint wakeup_src)
{
	uint i, reg_addr, reg_val, reg_mask, tmp_mask;
	
	u16 reg_vals[4], reg_masks[4];
	
	int ret;
	
	if (wakeup_mask == 0)
	{
		return 0;
	}
	
	memset(reg_vals, 0, sizeof(reg_vals));
	memset(reg_masks, 0, sizeof(reg_masks));
	
	wakeup_src_desc = sc_atc260x_pm_wakeup_src_desc_tbl[atc260x_pm->pmic_type];
	
	tmp_mask = wakeup_mask;
	
	while (tmp_mask)
	{
		uint src_index, comb_bit, reg_index, reg_bit;
		
		uint tmp_mask1 = tmp_mask & ~(tmp_mask -1U);
		
		tmp_mask &= ~tmp_mask1;
		
		src_index = __ffs(tmp_mask1);
		
		comb_bit = wakeup_src_desc->bit_tbl[src_index][0];
		
		if (comb_bit == 255)
		{
			if (wakeup_src & (1U<<src_index))
			{
				dev_warn(atc260x_pm->dev, "%s() src %u (mask 0x%x) not support\n", __func__, src_index, (1U<<src_index));
			}
			continue;
		}
		reg_index = comb_bit / 16U;
		reg_bit = comb_bit % 16U;


		reg_masks[reg_index] |= 1U << reg_bit;
		
		if (wakeup_src & (1U << src_index)) {
			reg_vals[reg_index] |= 1U << reg_bit;
		}
	}
	for (i = 0; i < ARRAY_SIZE(reg_masks); i++)
	{
		reg_addr = wakeup_src_desc->regs[i];
		
		if (reg_masks[i] != 0 && reg_addr != 0xffff)
		{
			ret = atc260x_reg_setbits(atc260x_pm->atc260x, reg_addr, reg_masks[i], reg_vals[i]);
			
			if (ret) {
				dev_err(atc260x_pm->dev, "%s() io err, ret=%d\n", __func__, ret);
				return -EIO;
			}
		}
	}
	
	reg_val = reg_mask = 0;
	
	if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_REMCON)
	{
		reg_mask |= PMU_SYS_CTL5_REMCON_DECT_EN;
		
		if (wakeup_src & OWL_PMIC_WAKEUP_SRC_REMCON)
		{
			reg_val |= PMU_SYS_CTL5_REMCON_DECT_EN;
		}
	}
	
	atc260x_reg_setbits(atc260x_pm->atc260x,ATC2603C_PMU_SYS_CTL5, reg_mask, reg_val);
	
	return 0;
}*/

static int pmic_powerdown(bool deep_pwrdn)
{
	int ret;
	
	ret = pmic_reg_setbits(ATC2603C_PMU_SYS_CTL3, PMU_SYS_CTL3_EN_S2 | PMU_SYS_CTL3_EN_S3, deep_pwrdn ? 0 : PMU_SYS_CTL3_EN_S3);
	
	if (ret) {
		return ret;
	}

	return pmic_reg_setbits(ATC2603C_PMU_SYS_CTL1, PMU_SYS_CTL1_EN_S1, 0);
}

static void board_pm_halt(void)
{
    int ret;
    
	// power off system
	// default sleep mode and wakeup source
	
	INFO_MSG("System power off\n");
	
	//pmic_set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ALL, OWL_PMIC_WAKEUP_SRC_RESET | OWL_PMIC_WAKEUP_SRC_ONOFF_LONG);
	
	ret = pmic_powerdown(false);
	
	if (ret) {
	    ERR_MSG("Power off failed");
	}
	
	while (1) {
		mdelay(200);
	}
}

//EXPORT_SYMBOL_GPL(board_pm_halt);

///////////////

static void __init board_l2x0_init(void)
{
	act_writel(0x78800002, (unsigned int)OWL_PA_L2CC + L310_PREFETCH_CTRL);
	
	act_writel(L310_DYNAMIC_CLK_GATING_EN | L310_STNDBY_MODE_EN, (unsigned int)OWL_PA_L2CC + L310_POWER_CTRL);
}

static void __init board_init_early(void)
{
    int ret;
    
	board_check_revision();
	
	//board_l2x0_init();

	ret = pmic_setup();
	
    if (ret < 0)
    {
        ERR_MSG("Could not setup the PMIC\n");
        return;
    }
    
    ret = cpu_set_clock(CPU_CORE_FREQ, CPU_CORE_VOLT);
    
    if (ret < 0)
    {
        ERR_MSG("Could not set CPU core speed\n");
        return;
    }
}

static void __init board_init_irq(void)
{
	irqchip_init();
}

static bool __init board_smp_init(void)
{
    smp_set_ops(&board_smp_ops);
    return true;
}

static struct of_device_id board_dt_match_table[] __initdata = {
	{ .compatible = "simple-bus", },
	{}
};

static void board_init(void)
{
	int ret;
	
	pm_power_off = board_pm_halt;
	
	INFO_MSG("System halt enabled\n");

	ret = of_platform_populate(NULL, board_dt_match_table, NULL, NULL);
	
	if (ret) {
		ERR_MSG("OF platform populate failed\n");
	}
}

static const char * board_dt_match[] __initconst = {
	"caninos,labrador",
	NULL,
};

// Instruction prefetch enable
// Data prefetch enable
// Round-robin replacement
// Use AWCACHE attributes for WA
// 32kB way size, 16 way associativity
// Disable exclusive cache

#define L310_MASK (0xc0000fff)

#define L310_VAL (L310_AUX_CTRL_INSTR_PREFETCH | L310_AUX_CTRL_DATA_PREFETCH \
                 | L310_AUX_CTRL_NS_INT_CTRL | L310_AUX_CTRL_NS_LOCKDOWN \
                 | L310_AUX_CTRL_CACHE_REPLACE_RR \
                 | L2C_AUX_CTRL_WAY_SIZE(2) | L310_AUX_CTRL_ASSOCIATIVITY_16)

DT_MACHINE_START(CANINOS, "labrador")
	.dt_compat    = board_dt_match,
	.atag_offset  = 0x00000100,
	.l2c_aux_val  = L310_VAL,
	.l2c_aux_mask = L310_MASK,
	.smp_init     = board_smp_init,
	.init_early   = board_init_early,
	.map_io       = board_map_io,
	.reserve      = board_reserve,
	.init_irq     = board_init_irq,
	.init_machine = board_init,
MACHINE_END

