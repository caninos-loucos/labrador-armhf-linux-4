#include <linux/module.h>
#include <linux/init.h>
#include <asm-generic/barrier.h>

#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/mmc/host.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/hdmac-owl.h>
#include <mach/hardware.h>

// Uncoment the line below to enable debug messages.
#define DEBUG 

#ifdef DEBUG
#define INFO_MSG(dev,...) dev_info(dev,__VA_ARGS__)
#else
#define INFO_MSG(dev,...)
#endif

#define ERR_MSG(dev,...) dev_err(dev,__VA_ARGS__)

// DMA mode config

#define SDC0WT_DMAMODE (0x00010202)	// DDR->FIFO
#define SDC1WT_DMAMODE (0x00010203)
#define SDC2WT_DMAMODE (0x00010204)

#define SDC0RD_DMAMODE (0x00040802)	// FIFO->DDR
#define SDC1RD_DMAMODE (0x00040803)
#define SDC2RD_DMAMODE (0x00040804)

//

#define SDC_WDELAY_LOW_CLK  (0xf)
#define SDC_WDELAY_MID_CLK  (0xa)
#define SDC_WDELAY_HIGH_CLK (0x9)

#define SDC_RDELAY_LOW_CLK	(0xf)
#define SDC_RDELAY_MID_CLK	(0xa)
#define SDC_RDELAY_HIGH_CLK (0x9)

//

#define SD_EN_OFFSET			0x0000
#define SD_CTL_OFFSET			0x0004
#define SD_STATE_OFFSET			0x0008
#define SD_CMD_OFFSET			0x000c
#define SD_ARG_OFFSET			0x0010
#define SD_RSPBUF0_OFFSET		0x0014
#define SD_RSPBUF1_OFFSET		0x0018
#define SD_RSPBUF2_OFFSET		0x001c
#define SD_RSPBUF3_OFFSET		0x0020
#define SD_RSPBUF4_OFFSET		0x0024
#define SD_DAT_OFFSET			0x0028
#define SD_BLK_SIZE_OFFSET		0x002c
#define SD_BLK_NUM_OFFSET		0x0030
#define SD_BUF_SIZE_OFFSET		0x0034

// Register SD_EN

#define SD_EN_RANE			(1 << 31)
#define SD_EN_RAN_SEED(x)	(((x) & 0x3f) << 24)
#define SD_EN_S18EN			(1 << 12)
#define SD_EN_RESE			(1 << 10)
#define SD_EN_DAT1_S		(1 << 9)
#define SD_EN_CLK_S			(1 << 8)
#define SD_ENABLE			(1 << 7)
#define SD_EN_BSEL			(1 << 6)
#define SD_EN_SDIOEN		(1 << 3)
#define SD_EN_DDREN			(1 << 2)
#define SD_EN_DATAWID(x)	(((x) & 0x3) << 0)

// Register SD_CTL

#define SD_CTL_TOUTEN		(1 << 31)
#define SD_CTL_TOUTCNT(x)	(((x) & 0x7f) << 24)
#define SD_CTL_RDELAY(x)	(((x) & 0xf) << 20)
#define SD_CTL_WDELAY(x)	(((x) & 0xf) << 16)
#define SD_CTL_CMDLEN		(1 << 13)
#define SD_CTL_SCC			(1 << 12)
#define SD_CTL_TCN(x)		(((x) & 0xf) << 8)
#define SD_CTL_TS			(1 << 7)
#define SD_CTL_LBE			(1 << 6)
#define SD_CTL_C7EN			(1 << 5)
#define SD_CTL_TM(x)		(((x) & 0xf) << 0)

// Register SD_STATE

#define SD_STATE_DAT1BS			(1 << 18)
#define SD_STATE_SDIOB_P		(1 << 17)
#define SD_STATE_SDIOB_EN		(1 << 16)
#define SD_STATE_TOUTE			(1 << 15)
#define SD_STATE_BAEP			(1 << 14)
#define SD_STATE_MEMRDY			(1 << 12)
#define SD_STATE_CMDS			(1 << 11)
#define SD_STATE_DAT1AS			(1 << 10)
#define SD_STATE_SDIOA_P		(1 << 9)
#define SD_STATE_SDIOA_EN		(1 << 8)
#define SD_STATE_DAT0S			(1 << 7)
#define SD_STATE_TEIE			(1 << 6)
#define SD_STATE_TEI			(1 << 5)
#define SD_STATE_CLNR			(1 << 4)
#define SD_STATE_CLC			(1 << 3)
#define SD_STATE_WC16ER			(1 << 2)
#define SD_STATE_RC16ER			(1 << 1)
#define SD_STATE_CRC7ER			(1 << 0)

//

struct sdiohost
{
    u32 module_id;
    const char * clk_id;
    
    struct mmc_host * mmc;
    struct regulator * reg;
    struct pinctrl * pcl;
    struct clk * clk;
    
    spinlock_t lock;
    
    void __iomem * iobase;
    u32 physbase;
	
	struct completion dma_complete;
	struct completion sdc_complete;
	
    int irq_num;
    
    u32 clock;
    u32 actual_clock;
    u32 power_mode;
    u32 chip_select;
    u32 bus_width;
    u32 signal_voltage;
    
    bool sdc_enabled;
    bool ddr_enabled;
    
    bool low_voltage_support;
   
	bool sdio_mode;
	bool sdio_irq_enabled;
    
    int card_en_gpio;
    int card_pw_gpio;
    int card_dt_gpio;
    
    enum dma_data_direction	dma_dir;
    struct dma_slave_config dma_conf;
    
    struct dma_chan	* dma;
    struct owl_dma_slave dma_slave;
    struct dma_async_tx_descriptor * desc;
    
    u32 dma_wmode, dma_rmode;
};

static void set_timing(struct mmc_host * mmc, u32 freq)
{
    u32 mode;
    
    struct sdiohost * host = mmc_priv(mmc);
    
    INFO_MSG(mmc->parent, "timing adjusted to freq %u\n", freq);
    
    mode = readl(host->iobase + SD_CTL_OFFSET);
	smp_rmb();
	
	mode &= ~(0xff << 16);
    
	if (freq <= 1000000)
	{
	    mode |= SD_CTL_RDELAY(SDC_RDELAY_LOW_CLK);
	    mode |= SD_CTL_WDELAY(SDC_WDELAY_LOW_CLK);
	}
	else if ((freq > 1000000) && (freq <= 26000000))
	{
	    mode |= SD_CTL_RDELAY(SDC_RDELAY_MID_CLK);
	    mode |= SD_CTL_WDELAY(SDC_WDELAY_MID_CLK);
	}
	else if ((freq > 26000000) && (freq <= 52000000)) 
	{
	    mode |= SD_CTL_RDELAY(SDC_RDELAY_HIGH_CLK);
	    mode |= SD_CTL_WDELAY(SDC_WDELAY_HIGH_CLK);
	}
	else
	{
	    mode |= SD_CTL_RDELAY(6);
	    mode |= SD_CTL_WDELAY(4);
	}
	
	writel(mode, host->iobase + SD_CTL_OFFSET);
	smp_wmb();
}

static void set_actual_clock(struct mmc_host * mmc, u32 freq)
{
    unsigned long rate;
    struct sdiohost * host = mmc_priv(mmc);
    
    if (freq == 0) {
        return;
    }
    
    if (host->actual_clock == freq) {
        return;
    }
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    INFO_MSG(mmc->parent, "actual clock set to %u\n", freq);
    host->actual_clock = freq;
    
    rate = clk_round_rate(host->clk, freq);
    clk_set_rate(host->clk, rate);
    mdelay(1);
    
    set_timing(mmc, freq);
}

static void controller_chip_select(struct mmc_host * mmc, u32 chip_select, bool show)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;
    
    if (chip_select == MMC_CS_DONTCARE) {
        return;
    }
    
    if (chip_select == host->chip_select) {
        return;
    }
    
    if (host->bus_width != MMC_BUS_WIDTH_1) {
        return;
    }
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    smp_rmb();
    
    enable &= ~0x3;
    enable |= SD_EN_RESE;
    
    if (chip_select == MMC_CS_HIGH)
    {
		//enable |= 0x1;
		
		INFO_MSG(mmc->parent, "chip select set to high\n");
		
		host->chip_select = MMC_CS_HIGH;
    }
    else
    {
        INFO_MSG(mmc->parent, "chip select set to low\n");
        
        host->chip_select = MMC_CS_LOW;
    }

    writel(enable, host->iobase + SD_EN_OFFSET);
    smp_wmb();
}

static void controller_enable(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;
    
    if (host->sdc_enabled) {
        return;
    }
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    smp_rmb();
    
    enable &= ~SD_EN_S18EN;  // enable 3.3V signaling
    
    enable &= ~SD_EN_DAT1_S; // select SD0_D1A pad
    enable &= ~SD_EN_CLK_S;  // select SD0_CLKA pad
    
    enable |= SD_ENABLE;     // enable SD module
    
    if (host->sdio_mode) {
    	enable |= SD_EN_SDIOEN;
    }
    else {
    	enable &= ~SD_EN_SDIOEN;
    }
    
    writel(enable, host->iobase + SD_EN_OFFSET);
    smp_wmb();
    
    INFO_MSG(mmc->parent, "controller enabled\n");
    host->sdc_enabled = true;
}

static void controller_enable_sdio_irq(struct mmc_host * mmc)
{
	struct sdiohost * host = mmc_priv(mmc);
    u32 state;
    
    if (!host->sdio_mode) {
    	return;
    }
    
    if (host->sdio_irq_enabled) {
        return;
    }
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    state = readl(host->iobase + SD_STATE_OFFSET);
    smp_rmb();
    
    state |= SD_STATE_SDIOA_EN; // enable sdio irq
    state |= SD_STATE_SDIOA_P; // clear sdio irq
    
    writel(state, host->iobase + SD_STATE_OFFSET);
    smp_wmb();
    
    INFO_MSG(mmc->parent, "controller sdio irq enabled\n");
    host->sdio_irq_enabled = true;
}

static void controller_disable_sdio_irq(struct mmc_host * mmc)
{
	struct sdiohost * host = mmc_priv(mmc);
    u32 state;
    
    if (!host->sdio_mode) {
    	return;
    }
    
    if (!host->sdio_irq_enabled) {
        return;
    }
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    state = readl(host->iobase + SD_STATE_OFFSET);
    smp_rmb();
    
    state &= ~SD_STATE_SDIOA_EN; // disable sdio irq
    state |= SD_STATE_SDIOA_P; // clear sdio irq
    
    writel(state, host->iobase + SD_STATE_OFFSET);
    smp_wmb();
    
    INFO_MSG(mmc->parent, "controller sdio irq disabled\n");
    host->sdio_irq_enabled = false;
}

static void controller_enable_irq(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 state;
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    state = readl(host->iobase + SD_STATE_OFFSET);
    smp_rmb();

    state |= SD_STATE_TEIE; // enable transfer end IRQ
    state |= SD_STATE_TEI; // clear transfer end IRQ
    
    writel(state, host->iobase + SD_STATE_OFFSET);
    smp_wmb();
    
    INFO_MSG(mmc->parent, "controller irq enabled\n");
    
    reinit_completion(&host->sdc_complete);
}

static void controller_power_up(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);

    if (host->power_mode == MMC_POWER_UP) {
        return;
    }
    
    INFO_MSG(mmc->parent, "device power up\n");
    host->power_mode = MMC_POWER_UP;
    
    if (host->card_pw_gpio >= 0) {
        gpio_direction_output(host->card_pw_gpio, 0);
    }
    
    if (host->card_en_gpio >= 0) {
        gpio_direction_output(host->card_en_gpio, 0);
    }
    
    module_clk_disable(host->module_id);
    
    mdelay(20);
    
    module_clk_enable(host->module_id);
    module_reset(host->module_id);
    
    set_actual_clock(mmc, mmc->f_init);
    
    controller_chip_select(mmc, MMC_CS_HIGH, true);
    
    if (host->card_pw_gpio >= 0) {
        gpio_set_value(host->card_pw_gpio, 1);
    }
    
    mdelay(40);
}

static void controller_power_on(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    if (host->power_mode == MMC_POWER_ON) {
        return;
    }
    
    INFO_MSG(mmc->parent, "device power on\n");
    host->power_mode = MMC_POWER_ON;
    
    controller_enable(mmc);
    
    controller_enable_irq(mmc);
    
    //controller_enable_sdio_irq(mmc);
    
    if (host->card_en_gpio >= 0) {
        gpio_set_value(host->card_en_gpio, 1);
    }
    
    mdelay(40);
}

static void controller_power_off(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    INFO_MSG(mmc->parent, "device power off\n");
    host->power_mode = MMC_POWER_OFF;
    
    if (host->card_pw_gpio >= 0) {
        gpio_set_value(host->card_pw_gpio, 0);
    }
    
    if (host->card_en_gpio >= 0) {
        gpio_set_value(host->card_en_gpio, 0);
    }
    
    host->clock          = 0;
	host->actual_clock   = 0;
    host->power_mode     = MMC_POWER_UNDEFINED;
    host->chip_select    = MMC_CS_DONTCARE;
    host->bus_width      = MMC_BUS_WIDTH_1;
    host->signal_voltage = MMC_SIGNAL_VOLTAGE_330;
    
    host->sdc_enabled = false;
    host->ddr_enabled = false;
    
    host->sdio_irq_enabled = false;
    
    module_clk_disable(host->module_id);
    mdelay(20);
}

static void controller_set_clock(struct mmc_host * mmc, u32 freq)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    u32 mode;

    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    if (freq == host->clock) {
        return;
    }
    
    set_actual_clock(mmc, freq);
    
    mode = readl(host->iobase + SD_CTL_OFFSET);
	smp_rmb();
	
	if (freq == 0)
	{
	    INFO_MSG(mmc->parent, "clock to card disabled\n");
	    mode &= ~SD_CTL_SCC;
	}
	else
	{
	    INFO_MSG(mmc->parent, "clock to card enabled\n");
	    mode |= SD_CTL_SCC;
	}
	
	writel(mode, host->iobase + SD_CTL_OFFSET);
	smp_wmb();
	
	host->clock = freq;
}

static void controller_set_bus_width(struct mmc_host * mmc, u32 bus_width)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;

    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    if (bus_width == host->bus_width) {
        return;
    }
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    smp_rmb();
    
    enable &= ~0x3;
    enable |= SD_EN_RESE;
    
    switch (bus_width)
    {
    case MMC_BUS_WIDTH_1:
        INFO_MSG(mmc->parent, "bus width set to 1 bit\n");
        break;
    
    case MMC_BUS_WIDTH_4:
        INFO_MSG(mmc->parent, "bus width set to 4 bits\n");
        enable |= 0x1;
        break;
        
    case MMC_BUS_WIDTH_8:
        INFO_MSG(mmc->parent, "bus width set to 8 bits\n");
        enable |= 0x2;
        break;
    }

    writel(enable, host->iobase + SD_EN_OFFSET);
    smp_wmb();
    
    host->bus_width = bus_width;
}

static void sdc_reset_hardware_state_mach(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 mode;
    
    mode = readl(host->iobase + SD_CTL_OFFSET);
    
    if (mode & SD_CTL_TS)
    {
        mode &= ~SD_CTL_TS;
        writel(mode, host->iobase + SD_CTL_OFFSET);
        mdelay(1);
    }
}

static int sdiohost_card_busy(struct mmc_host * mmc)
{
	struct sdiohost * host = mmc_priv(mmc);
	unsigned long flags;
	u32 state, i = 0;
	
	spin_lock_irqsave(&host->lock, flags);
	
	for(i = 0; i < 100; i++)
	{
	    state = readl(host->iobase + SD_STATE_OFFSET);
	    
	    if ( ((state & SD_STATE_DAT0S) == 0) && ((state & SD_STATE_CMDS) == 0) )
	    {
	    	spin_unlock_irqrestore(&host->lock, flags);
	    	return 1;
	    }
		
		udelay(100);
	}
	
	spin_unlock_irqrestore(&host->lock, flags);
	return 0;
}

static irqreturn_t irq_handler(int irqnum, void * param)
{
    struct sdiohost * host = param;
    unsigned long flags;
    u32 state;
    
    spin_lock_irqsave(&host->lock, flags);

    state = readl(host->iobase + SD_STATE_OFFSET);
    
	// clear IRQ pending bit
	writel(state, host->iobase + SD_STATE_OFFSET); 
	
	spin_unlock_irqrestore(&host->lock, flags);
	
	if ((state & SD_STATE_TEIE) && (state & SD_STATE_TEI)) {
		complete(&host->sdc_complete);
	}
	
	if ((state & SD_STATE_SDIOA_EN) && (state & SD_STATE_SDIOA_P))
	{
		if (host->sdio_irq_enabled)
		{
			mmc_signal_sdio_irq(host->mmc);
			
			INFO_MSG(host->mmc->parent, "sdio irq received\n");
		}
	}
	
    return IRQ_HANDLED;
}

static void sdiohost_dma_complete(void * dma_async_param)
{
	struct sdiohost * host = (struct sdiohost *) dma_async_param;
	
	complete(&host->dma_complete);
}

static int sdiohost_dma_prepare_data(struct mmc_host * mmc, struct mmc_data *data)
{
    enum dma_transfer_direction slave_dirn;
	u32 sglen;
	
	struct sdiohost * host = mmc_priv(mmc);
	
    if (data->flags & MMC_DATA_READ)
	{
		host->dma_dir = DMA_FROM_DEVICE;
		host->dma_conf.direction = slave_dirn = DMA_DEV_TO_MEM;
	}
	else
	{
		host->dma_dir = DMA_TO_DEVICE;
		host->dma_conf.direction = slave_dirn = DMA_MEM_TO_DEV;
	}
	
	sglen = dma_map_sg(host->dma->device->dev, data->sg, data->sg_len, host->dma_dir);
	
	host->dma_slave.dma_dev = host->dma->device->dev;
	host->dma_slave.trans_type = SLAVE;
	
	if (data->flags & MMC_DATA_READ) {
	    host->dma_slave.mode = host->dma_rmode;
	}
	else {
		host->dma_slave.mode = host->dma_wmode;
	}
	
	host->dma->private = (void *) &host->dma_slave;
	
	if (dmaengine_slave_config(host->dma, &host->dma_conf))
	{
		ERR_MSG(mmc->parent, "failed to configure DMA channel\n");
		return -EINVAL;
	}

	host->desc = dmaengine_prep_slave_sg(host->dma, data->sg, sglen, slave_dirn, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		
	if (!host->desc)
	{
		ERR_MSG(mmc->parent, "dmaengine_prep_slave_sg failed\n");
		return -EBUSY;
	}

	host->desc->callback = sdiohost_dma_complete;
	host->desc->callback_param = host;
	
	return 0;
}

static void sdiohost_snd_rcv(struct mmc_host * mmc, struct mmc_command *cmd, 
                            int * err_cmd, int * err_data, struct mmc_data *data)
{
	u32 state, enable, mode;
	u32 tmode;
	u32 tmp[2];
	u32 total, blksz, blocks;
	bool write;
	
	u32 opcode, arg;
    u32 * resp;
    u32 resp_len;
    bool crc, busy;
    
    struct sdiohost * host = mmc_priv(mmc);
    
    resp_len = 0;
    busy = false;
    
    crc = (cmd->flags & MMC_RSP_CRC) ? true : false;
    
    if (cmd->flags & MMC_RSP_PRESENT)
    {
        if (cmd->flags & MMC_RSP_136) {
	        resp_len = 4;
	    }
	    else
	    {
	        resp_len = 2;
	        busy = (cmd->flags & MMC_RSP_BUSY) ? true : false;
	    }
    }
    
    opcode = cmd->opcode;
    arg    = cmd->arg;
    resp   = cmd->resp;
	
	total = blksz = blocks = 0;
	
	if (data)
	{
		blksz  = data->blksz;
        blocks = data->blocks;
		total  = blksz * blocks;
		
		write = (data->flags & MMC_DATA_WRITE) ? true : false;
	}
	
	if (total != 0)
	{
	    if (write) { 
	        tmode = 5; // Send to device
	    }
	    else { 
	        tmode = 4; // Receive from device
	    }
	}
	else if (resp_len != 0 && resp != NULL)
	{
	    if (resp_len >= 4) { // 17 bytes response
	        tmode = 2;
	    }
		else if (busy) { // 6 bytes response with busy
		    tmode = 3;
		}
		else { // 6 bytes response
		    tmode = 1;
		}
	}
	else {
	    tmode = 0;
	}
	
	state = readl(host->iobase + SD_STATE_OFFSET);
	smp_rmb();
	
	state &= ~SD_STATE_TEIE; // Disable transfer end IRQ
	state |= SD_STATE_TEI; // Set TEI to clear any pending IRQ
	
    writel(state, host->iobase + SD_STATE_OFFSET);
    smp_wmb();
    
    mode = readl(host->iobase + SD_CTL_OFFSET);
	enable = readl(host->iobase + SD_EN_OFFSET);
	smp_rmb();
	
	if (total != 0) {
	    enable |= SD_EN_BSEL; // use DMA channel
	}
	else {
	    enable &= ~SD_EN_BSEL; // do not use DMA channel
	}
	
	mode |= SD_CTL_LBE; // send 8 clocks after transmission
    
    if (tmode >= 3)
    {
        mode |= SD_CTL_TOUTEN; // enable hardware timeout
        mode &= ~(0x7f << 24);
        mode |= SD_CTL_TOUTCNT(0x64);
    }
    else {
        mode &= ~SD_CTL_TOUTEN; // disable hardware timeout
    }
    
    if (crc && resp_len != 0 && resp != NULL) {
        mode &= ~SD_CTL_C7EN; // enable hardware crc check
    }
    else {
        mode |= SD_CTL_C7EN; // disable hardware crc check
    }
    
    mode &= ~(0xf << 0);
    mode |= SD_CTL_TM(tmode);
    
    writel(mode, host->iobase + SD_CTL_OFFSET);
    writel(enable, host->iobase + SD_EN_OFFSET);
    
    if (total != 0)
    {
        writel(blksz, host->iobase + SD_BLK_SIZE_OFFSET);
        writel(blocks, host->iobase + SD_BLK_NUM_OFFSET);
        
        if (total < 512)
        {
        	writel(total, host->iobase + SD_BUF_SIZE_OFFSET);
        }
        else
        {
        	writel(512, host->iobase + SD_BUF_SIZE_OFFSET);
        }
    }
    
	smp_wmb();
	
	writel(arg, host->iobase + SD_ARG_OFFSET);
	smp_wmb();
	
	writel(opcode, host->iobase + SD_CMD_OFFSET);
	smp_wmb();
	
	reinit_completion(&host->sdc_complete);
	
	if (total != 0)
	{
	    reinit_completion(&host->dma_complete);
	    dmaengine_submit(host->desc);
		dma_async_issue_pending(host->dma);
	}
	
	state = readl(host->iobase + SD_STATE_OFFSET);
	mode = readl(host->iobase + SD_CTL_OFFSET);
	smp_rmb();
	
	state |= SD_STATE_TEIE; // Enable transfer end IRQ
    mode |= SD_CTL_TS; // start transfer
    
    writel(state, host->iobase + SD_STATE_OFFSET);
    writel(mode, host->iobase + SD_CTL_OFFSET); 
    smp_wmb();
    
    wait_for_completion_timeout(&host->sdc_complete, msecs_to_jiffies(2000));
    
    if (total != 0 && wait_for_completion_timeout(&host->dma_complete, msecs_to_jiffies(2000)) == 0)
    {
        ERR_MSG(mmc->parent, "CMD%u ARG = 0x%x - dma operation timeout\n", opcode, arg);
        sdc_reset_hardware_state_mach(mmc);
        dmaengine_terminate_all(host->dma);
        
        *err_cmd  = -ETIMEDOUT;
        *err_data = -ETIMEDOUT;
        
        return;
    }
    
    state = readl(host->iobase + SD_STATE_OFFSET);
    smp_rmb();
    
    if (!(state & SD_STATE_CLC))
    {
        ERR_MSG(mmc->parent, "CMD%u ARG = 0x%x - transmission timeout\n", opcode, arg);
        sdc_reset_hardware_state_mach(mmc);
        
        *err_cmd  = -ETIMEDOUT;
        *err_data = -ETIMEDOUT;
        
        return;
    }
    
    if (resp_len != 0 && resp != NULL)
    {
        if (state & SD_STATE_CLNR)
        {
        	INFO_MSG(mmc->parent, "CMD%u ARG = 0x%x - no response\n", opcode, arg);
            sdc_reset_hardware_state_mach(mmc);
            
            *err_cmd  = -EILSEQ;
            *err_data = -EILSEQ;
            
            return;
        }
        
        if (crc && state & SD_STATE_CRC7ER)
        {
            INFO_MSG(mmc->parent, "CMD%u ARG = 0x%x - response crc error\n", opcode, arg);
            sdc_reset_hardware_state_mach(mmc);
            
            *err_cmd  = -EILSEQ;
            *err_data = -EILSEQ;
            
            return;
        }
        
        if (resp_len >= 4)
        {
            resp[3] = readl(host->iobase + SD_RSPBUF0_OFFSET);	
			resp[2] = readl(host->iobase + SD_RSPBUF1_OFFSET);	
			resp[1] = readl(host->iobase + SD_RSPBUF2_OFFSET);	
			resp[0] = readl(host->iobase + SD_RSPBUF3_OFFSET);
        }
        else
        {
        
            tmp[0] = readl(host->iobase + SD_RSPBUF0_OFFSET);
            tmp[1] = readl(host->iobase + SD_RSPBUF1_OFFSET);
        
            resp[1] = tmp[1] >> 8;
            resp[0] = tmp[1] << 24 | tmp[0] >> 8;
        }
    }
    
    *err_cmd = 0;
    
    if (total != 0)
	{
	    if (write)
	    { 
	        if (state & SD_STATE_WC16ER)
	        {
	            INFO_MSG(mmc->parent, "CMD%u ARG = 0x%x - write data crc error\n", opcode, arg);
                sdc_reset_hardware_state_mach(mmc);
                
                *err_data = -EILSEQ;
                
                return;
	        }
	    }
	    else
	    {
	        if (state & SD_STATE_RC16ER)
	        {
	            INFO_MSG(mmc->parent, "CMD%u ARG = 0x%x - read data crc error\n", opcode, arg);
                sdc_reset_hardware_state_mach(mmc);
                
                *err_data = -EILSEQ;
                
                return;
	        }
	    }
	}
	
	*err_data = 0;
}

static void sdiohost_request(struct mmc_host * mmc, struct mmc_request * mrq)
{
    int total, err_cmd, err_data;
    
    struct sdiohost * host = mmc_priv(mmc);
    
    total = 0;

    if (mrq->data)
    {
    	if (!mrq->cmd) 
    	{
    		ERR_MSG(mmc->parent, "data transfer only request not supported\n");
    		mrq->data->error = -EINVAL;
    		mmc_request_done(mmc, mrq);
    		return;
    	}
    	
        if (sdiohost_dma_prepare_data(mmc, mrq->data) < 0)
        {
        	ERR_MSG(mmc->parent, "dma prepare data error\n");
        	mrq->data->error = -EINVAL;
            mmc_request_done(mmc, mrq);
            return;
        }
        
        total = mrq->data->blksz * mrq->data->blocks;
    }
    
    sdiohost_snd_rcv(mmc, mrq->cmd, &err_cmd, &err_data, mrq->data);
    
    mrq->cmd->error = err_cmd;
    
    if (mrq->data)
    {
        mrq->data->error = err_data;
        dma_unmap_sg(host->dma->device->dev, mrq->data->sg, mrq->data->sg_len, host->dma_dir);
    }
    
    if (total != 0 && err_data == 0)
    {
        mrq->data->bytes_xfered = total;
        
        if (mrq->stop)
        {
        	sdiohost_snd_rcv(mmc, mrq->stop, &err_cmd, &err_data, NULL);
        	mrq->stop->error = err_cmd;
        }
    }

    mmc_request_done(mmc, mrq);
}

static int controller_set_voltage(struct mmc_host * mmc, u32 voltage)
{
	struct sdiohost * host = mmc_priv(mmc);
    u32 enable, mode, state;
    int i;

    if (host->power_mode == MMC_POWER_OFF) {
        return 0;
    }
    
    if (voltage == host->signal_voltage) {
		return 0;
	}
	
	if (voltage == MMC_SIGNAL_VOLTAGE_120)
	{
		ERR_MSG(mmc->parent, "voltage not supported\n");
		return -1;
	}
	
	if (voltage == MMC_SIGNAL_VOLTAGE_180 && host->low_voltage_support == false)
	{
		ERR_MSG(mmc->parent, "voltage not supported\n");
		return -1;
	}
    
    if (host->clock != 0)
    {
    	mode = readl(host->iobase + SD_CTL_OFFSET);
    	smp_rmb();
    
    	mode &= ~SD_CTL_SCC;
    	
    	writel(mode, host->iobase + SD_CTL_OFFSET);
    	smp_wmb();
    }
    
    mdelay(10);
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    smp_rmb();
    
    if (voltage == MMC_SIGNAL_VOLTAGE_330) {
    	enable &= ~SD_EN_S18EN;
    }
    else {
    	enable |= SD_EN_S18EN;
    }
    
    writel(enable, host->iobase + SD_EN_OFFSET);
    smp_wmb();
    
    mdelay(20);
    
    if (host->clock != 0)
    {
    	mode = readl(host->iobase + SD_CTL_OFFSET);
    	smp_rmb();
    
    	mode |= SD_CTL_SCC;
    	
    	writel(mode, host->iobase + SD_CTL_OFFSET);
    	smp_wmb();
    }
    
    for (i = 0; i < 100; i++)
	{
		state = readl(host->iobase + SD_STATE_OFFSET);
		smp_rmb();
			
		if ((state & SD_STATE_CMDS) && (state & SD_STATE_DAT0S)) {
			break;
		}
			
		udelay(100);
	}
	
	if (i >= 100)
	{
		ERR_MSG(mmc->parent, "could not change voltage\n");
		return -1;
	}
	
	if (voltage == MMC_SIGNAL_VOLTAGE_330) {
    	INFO_MSG(mmc->parent, "voltage set to 3.3V\n");
    }
    else {
    	INFO_MSG(mmc->parent, "voltage set to 1.8V\n");
    }
    
    host->signal_voltage = voltage;
	return 0;
}

static int sdiohost_start_signal_voltage_switch(struct mmc_host * mmc, struct mmc_ios * ios)
{	
	return controller_set_voltage(mmc, ios->signal_voltage);
}

static void controller_set_ddr(struct mmc_host * mmc, bool activate)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;

    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    if (activate == host->ddr_enabled) {
        return;
    }
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    smp_rmb();
    
    enable &= ~SD_EN_DDREN;
    
    if (activate)
    {
        enable |= SD_EN_DDREN;
        INFO_MSG(mmc->parent, "ddr mode enabled\n");
    }
    else {
        INFO_MSG(mmc->parent, "ddr mode disabled\n");
    }
    
    writel(enable, host->iobase + SD_EN_OFFSET);
    smp_wmb();
    
    host->ddr_enabled = activate;
}

static void sdiohost_set_ios(struct mmc_host * mmc, struct mmc_ios * ios)
{    
    switch(ios->power_mode)
	{
	case MMC_POWER_UP:
	    controller_power_up(mmc);
	    break;
	    
	case MMC_POWER_ON:
	    controller_power_on(mmc);
	    break;
	
	case MMC_POWER_OFF:
	    controller_power_off(mmc);
	    break;
	}
	
	controller_set_clock(mmc, ios->clock);
	
	controller_set_bus_width(mmc, ios->bus_width);
	
	controller_chip_select(mmc, ios->chip_select, true);
	
	if (ios->timing == MMC_TIMING_UHS_DDR50 || ios->timing == MMC_TIMING_MMC_DDR52) {
	    controller_set_ddr(mmc, true);
	}
	else {
	    controller_set_ddr(mmc, false);
	}
}

static void sdiohost_enable_sdio_irq(struct mmc_host * mmc, int enable)
{
	struct sdiohost * host = mmc_priv(mmc);
	unsigned long flags;
	
	spin_lock_irqsave(&host->lock, flags);
	
	if (enable) {
		controller_enable_sdio_irq(mmc);
	}
	else {
		controller_disable_sdio_irq(mmc);
	}
	
	spin_unlock_irqrestore(&host->lock, flags);
}

static const struct mmc_host_ops sdiohost_ops =
{
    .request = sdiohost_request,
    .set_ios = sdiohost_set_ios,
	.card_busy = sdiohost_card_busy,
	.start_signal_voltage_switch = sdiohost_start_signal_voltage_switch,
	.enable_sdio_irq = sdiohost_enable_sdio_irq,
};

enum
{
	DEV_SDCARD = 0,
	DEV_SDIO,
	DEV_EMMC,
};

static int sdiohost_init(struct sdiohost * host, struct platform_device * pdev)
{
    struct mmc_host * mmc = host->mmc;
    struct device * parent;
    struct resource * res;
    dma_cap_mask_t mask;
    char buffer[32];
    const char * pm;
    int ret, dev_type;
    const char *out;
    
    parent = mmc->parent;
    
    spin_lock_init(&host->lock);
    
    init_completion(&host->dma_complete);
    init_completion(&host->sdc_complete);
    
	host->clock          = 0;
	host->actual_clock   = 0;
    host->power_mode     = MMC_POWER_UNDEFINED;
    host->chip_select    = MMC_CS_DONTCARE;
    host->bus_width      = MMC_BUS_WIDTH_1;
    host->signal_voltage = MMC_SIGNAL_VOLTAGE_330;
    host->card_en_gpio = -1;
    host->card_pw_gpio = -1;
    host->card_dt_gpio = -1;
    host->sdio_irq_enabled = false;
    host->sdc_enabled = false;
    host->ddr_enabled = false;
    
    ret = of_property_read_string(parent->of_node, "device_type", &out);
    
    if (ret < 0)
    {
    	ERR_MSG(parent, "could not get device type from DTS\n");
		goto out;
    }
    
    if (strcmp(out, "sdcard") == 0) {
    	dev_type = DEV_SDCARD;
    }
    else if (strcmp(out, "emmc") == 0) {
    	dev_type = DEV_EMMC;
    }
    else if (strcmp(out, "sdio") == 0) {
    	dev_type = DEV_SDIO;
    }
    else
    {
    	ERR_MSG(parent, "unrecognized device type\n");
    	ret = -EINVAL;
		goto out;
    }
    
    mmc->ocr_avail  = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30;
    mmc->ocr_avail |= MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33;
    mmc->ocr_avail |= MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36;
    
    mmc->ocr_avail_sdio = mmc->ocr_avail;
	mmc->ocr_avail_mmc  = mmc->ocr_avail;
	mmc->ocr_avail_sd   = mmc->ocr_avail;
    
    mmc->f_min = 187500;
    mmc->f_max = 52000000;
    
    mmc->max_seg_size  = 256 * 512;
    mmc->max_segs      = 128;
    mmc->max_req_size  = 512 * 256;
    mmc->max_blk_size  = 512;
    mmc->max_blk_count = 256;
    
    mmc->caps  = MMC_CAP_4_BIT_DATA;
    mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50;
    mmc->caps2 = MMC_CAP2_NO_WRITE_PROTECT;
    
    switch (dev_type)
    {
    case DEV_SDCARD:
    	mmc->caps  |= MMC_CAP_ERASE | MMC_CAP_SD_HIGHSPEED;
    	mmc->caps2 |= MMC_CAP2_NO_SDIO | MMC_CAP2_NO_MMC;
    	host->sdio_mode = false;
    	break;
    	
    case DEV_EMMC:
    	mmc->caps  |= MMC_CAP_ERASE | MMC_CAP_MMC_HIGHSPEED;
    	mmc->caps  |= MMC_CAP_NONREMOVABLE;
    	mmc->caps2 |= MMC_CAP2_BOOTPART_NOACC;
    	mmc->caps2 |= MMC_CAP2_NO_SD | MMC_CAP2_NO_SDIO;
    	host->sdio_mode = false;
    	break;
    	
    case DEV_SDIO:
    	//mmc->caps  |= MMC_CAP_WAIT_WHILE_BUSY;
    	mmc->caps2 |= MMC_CAP2_NO_SD | MMC_CAP2_NO_MMC;
    	host->sdio_mode = true;
    	break;
    }
    
    /*host->low_voltage_support = of_property_read_bool(parent->of_node, "low_voltage_support");
    
    if (host->low_voltage_support)
    {
    	INFO_MSG(parent, "low voltage support enabled\n");
    	ocr_avail  |= MMC_VDD_165_195;
    	mmc->caps  |= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50 | MMC_CAP_8_BIT_DATA;
    	mmc->caps2 |= MMC_CAP2_BOOTPART_NOACC;
    }*/
    
	mmc->ops = &sdiohost_ops;
	
    host->clk_id = NULL;
    
	host->pcl = pinctrl_get_select_default(parent);
	
	if (IS_ERR(host->pcl))
	{
	    ERR_MSG(parent, "could not get default pinctrl\n");
		ret = -ENODEV;
		goto out;
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	
	if (!res)
	{
		ERR_MSG(parent, "no memory resource\n");
		ret = -ENODEV;
		goto out_put_pinctrl;
	}
	
	if (!request_mem_region(res->start, resource_size(res), pdev->name))
	{
		ERR_MSG(parent, "unable to request memory region\n");
		ret = -EBUSY;
		goto out_put_pinctrl;
	}
	
	host->iobase = ioremap(res->start, resource_size(res));
	
	if (host->iobase == NULL)
	{
		ERR_MSG(parent, "unable to remap memory region\n");
		ret = -ENXIO;
		goto out_put_pinctrl;
	}
	
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	
	host->dma = dma_request_channel(mask, NULL, NULL);
	
	if (host->dma == NULL)
	{
	    ERR_MSG(parent, "failed to request DMA channel\n");
		ret = -ENODEV;
		goto out_put_pinctrl;
	}
	
	host->physbase = res->start;
	
	host->dma_conf.src_addr       = host->physbase + SD_DAT_OFFSET;
	host->dma_conf.dst_addr       = host->physbase + SD_DAT_OFFSET;
	host->dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	host->dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	host->dma_conf.device_fc      = false;
	
	if (host->physbase & 0x4000)
	{
		host->module_id = MOD_ID_SD1;
		host->clk_id    = CLKNAME_SD1_CLK;
		host->dma_wmode = SDC1WT_DMAMODE;
		host->dma_rmode = SDC1RD_DMAMODE;
	}
	else if (host->physbase & 0x8000)
	{
		host->module_id = MOD_ID_SD2;
		host->clk_id    = CLKNAME_SD2_CLK;
		host->dma_wmode = SDC2WT_DMAMODE;
		host->dma_rmode = SDC2RD_DMAMODE;
	}
	else
	{
		host->module_id = MOD_ID_SD0;
		host->clk_id    = CLKNAME_SD0_CLK;
		host->dma_wmode = SDC0WT_DMAMODE;
		host->dma_rmode = SDC0RD_DMAMODE;
	}
	
	snprintf(buffer, sizeof(buffer), "sdc_irq_mod%u", host->module_id);
	
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	
	if (!res)
	{
		ERR_MSG(parent, "no irq resource\n");
		ret = -ENODEV;
		goto out_free_dma;
	}
	
	host->irq_num = res->start;
	
	ret = request_irq(host->irq_num, irq_handler, 0, buffer, host);
	
	if (ret < 0)
	{
		ERR_MSG(parent, "irq request failed\n");
		goto out_free_dma;
	}
	
	host->clk = clk_get(NULL, host->clk_id);
	
	if (IS_ERR(host->clk))
	{
	    ERR_MSG(parent, "could not get device clock\n");
		ret = -ENODEV;
		goto out_free_irq;
	}
	
	host->card_dt_gpio = of_get_named_gpio(parent->of_node, "card_dt_gpio", 0);
	
	if (gpio_is_valid(host->card_dt_gpio))
	{
		snprintf(buffer, sizeof(buffer), "sdc_dt_gpio_mod%u", host->module_id);
		
		ret = gpio_request(host->card_dt_gpio, buffer);
	
	    if (ret < 0)
	    {
	        ERR_MSG(parent, "could not claim card detect gpio\n");
		    goto out_free_clk;
	    }
	}
	else
	{
		INFO_MSG(parent, "card detect gpio disabled by DTS\n");
		host->card_dt_gpio = -1;
	}
	
	host->card_pw_gpio = of_get_named_gpio(parent->of_node, "card_pw_gpio", 0);
	
	if (gpio_is_valid(host->card_pw_gpio))
	{
		snprintf(buffer, sizeof(buffer), "sdc_pw_gpio_mod%u", host->module_id);
		
		ret = gpio_request(host->card_pw_gpio, buffer);
	
	    if (ret < 0)
	    {
	        ERR_MSG(parent, "could not claim card power gpio\n");
		    goto out_free_dt_gpio;
	    }
	}
	else
	{
		INFO_MSG(parent, "card power gpio disabled by DTS\n");
		host->card_pw_gpio = -1;
	}
	
	host->card_en_gpio = of_get_named_gpio(parent->of_node, "card_en_gpio", 0);
	
	if (gpio_is_valid(host->card_en_gpio))
	{
		snprintf(buffer, sizeof(buffer), "sdc_en_gpio_mod%u", host->module_id);
		
		ret = gpio_request(host->card_en_gpio, buffer);
	
	    if (ret < 0)
	    {
	        ERR_MSG(parent, "could not claim card enable gpio\n");
		    goto out_free_pw_gpio;
	    }
	}
	else
	{
		INFO_MSG(parent, "card enable gpio disabled by DTS\n");
		host->card_en_gpio = -1;
	}
	
	host->reg = NULL;
	
	if (of_find_property(parent->of_node, "card_vcc", NULL))
	{
		ret = of_property_read_string(parent->of_node, "card_vcc", &pm);
		
		if (ret < 0)
		{
			ERR_MSG(parent, "could not read card vcc power source\n");
			goto out_free_en_gpio;
		}

		host->reg = regulator_get(NULL, pm);
		
		if (IS_ERR(host->reg))
		{
			ERR_MSG(parent, "could not get card vcc power source\n");
			ret = -ENODEV;
			goto out_free_en_gpio;
		}

		regulator_enable(host->reg);
	}
	
	mmc_add_host(mmc);  
    return 0;
    

out_free_en_gpio:
	if (host->card_en_gpio >= 0) {
		gpio_free(host->card_en_gpio);
	}
	
out_free_pw_gpio:
	if (host->card_pw_gpio >= 0) {
		gpio_free(host->card_pw_gpio);
	}

out_free_dt_gpio:
	if (host->card_dt_gpio >= 0) {
		gpio_free(host->card_dt_gpio);
	}

out_free_clk:
	clk_put(host->clk);
    
out_free_irq:
    free_irq(host->irq_num, host);
    
out_free_dma:
    dma_release_channel(host->dma);
    
out_put_pinctrl:
	pinctrl_put(host->pcl);
    
out:
    return ret;
}

static int __init emmc_driver_probe(struct platform_device * pdev)
{
    struct sdiohost * host = NULL;
    struct mmc_host * mmc = NULL;
    int ret;
    
    platform_set_drvdata(pdev, NULL);
    
    mmc = mmc_alloc_host(sizeof(struct sdiohost), &pdev->dev);
    
    if (!mmc)
    {
        ERR_MSG(&pdev->dev, "host allocation failed\n");
        return -ENOMEM;
    }
    
    host = mmc_priv(mmc);
    host->mmc = mmc;
    
    ret = sdiohost_init(host, pdev);
    
    if (ret < 0)
    {
    	mmc_free_host(mmc);
        return ret;
    }
    
    platform_set_drvdata(pdev, mmc);
    return 0;
}

static int __exit emmc_driver_remove(struct platform_device *pdev)
{
	struct sdiohost * host = NULL;
	struct mmc_host * mmc = NULL;
	
	mmc = platform_get_drvdata(pdev);
	
	if (mmc)
	{
		host = mmc_priv(mmc);
		
		mmc_remove_host(mmc);
		
		if (host->card_dt_gpio >= 0) {
			gpio_free(host->card_dt_gpio);
		}
		
		if (host->card_pw_gpio >= 0) {
			gpio_free(host->card_pw_gpio);
		}
		
		if (host->card_en_gpio >= 0) {
			gpio_free(host->card_en_gpio);
		}
		
		if (host->reg)
		{
			regulator_disable(host->reg);
			regulator_put(host->reg);
		}
		
		clk_put(host->clk);
		
		free_irq(host->irq_num, host);
		
		dma_release_channel(host->dma);
		
		pinctrl_put(host->pcl);
		
		mmc_free_host(mmc);
	}
	
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id emmc_dt_match[]  = {
	{.compatible = "caninos,labrador-emmc"},
	{}
};

MODULE_DEVICE_TABLE(of, emmc_dt_match);

static struct platform_driver __refdata emmc_driver = {
	.probe = emmc_driver_probe,
	.remove = emmc_driver_remove,
	.driver = {
		.name = "caninos-labrador-emmc",
		.owner = THIS_MODULE,
		.of_match_table = emmc_dt_match,
	},
};

static int __init emmc_driver_init(void)
{
	return platform_driver_register(&emmc_driver);
}

late_initcall(emmc_driver_init);

