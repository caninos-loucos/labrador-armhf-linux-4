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

extern void wifibt_turn_on(void);
extern void wifibt_turn_off(void);

struct sdiohost
{
    u32 module_id;
    const char * clk_id;
    
    struct mmc_host * mmc;
    struct pinctrl * pcl;
    struct clk * clk;
    
    spinlock_t lock;
    
    void __iomem * iobase;
    u32 physbase;
	
	struct completion dma_complete;
	struct completion sdc_complete;
	
    int irq_num;
    
    int clock;
    int actual_clock;
    u32 power_mode;
    u32 chip_select;
    u32 bus_width;
    
	bool sdio_mode;
	bool scc;
    
    enum dma_data_direction	dma_dir;
    struct dma_slave_config dma_conf;
    
    struct dma_chan	* dma;
    struct owl_dma_slave dma_slave;
    struct dma_async_tx_descriptor * desc;
    
    u32 dma_wmode, dma_rmode;
};

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
		
		return -EINVAL;
	}

	host->desc = dmaengine_prep_slave_sg
		(host->dma, data->sg, sglen, slave_dirn, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		
	if (!host->desc)
	{
		
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
        
        if (total < 512) {
        	writel(total, host->iobase + SD_BUF_SIZE_OFFSET);
        }
        else {
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
        
        sdc_reset_hardware_state_mach(mmc);
        
        *err_cmd  = -ETIMEDOUT;
        *err_data = -ETIMEDOUT;
        
        return;
    }
    
    if (resp_len != 0 && resp != NULL)
    {
        if (state & SD_STATE_CLNR)
        {
        	
            sdc_reset_hardware_state_mach(mmc);
            
            *err_cmd  = -EILSEQ;
            *err_data = -EILSEQ;
            
            return;
        }
        
        if (crc && state & SD_STATE_CRC7ER)
        {
            
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
	            
                sdc_reset_hardware_state_mach(mmc);
                
                *err_data = -EILSEQ;
                
                return;
	        }
	    }
	    else
	    {
	        if (state & SD_STATE_RC16ER)
	        {
	            
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
    		
    		mrq->data->error = -EINVAL;
    		mmc_request_done(mmc, mrq);
    		return;
    	}
    	
        if (sdiohost_dma_prepare_data(mmc, mrq->data) < 0)
        {
        	
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

static void set_timing(struct mmc_host * mmc, u32 freq)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 mode;
    
    mode = readl(host->iobase + SD_CTL_OFFSET);
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
}

static void controller_set_bus_width_chip_sel
	(struct mmc_host * mmc, u32 bus_width, u32 chip_select)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;
    
    if ((bus_width == host->bus_width) &&
    	(chip_select == host->chip_select)) {
        return;
    }
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    enable &= ~0x3;
    
    switch (bus_width)
    {
    case MMC_BUS_WIDTH_1:
        if (chip_select == MMC_CS_HIGH) {
			enable |= 0x1;
    	}
        break;
    
    case MMC_BUS_WIDTH_4:
        enable |= 0x1;
        break;
        
    case MMC_BUS_WIDTH_8:
        enable |= 0x2;
        break;
    }

    writel(enable, host->iobase + SD_EN_OFFSET);
    host->chip_select = chip_select;
    host->bus_width = bus_width;
}

static void set_actual_clock(struct mmc_host * mmc, int freq)
{
    unsigned long rate;
    struct sdiohost * host = mmc_priv(mmc);
    
    if (freq == 0) {
        return;
    }
    
    host->actual_clock = freq;
    
    rate = clk_round_rate(host->clk, freq);
    
    clk_set_rate(host->clk, rate);
    
    mdelay(1);
    
    set_timing(mmc, freq);
}

static void controller_scc(struct mmc_host * mmc, bool enable)
{
	struct sdiohost * host = mmc_priv(mmc);
    u32 mode;
    
	mode = readl(host->iobase + SD_CTL_OFFSET);
	
	mode &= ~SD_CTL_SCC;
	
	if (enable) {
    	mode |= SD_CTL_SCC;
    }
    
    writel(mode, host->iobase + SD_CTL_OFFSET);
}

static void controller_set_clock(struct mmc_host * mmc, int freq)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    if (freq == host->clock) {
        return;
    }
    
    if ((freq == 0) && (host->scc)) {
    	controller_scc(mmc, false);
    }
    else
    {
    	set_actual_clock(mmc, freq);
    	
    	if (host->scc) {
    		controller_scc(mmc, true);
    	}
    }
    
	host->clock = freq;
}

static void controller_power_off(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    if (host->power_mode == MMC_POWER_OFF) {
        return;
    }
    
    host->power_mode = MMC_POWER_OFF;
    
    host->clock          = -1;
	host->actual_clock   = -1;
    host->power_mode     = MMC_POWER_UNDEFINED;
    host->chip_select    = MMC_CS_DONTCARE;
    host->bus_width      = MMC_BUS_WIDTH_1;
    
    module_clk_disable(host->module_id);
    
    if (host->module_id == MOD_ID_SD1) {
    	wifibt_turn_off();
    }
}

static void controller_power_on(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    
    if (host->power_mode == MMC_POWER_ON) {
        return;
    }
    
    host->power_mode = MMC_POWER_ON;
}

static void controller_send_init_clk(struct mmc_host * mmc)
{
	struct sdiohost * host = mmc_priv(mmc);
	u32 mode, enable, state;
	
	// set CS to high
	enable = readl(host->iobase + SD_EN_OFFSET);
    enable &= ~0x3;
    enable |= 0x1;
    writel(enable, host->iobase + SD_EN_OFFSET);
    
    // turn on wifi/bt card
    if (host->module_id == MOD_ID_SD1) {
    	wifibt_turn_on();
    }

	// enable interrupts
	reinit_completion(&host->sdc_complete);
	
	state = readl(host->iobase + SD_STATE_OFFSET);
    state |= SD_STATE_TEIE;
    writel(state, host->iobase + SD_STATE_OFFSET);
    
    // send 80 clocks
	mode = SD_CTL_TS  | SD_CTL_TCN(5) | SD_CTL_TM(8);
	mode |= (readl(host->iobase + SD_CTL_OFFSET) & (0xff << 16));
	writel(mode, host->iobase + SD_CTL_OFFSET);
	
	if (!wait_for_completion_timeout(&host->sdc_complete, HZ)) {
		dev_err(mmc->parent, "send 80 init clock timeout\n");
	}
}

static void controller_power_up(struct mmc_host * mmc)
{
    struct sdiohost * host = mmc_priv(mmc);
    u32 enable;
    
    if (host->power_mode == MMC_POWER_UP) {
    	return;
    }
    
    host->power_mode   = MMC_POWER_UP;
    host->chip_select  = MMC_CS_HIGH;
    host->bus_width    = MMC_BUS_WIDTH_1;
    
    module_clk_enable(host->module_id);
    module_reset(host->module_id);
    
    mdelay(5);
    
    set_actual_clock(mmc, mmc->f_init);
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    enable |= SD_ENABLE;
    writel(enable, host->iobase + SD_EN_OFFSET);
    
    enable = readl(host->iobase + SD_EN_OFFSET);
    enable |= SD_EN_RESE;
    writel(enable, host->iobase + SD_EN_OFFSET);
    
    mdelay(1);
    
    controller_send_init_clk(mmc);
    
    if (host->scc) {
    	controller_scc(mmc, true);
    }
    
    if (host->sdio_mode)
    {
    	enable = readl(host->iobase + SD_EN_OFFSET);
    	enable |= SD_EN_SDIOEN;
    	writel(enable, host->iobase + SD_EN_OFFSET);
    }
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
	
	controller_set_bus_width_chip_sel(mmc, ios->bus_width, ios->chip_select);
}

static void sdiohost_enable_sdio_irq(struct mmc_host * mmc, int enable)
{
	struct sdiohost * host = mmc_priv(mmc);
	unsigned long flags;
	u32 state;
	
	spin_lock_irqsave(&host->lock, flags);
	state = readl(host->iobase + SD_STATE_OFFSET);
	
	if (enable)
	{
		state |= SD_STATE_SDIOA_EN;
		state &= ~SD_STATE_SDIOA_P;
		state &= ~SD_STATE_TEI;
	}
	else
	{
		state |= SD_STATE_SDIOA_P;
		state &= ~SD_STATE_SDIOA_EN;
		state &= ~SD_STATE_TEI;
	}
	
	writel(state, host->iobase + SD_STATE_OFFSET);
	spin_unlock_irqrestore(&host->lock, flags);
}

static irqreturn_t irq_handler(int irqnum, void * param)
{
    struct sdiohost * host = param;
    unsigned long flags;
    u32 state, temp;
    
    spin_lock_irqsave(&host->lock, flags);
    state = readl(host->iobase + SD_STATE_OFFSET);
    
    if (state & SD_STATE_TEI)
    {
    	temp = readl(host->iobase + SD_STATE_OFFSET);
    	temp &= ~(SD_STATE_SDIOA_P);
    	temp |= SD_STATE_TEI;
    	writel(temp, host->iobase + SD_STATE_OFFSET);
		complete(&host->sdc_complete);
	}
    
    spin_unlock_irqrestore(&host->lock, flags);
    
    if (host->mmc->caps & MMC_CAP_SDIO_IRQ)
    {
    	if ((state & SD_STATE_SDIOA_P) && (state & SD_STATE_SDIOA_EN)) {
			mmc_signal_sdio_irq(host->mmc);
		}
    }
	
    return IRQ_HANDLED;
}

static int sdiohost_voltage_switch(struct mmc_host * mmc, struct mmc_ios * ios)
{		
	struct sdiohost * host = mmc_priv(mmc);
	int ret = 0;
    u32 enable;
    
    switch (ios->signal_voltage)
    {
    case MMC_SIGNAL_VOLTAGE_330:
    	break;
    
    case MMC_SIGNAL_VOLTAGE_180:
		enable = readl(host->iobase + SD_EN_OFFSET);
		enable |= SD_EN_S18EN;
		writel(enable, host->iobase + SD_EN_OFFSET);
		break;
	
	default:
		ret = -EINVAL;
		break;
	}
	
	return ret;
}

static int sdiohost_card_busy(struct mmc_host * mmc)
{
	struct sdiohost * host = mmc_priv(mmc);
	u32 state;
	
	state = readl(host->iobase + SD_STATE_OFFSET);
	
	if ((state & SD_STATE_CMDS) && (state & SD_STATE_DAT0S))
	{
		return 0;
	}
	
	return 1;
}

static const struct mmc_host_ops sdiohost_ops = {
    .request = sdiohost_request,
    .set_ios = sdiohost_set_ios,
	.enable_sdio_irq = sdiohost_enable_sdio_irq,
	.start_signal_voltage_switch = sdiohost_voltage_switch,
	.card_busy = sdiohost_card_busy,
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
    int ret, dev_type;
    const char *out;
    
    parent = mmc->parent;
    
    spin_lock_init(&host->lock);
    
    init_completion(&host->dma_complete);
    init_completion(&host->sdc_complete);
    
	host->clock = -1;
	host->actual_clock = -1;
	
    host->power_mode = MMC_POWER_UNDEFINED;
    host->chip_select = MMC_CS_DONTCARE;
    host->bus_width = MMC_BUS_WIDTH_1;
    
    host->scc = false;
    host->sdio_mode = false;
    
    ret = of_property_read_string(parent->of_node, "device_type", &out);
    
    if (ret < 0)
    {
    	dev_err(parent, "could not get device type from DTS\n");
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
    	dev_err(parent, "unrecognized device type\n");
    	ret = -EINVAL;
		goto out;
    }
    
    mmc->ocr_avail  = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30;
    mmc->ocr_avail |= MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33;
    mmc->ocr_avail |= MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36;
    
    mmc->f_min = 187500;
    mmc->f_max = 52000000;
    
    mmc->max_seg_size  = 256 * 512;
    mmc->max_segs      = 128;
    mmc->max_req_size  = 512 * 256;
    mmc->max_blk_size  = 512;
    mmc->max_blk_count = 256;
    
    mmc->caps  = MMC_CAP_4_BIT_DATA | MMC_CAP_WAIT_WHILE_BUSY;
    mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED;
    mmc->caps2 = MMC_CAP2_NO_WRITE_PROTECT | MMC_CAP2_BOOTPART_NOACC;
    
    switch (dev_type)
    {
    case DEV_SDCARD:
    	mmc->caps  |= MMC_CAP_ERASE | MMC_CAP_NEEDS_POLL;
    	mmc->caps2 |= MMC_CAP2_NO_SDIO | MMC_CAP2_NO_MMC;
    	break;
    	
    case DEV_EMMC:
    	mmc->caps  |= MMC_CAP_ERASE | MMC_CAP_NONREMOVABLE;
    	mmc->caps2 |= MMC_CAP2_NO_SD | MMC_CAP2_NO_SDIO;
    	break;
    	
    case DEV_SDIO:
    	mmc->caps  |= MMC_CAP_SDIO_IRQ | MMC_CAP_NONREMOVABLE;
    	mmc->caps2 |= MMC_CAP2_NO_MMC;
    	host->sdio_mode = true;
    	host->scc = true;
    	break;
    }
    
    mmc->ocr_avail_sdio = mmc->ocr_avail;
	mmc->ocr_avail_mmc  = mmc->ocr_avail;
	mmc->ocr_avail_sd   = mmc->ocr_avail;
    
	mmc->ops = &sdiohost_ops;
	
    host->clk_id = NULL;
    
	host->pcl = pinctrl_get_select_default(parent);
	
	if (IS_ERR(host->pcl))
	{
	    dev_err(parent, "could not get default pinctrl\n");
		ret = -ENODEV;
		goto out;
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	
	if (!res)
	{
		dev_err(parent, "no memory resource\n");
		ret = -ENODEV;
		goto out_put_pinctrl;
	}
	
	if (!request_mem_region(res->start, resource_size(res), pdev->name))
	{
		dev_err(parent, "unable to request memory region\n");
		ret = -EBUSY;
		goto out_put_pinctrl;
	}
	
	host->iobase = ioremap(res->start, resource_size(res));
	
	if (host->iobase == NULL)
	{
		dev_err(parent, "unable to remap memory region\n");
		ret = -ENXIO;
		goto out_put_pinctrl;
	}
	
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	
	host->dma = dma_request_channel(mask, NULL, NULL);
	
	if (host->dma == NULL)
	{
	    dev_err(parent, "failed to request DMA channel\n");
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
		dev_err(parent, "no irq resource\n");
		ret = -ENODEV;
		goto out_free_dma;
	}
	
	host->irq_num = res->start;
	
	ret = request_irq(host->irq_num, irq_handler, 0, buffer, host);
	
	if (ret < 0)
	{
		dev_err(parent, "irq request failed\n");
		goto out_free_dma;
	}
	
	host->clk = clk_get(NULL, host->clk_id);
	
	if (IS_ERR(host->clk))
	{
	    dev_err(parent, "could not get device clock\n");
		ret = -ENODEV;
		goto out_free_irq;
	}
	
	ret = mmc_add_host(mmc);
	
	if (ret < 0)
	{
		dev_err(parent, "could not add mmc host\n");
		goto out_free_clk;
	}
	
    return 0;

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
        dev_err(&pdev->dev, "host allocation failed\n");
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

