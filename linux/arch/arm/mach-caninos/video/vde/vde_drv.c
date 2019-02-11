#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/sem.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>

#define IC_TYPE_GL5203 0x5203
#define IC_TYPE_GL5206 0x5206

#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/powergate.h>
#include <mach/clock-owl.h>

#include "vde_core.h"
#include "vde_drv.h"

#define VDE_FREQ_EXTREME 642

#define VDE_REG_BACKUP_NUM 47

#define DEVDRV_NAME_VDE "vde"

#define DEV_CLK_INTERFACE_VDE_BIT 25

#define VDE_REGISTER_BASE 0xb0280000

#define VDE_REG_BASE   VDE_REGISTER_BASE
#define CMU_REG_BASE   CMU_CONTROL_REGISTER_BASE
#define VDE_REG_STATUS ((VDE_REG_BASE) + 0x4)
#define VDE_REG_4      ((VDE_REG_BASE) + 0x10)

// 20141023 : set 5206 pll and clock source
#define VDE_FREQ_DEFAULT 			300
#define VDE_FREQ_D1      			150
#define VDE_FREQ_720P    			240
#define VDE_FREQ_1080P   			300
#define VDE_FREQ_MULTI   			480 
#define VDE_FREQ_4Kx2K   			600 

#define CLK_NAME_DEVPLL				CLKNAME_DEVPLL
#define CLK_NAME_NANDPLL			CLKNAME_NANDPLL
#define CLK_NAME_DISPLAYPLL			CLKNAME_DISPLAYPLL
#define CLK_NAME_DDRPLL				CLKNAME_DDRPLL

#define CLK_NAME_DEVCLK				CLKNAME_DEV_CLK
#define CLK_NAME_VDE_CLK			CLKNAME_VDE_CLK

void __iomem *Share_Mem_REG;

/*
 SPS_PG_CTL,b0 VDE power on enable
 VDE_CMU_VDE_CLK, VDE clock source and division
 VDE_CMU_DEVCLKEN0, 25bit,VDE interface clock enable, switch AHB clock and VDE_CLK
 VDE_CMU_DEVRST0, 19bit, VDE control block reset
 */
unsigned int sps_base = 0;
unsigned int cmu_base = 0;
void __iomem *VDE_SPS_PG_CTL;
void __iomem *VDE_CMU_DEVRST0;
void __iomem *VDE_CMU_DEVCLKEN0;
void __iomem *VDE_CMU_VDE_CLK;
void __iomem *VDE_CMU_COREPLL;
              
typedef struct {
    unsigned int regs[VDE_REG_BACKUP_NUM];
} vde_user_data_t;

#define VDE_DEBUG_PRINTK    printk("%s %d\n",__FILE__,__LINE__)

struct asoc_vde_dev {
    struct device   *dev;
    void __iomem    *base;
    struct clk      *clk;
    int             irq;
    unsigned int    setting;
	struct regulator *power;

};
struct asoc_vde_dev *gVDE;

static void vde_write(struct asoc_vde_dev *vde, u32 val, u32 reg)
{
    void volatile *write_add = (void*)((unsigned int)vde->base + reg);
    writel_relaxed(val, write_add);
}

static u32 vde_read(struct asoc_vde_dev *vde, u32 reg)
{
    unsigned int data = 0;   
    data = readl_relaxed((void volatile *)((unsigned int)vde->base + reg));
    return data;
}

typedef enum {SLOT_IDLE, SLOT_OCCUPIED, SLOT_COMPLETED} slot_state_e;

#define VDD_0_9V     900000
#define VDD_1_0V    1000000
#define VDD_1_05V   1050000
#define VDD_1_1V    1100000
#define VDD_1_15V   1150000
#define VDD_1_2V    1200000

typedef struct slot_s{
    struct completion isr_complete;
    slot_state_e state;
    unsigned int clientRegPtr;
    unsigned int vde_status;
    unsigned int slice_mode;
    int pid;
    vde_user_data_t user_data;
} slot_t;

static int vde_cur_slot_id = -1;
static int vde_slice_slot_id = -1;
static unsigned int vde_cur_slot_num = 0;
static int vde_set_voltage_limit=0;
static int vde_last_status = 0; // 0:normal, -1:error;

static int multi_instance_mode = 0;
static int debug_enable = 0;
static int autoFreq_enable = 1;
static int vde_pre_freq = 0;
static int adjust_freq_flag = 1;
static int ic_type = IC_TYPE_GL5203;

#define VDE_DBG(...)     if(debug_enable==1) {printk(__VA_ARGS__);}
#define VDE_DBG2(...)     if(debug_enable==2) {printk(__VA_ARGS__);}

#define MAX_INSTANCE_NUM 8
static slot_t slot[MAX_INSTANCE_NUM];

static int slot_reset(int i) {   
    init_completion(&slot[i].isr_complete);
    slot[i].state = SLOT_IDLE; 
    slot[i].clientRegPtr = 0; 
    slot[i].slice_mode = 0;  
    slot[i].pid = -1; 
    if(vde_cur_slot_num > 0) 
       vde_cur_slot_num--;
    
    return 0;
}

static int slot_get(void) 
{        
    int i;
    for(i=0;i<MAX_INSTANCE_NUM;i++) {    
        if(slot[i].state == SLOT_IDLE) {    
            init_completion(&slot[i].isr_complete);   
            slot[i].state = SLOT_OCCUPIED;
            vde_cur_slot_num++;
            slot[i].pid = task_tgid_vnr(current);
            return i;
        }
    }
		
    if(i == MAX_INSTANCE_NUM){
        printk("vde : no idle slot, max %d slots\n", MAX_INSTANCE_NUM);
        return -1;
    }
    return -1;
}
struct ic_info {
	int ic_type;
};

static struct ic_info  gl5203_data = {
   .ic_type = IC_TYPE_GL5203,
};
 
static  struct ic_info gl5206_data = {
   .ic_type = IC_TYPE_GL5206,
};

static const struct of_device_id gl520x_vde_of_match[] = {

         {.compatible = "actions,atm7039c-vde", .data = &gl5203_data},

         {.compatible = "actions,atm7059a-vde", .data = &gl5206_data},

         {}
};

MODULE_DEVICE_TABLE(of, gl520x_vde_of_match);

static int slot_complete(int i, unsigned int vde_status) 
{        
    if(slot[i].state != SLOT_OCCUPIED) {
        printk("vde : slot is idle, staus error\n");
        return -1;
    }  
    
    slot[i].vde_status = vde_status;
    slot[i].state = SLOT_COMPLETED;
    return 0;
}

static int vde_irq_registered = 0;
static int vde_open_count = 0;
static unsigned long vde_cur_need_freq = 0;

static int vde_occupied = 0;

//for multi process
static struct mutex m_mutex;
static struct mutex m_freq_adjust_mutex;
static DECLARE_WAIT_QUEUE_HEAD(waitqueue);
static int vde_idle_flag = 1;
static int vde_clk_isEnable = 0;

/* calculate VDE clk's divider  */
static int vde_calculate_divider(unsigned long parent_pll, unsigned long rate)
{
	unsigned long clk_left, clk_right;
	int tmp_divider;
	if (parent_pll < rate) {
		return 1;
	}
	
	tmp_divider = parent_pll/rate;
	
	tmp_divider = (tmp_divider>0) ? tmp_divider : 1;
	clk_left = parent_pll/(tmp_divider + 1);
	clk_right = parent_pll/tmp_divider;
	if (rate == clk_right) {
		return tmp_divider;
	}
	
	if ((rate - clk_left) < (clk_right - rate)) {
		return (tmp_divider + 1);
	}else {
		return tmp_divider;
	}
}

static int vde_clk_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	struct clk_notifier_data *cnd;
	struct clk *vde_clk;
	unsigned long rate;
	unsigned long old_parent_rate, new_parent_rate;
	
	int old_divider, new_divider, old_divide_ratio, new_divide_ratio;
	int ret, pll_is_changed;
	
	if (action == PRE_RATE_CHANGE) {
			
		cnd = data;
		
		vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);
		if (vde_clk != cnd->clk) {
			printk("vde : notify_pre: vde_limit_notify: wrong vde clk value, and NOTIFY_BAD \n");
			return NOTIFY_BAD;
		}
		
		// 1. get cur parent pll or clk
		old_parent_rate = owl_get_putaway_parent_rate(vde_clk);
		new_parent_rate = owl_getparent_newrate(vde_clk);
		pll_is_changed = owl_pll_in_change();
		
		if(pll_is_changed == 0 || new_parent_rate == 0)
      return NOTIFY_OK;

		new_parent_rate = ((new_parent_rate == 0) &&(pll_is_changed == 0)) ? old_parent_rate : new_parent_rate;
			
		// 2. get divsion encoder, divider  must = 1/2/3/4(> 0), if not, owl_getdivider_index will return -1
 		old_divider = old_parent_rate/cnd->old_rate;
		old_divide_ratio = owl_getdivider_index(vde_clk, old_divider);
		new_divider = vde_calculate_divider(new_parent_rate, vde_cur_need_freq);
		new_divide_ratio = owl_getdivider_index(vde_clk, new_divider);
		
		// 3. set dicsion
		ret = owl_set_putaway_divsel(vde_clk, old_divide_ratio, new_divide_ratio);
		if (ret == -1) {
			printk("vde : notify_pre: owl_set_putaway_divsel() failed \n");
		}

		rate = new_parent_rate/new_divider;
		rate = (rate > 0) ? rate : vde_cur_need_freq;
		owl_update_notify_newrate(vde_clk, rate);
	} else if (action == POST_RATE_CHANGE) {
		cnd = data;
	}else if (action == ABORT_RATE_CHANGE) {
		cnd = data;
		printk("vde : notify_abort: ABORT_RATE_CHANGE: rate: [%ld, %ld]\n", cnd->old_rate, cnd->new_rate);
	}

	return NOTIFY_OK;
}

// only for dump jpeg reg when decoder stream error
static void vde_dump(void)
{
	int i;
	printk("vde : vde_dump In\n");
	printk("[sps_pg_ctl, 0x%08x]", readl_relaxed(VDE_SPS_PG_CTL));  //sps_pg_ctl
	printk("[cmu_devclken0, 0x%08x]", readl_relaxed(VDE_CMU_DEVCLKEN0)); //cmu_devclken0
	printk("[cmu_devrst0, 0x%08x], ", readl_relaxed(VDE_CMU_DEVRST0)); //cmu_devrst0
	printk("[vde_clk, 0x%08x] \n", readl_relaxed(VDE_CMU_VDE_CLK)); //cmu_vde_clk
	for(i=0; i < 47; i++) {
		if (i % 8 == 0) {
			printk("0x%08x: ", VDE_REG_BASE + (i<<2));
		}
		//printk("[%02d, 0x%08x], ", i, act_readl(VDE_REG_BASE + (i<<2)));
		//printk("%08x ", act_readl(VDE_REG_BASE + (i<<2)));
		printk("%08x ", vde_read(gVDE,(i<<2)));
		if (i % 8 == 7) {
			printk("\n");
		}
    }
	printk("\n");
	printk("vde : vde_dump Out\n");
}

static void vde_dump_pre(int slotId)
{
	int i;
	printk("vde : vde_dump_pre In\n");
	printk("[sps_pg_ctl, 0x%08x]", readl_relaxed(VDE_SPS_PG_CTL));  //sps_pg_ctl
	printk("[cmu_devclken0, 0x%08x]", readl_relaxed(VDE_CMU_DEVCLKEN0)); //cmu_devclken0
	printk("[cmu_devrst0, 0x%08x], ", readl_relaxed(VDE_CMU_DEVRST0)); //cmu_devrst0
	printk("[vde_clk, 0x%08x] \n", readl_relaxed(VDE_CMU_VDE_CLK)); //cmu_vde_clk
	for(i=0; i < 47; i++) {
		if (i % 8 == 0) {
			printk("0x%08x: ", slot[slotId].user_data.regs[i]);
		}
		
		printk("%08x ", slot[slotId].user_data.regs[i]);
		if (i % 8 == 7) {
			printk("\n");
		}
    	}

	printk("\n");
	printk("vde : vde_dump_pre Out\n");
}

static void vde_dump_ddr(int id)
{
	int i;
	unsigned int * v_addr = NULL;
	int addr = vde_read(gVDE, (id<<2));
	if (addr == 0) {
		VDE_DBG("vde : vde_dump_ddr, In: id = %d, addr = 0x%08x, return.\n", id, addr);
		return ;
	}
	
	VDE_DBG("vde : vde_dump_ddr, In: id = %d, addr = 0x%08x(0x%08x)\n", id, addr, addr & 0xFFFFFFFC);
	addr =  addr & 0xFFFFFFFC;
	
	v_addr = (unsigned int * )ioremap(addr, 2048);
	
	for(i=0; i < 512; i++) { // will read 2K bytes
		if (i % 8 == 0){
			printk("0x%08x: ", (unsigned int)&v_addr[i]);
		}
		//printk("(0x%08x, %08x) ", &v_addr[i], v_addr[i]);
		printk("%08x ", v_addr[i]);
		if (i % 8 == 7){
			printk("\n");
		}
    	}

	printk("\n");
	if (v_addr != NULL) {
		iounmap(v_addr);
		v_addr = NULL;
	}
	printk("vde : vde_dump_ddr Out\n");
}

static int vde_query_interval(unsigned int reg4)
{
    unsigned int mb_w = (reg4 >> 23)&0x1ff;
    unsigned int mb_h = (reg4 >> 15)&0xff;
    unsigned int r = (mb_w *mb_h * 300)/(1000 * vde_pre_freq);
    if(r < 5) {
	r = 5;
    }
    return r;
}

static void vde_waitForIdle(void)
{
   int ctr=0;

   int v = vde_read(gVDE, 0x4);//VDE_REG_STATUS
   int v4 = vde_read(gVDE, 0x10);//VDE_REG_4
 
   if((v&0x1) == 0x1) {
      VDE_DBG("vde : vde is on air\n");
      do{
        msleep(vde_query_interval(v4));
        ctr++;
        if(ctr==100){
           printk("vde : vde always busy\n");         
           break;
        }
      } while(((vde_read(gVDE, 0x4)&0x1)) == 0x1);
   }    
}

static int is_rlc_mode(unsigned int reg4)
{
    if(((reg4>>12) & 0x7) == 0x3)    
        return 0;
    return (reg4>>3)&0x1;
}

int get_mode(void)
{
    unsigned int v = act_readl(VDE_REG_4);   
    if(((v>>12)&0x7) == 0x3) {        
        return 1;
    }   
    if(is_rlc_mode(v)) {     
        return 1;
    }   
    return 0;
}


/* when decoder finished, reset parent pll(Displaypll) = 720M */
static void vde_reset_parent_pll(void)
{
	struct clk *pll_clk;
	struct clk *vde_clk;
	int ret;
	unsigned long vde_rate, pll_rate;
	
	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);
	pll_clk = clk_get_sys(NULL, (const char *)CLK_NAME_DISPLAYPLL);

	vde_rate = clk_get_rate(vde_clk);
	pll_rate = clk_get_rate(pll_clk);
	
	if ((vde_rate == VDE_FREQ_4Kx2K) && (pll_rate == VDE_FREQ_4Kx2K)) {
		VDE_DBG("vde : vde_reset_displaypll: vde: %lu displaypll: %lu\n", vde_rate, pll_rate);
		vde_waitForIdle();
		ret = clk_set_rate(pll_clk, 720000000);
		if(ret != 0) {
			printk("vde : set displaypll rate 720M,  failed!\n");
			return ;       
		}
		clk_put(pll_clk);
	}
	
	return;
}


static void vde_do_reset(void) 
{
	int ctor = 0;
	unsigned int value_assigned;  
	unsigned int vde_state = vde_read(gVDE,0x4);//VDE_REG_STATUS

	// avoid reset vde when vde is working
	while((vde_state & 0x1) == 0x1) {
		msleep(5);
		
		vde_state = vde_read(gVDE,0x4);//VDE_REG_STATUS
		if (vde_state & 0xda000) {
			printk("vde : warning, reset vde when working wrong. state: 0x%x\n", vde_state);
			break;
		}
		if (ctor > 10) {
			printk("vde : warning, reset vde when working. state: 0x%x, ctor(%d) > 10\n", vde_state, ctor);
			break;
		}
		
		ctor ++;
	}
	
	module_reset(MOD_ID_VDE);

	VDE_DBG("vde : %d checking reset .............\n", __LINE__);
	value_assigned = readl_relaxed(VDE_CMU_DEVRST0);    

	while(((value_assigned>>19) & 0x1) != 0x1) {      
		printk("vde : Fail to reset vde, DEVEN0 = 0x%x,CMU_DEVRST0 = 0x%x PG_CTL = 0x%x\n", \
						readl_relaxed(VDE_CMU_DEVCLKEN0), \
						readl_relaxed(VDE_CMU_DEVRST0), readl_relaxed(VDE_SPS_PG_CTL));

		value_assigned |= (0x1<<19);
		writel_relaxed(value_assigned, VDE_CMU_DEVRST0);
		usleep_range(50*1000, 50*1000);
		value_assigned = readl_relaxed(VDE_CMU_DEVRST0);        
	}

		 
	vde_idle_flag = 1;
	vde_occupied = 0;    
	vde_last_status = 0;
}

static void vde_reset(void)
{
    usleep_range(5*1000,5*1000);
    vde_do_reset();
} 

static void vde_power_on(void)
{
	owl_powergate_power_on(OWL_POWERGATE_VDE); 
	
	vde_reset();
}

static void vde_power_off(void)
{    
    owl_powergate_power_off(OWL_POWERGATE_VDE);
}

static void share_mem_reg_enable(void)
{
	unsigned int value,value1;
	
	value = readl_relaxed(Share_Mem_REG);
	value = value & (~0x2);	

	writel_relaxed(value,Share_Mem_REG);
	value1 = readl_relaxed(Share_Mem_REG);
}

static void share_mem_reg_disable(void)
{
	unsigned int value,value1;
	value = readl_relaxed(Share_Mem_REG);
	value = value | 0x2;
	writel_relaxed(value,Share_Mem_REG);
	value1 = readl_relaxed(Share_Mem_REG);
}

/* Enable VDE CLK */

static void vde_clk_enable(void)
{   
	struct clk *vde_clk; 

	if(vde_clk_isEnable!=0) {
		return;
	}
	
	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);
	
	if (IS_ERR(vde_clk))
	{
		printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
		return;      
	} 

	clk_prepare(vde_clk); 
	clk_enable(vde_clk); 
	clk_put(vde_clk);
	
	module_clk_enable(MOD_ID_VDE);
	
	vde_power_on();
	vde_clk_isEnable = 1;
}

static void vde_clk_disable(void)
{    
	struct clk *vde_clk; 
	
	VDE_DBG("vde : vde_clk_disable, In\n");
	if(vde_clk_isEnable == 0) {
		return;
	}

	vde_waitForIdle(); 
	
	module_clk_disable(MOD_ID_VDE);
	
	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);
	
	if (IS_ERR(vde_clk))
	{
		printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
		return;      
	} 	

	clk_disable(vde_clk);
	clk_unprepare(vde_clk);
	clk_put(vde_clk);

	vde_reset_parent_pll();
	
	vde_power_off();
	vde_clk_isEnable = 0;
	VDE_DBG("vde : vde_clk_disable, Out\n");
}

// =------==============================================================

#define MAX_VDE_REG_RETRY_TIME  5
/* enable int */
static inline void enable_vde_irq(void)
{
}
    
static inline void disable_vde_irq(void)
{    
	unsigned int v = vde_read(gVDE,0x4);//VDE_REG_STATUS 
	int c = MAX_VDE_REG_RETRY_TIME;  

	v &= ~(1<<8);
	vde_write(gVDE,v,0x4);//Wr_Reg(VDE_REG_STATUS, v)
	
	while((vde_read(gVDE,0x4)) & (0x1<<8) && c-->0){
	    	printk("vde : can not disable irq, write %x %x\n", VDE_REG_STATUS, vde_read(gVDE,0x4));
	  	vde_write(gVDE,v,0x4);//Wr_Reg(VDE_REG_STATUS, v);
	}
}

static void vde_drv_updateReg(int id)
{
    int i;
    // we never handle reg0, reg1
    for(i=1; i < VDE_REG_BACKUP_NUM; i++){
        slot[id].user_data.regs[i] = (unsigned int)(vde_read(gVDE,i*4));
    }
}

static void vde_drv_showReg(int id)
{
    if(id>=0) {
        VDE_DBG("vde : (showReg-1) 0x%08x 0x%08x 0x%08x 0x%08x\n", 
					slot[id].user_data.regs[1], slot[id].user_data.regs[2],
         			slot[id].user_data.regs[3], slot[id].user_data.regs[4]);                      
    }else{
        VDE_DBG("vde : (showReg-2) 0x%08x 0x%08x 0x%08x 0x%08x\n", 

        			(unsigned int)(vde_read(gVDE,1*4)), (unsigned int)(vde_read(gVDE,2*4)),
        			(unsigned int)(vde_read(gVDE,3*4)), (unsigned int)(vde_read(gVDE,4*4)));        
    }
}

/**
 * This function is vde ISR.
 */
irqreturn_t vde_ISR(int irq, void *dev_id)
{
	disable_vde_irq();      
	slot_complete(vde_cur_slot_id, vde_read(gVDE,0x4));//VDE_REG_STATUS
	vde_drv_updateReg(vde_cur_slot_id);
	
	{
	unsigned int s ;

	s = vde_read(gVDE,0x4);//VDE_REG_STATUS
	if(((s & (0x1<<17)) || (s & (0x1<<14))) && !(s & (0x1<<12))){        
		slot[vde_cur_slot_id].slice_mode = 1;        
		vde_slice_slot_id = vde_cur_slot_id;
		slot[vde_slice_slot_id].state = SLOT_OCCUPIED;
	}else {       
		slot[vde_cur_slot_id].slice_mode = 0; 
		vde_occupied = 0;
	}
	
	slot[vde_cur_slot_id].vde_status = vde_read(gVDE,0x4);//VDE_REG_STATUS;
	vde_idle_flag = 1;
		
	if(slot[vde_cur_slot_id].vde_status & 0xda000) { // when meet some error.
		vde_drv_showReg(-1);
		vde_last_status = -1;
	}else{
		//only if decoding normally, enable clk gating when decoder one frame.
		unsigned int r2 = vde_read(gVDE,0x8);
		r2 |= (0x1 << 10);
		vde_write(gVDE,r2,0x8);//act_writel(r2, VDE_REG_BASE+0x8);
	}
	}
	complete(&(slot[vde_cur_slot_id].isr_complete)); 	
    wake_up_interruptible(&waitqueue);
    return IRQ_HANDLED;
}

static void vde_drv_writeReg(unsigned int regNo, unsigned int value)
{       
    unsigned int value_recover;
    vde_write(gVDE,value,regNo*4); //Wr_Reg(VDE_REG_BASE+regNo*4, value);    

    value_recover = (unsigned int)(vde_read(gVDE,regNo*4));   
     
    while(value_recover != value)
    {      
        printk("vde : Fail to write reg 0x%x, (input,recover)=(0x%x, 0x%x), DEVEN0 = 0x%x,RST = 0x%x PG_CTL = 0x%x\n", \
                        regNo, value, value_recover, readl_relaxed(VDE_CMU_DEVCLKEN0), \
                        readl_relaxed(VDE_CMU_DEVRST0), readl_relaxed(VDE_SPS_PG_CTL));
        vde_write(gVDE,value,regNo*4);//Wr_Reg(VDE_REG_BASE+regNo*4, value);
        value_recover = (unsigned int)(vde_read(gVDE,regNo*4));
    }
}

static void vde_drv_flushReg(int id, void __user *v)
{
	int i, rt; 

	unsigned int value, tmpval;
	unsigned int width;
	unsigned int mode;

	rt = copy_from_user(&(slot[id].user_data.regs[0]), v, sizeof(vde_user_data_t));
	
	if(rt != 0) {
		printk("vde : Warning: copy_from_user failed, rt=%d\n",rt);
	}
	
	value = slot[id].user_data.regs[2];   
	tmpval = slot[id].user_data.regs[4];  
    width = ((tmpval >> 23) & 0x1ff) << 4;
    mode = (tmpval >> 12) & 0x7;

	value &= (~(1<<23));

	value &= (~(1<<10)); //disable clock gating
	value &= (~(0xff));
	slot[id].user_data.regs[2] = value;
	
	// we never handle reg0, reg1
	for(i=2; i < VDE_REG_BACKUP_NUM; i++) {
		vde_drv_writeReg(i, slot[id].user_data.regs[i]);
	}
}

VDE_Status_t vde_query_status(unsigned int vde_status)
{
	if(vde_status & (0x1<<12)) {        
		VDE_DBG("vde : vde status gotframe, status: 0x%x\n", vde_status);
		return VDE_STATUS_GOTFRAME;
	} else if(vde_status & (0x1<<19)) {
		VDE_DBG("vde : vde status directmv full, status: 0x%x\n", vde_status); 
		return VDE_STATUS_DIRECTMV_FULL;
	} else if(vde_status & (0x1<<18)) {
		VDE_DBG("vde : vde status timeout, status: 0x%x\n", vde_status);
		return VDE_STATUS_TIMEOUT;
	} else if(vde_status & (0x1<<17)) {
		return VDE_STATUS_JPEG_SLICE_READY;
	} else if(vde_status & (0x1<<16)) {
		VDE_DBG("vde : vde status stream error, status: 0x%x\n", vde_status);
		return VDE_STATUS_STREAM_ERROR;
	} else if(vde_status & (0x1<<15)) {
		VDE_DBG("vde : vde status aso detected, status: 0x%x\n", vde_status);
		return VDE_STATUS_ASO_DETECTED;
	} else if(vde_status & (0x1<<14)) {
		VDE_DBG("vde : vde status stream empty, status: 0x%x\n", vde_status);
		return VDE_STATUS_STREAM_EMPTY;
	} else if(vde_status & (0x1<<13)) {
		printk("vde : vde status Bus error, status: 0x%x\n", vde_status);
		return VDE_STATUS_BUS_ERROR;
	} else {        
		printk("vde : vde status unknow error, status: 0x%x\n", vde_status);
		return VDE_STATUS_UNKNOWN_ERROR;
	}
}

typedef unsigned long VDE_FREQ_T;

static int vde_set_corevdd(struct regulator *power, int voltage)
{
	if (!IS_ERR(power))
	{
    	if(regulator_set_voltage(power, voltage, INT_MAX))
    	{
			printk("cannot set corevdd to %duV !\n", voltage);
			return -EINVAL;
		}
	}
	return 0;
}

static int vde_set_parent_pll(const char* parent_name)
{
    struct clk *pre_parent;
    struct clk *cur_parent;
    struct clk *vde_clk;
    int ret;


 	cur_parent = clk_get_sys(NULL,(const char *)parent_name); //根据clk_name获取clk结构体

	if (IS_ERR(cur_parent)) {
        printk("vde : clk_get_sys(..) failed\n");
        return -1;
	}


	vde_clk = clk_get_sys(NULL,(const char *)CLK_NAME_VDE_CLK);

    if (IS_ERR(vde_clk)) {
        printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
        return -1;
    }
	pre_parent = clk_get_parent(vde_clk); 
	
	// Compare two clk sources like this??
	if(pre_parent == cur_parent) {
		//printk("vde : vde_set_parent_pll: parent pll no changed! \n");
	    return 0;
	}

	ret = clk_set_parent(vde_clk, cur_parent); 
	if(ret != 0) {
	    printk("vde : clk_set_parent failed(%d)\n", ret);
	    //fixme : now what?
	    return -1;
	}
    return 0;
}

VDE_FREQ_T vde_do_set_freq(VDE_FREQ_T new_rate)
{
	unsigned long rate;
	struct clk *vde_clk;
	int ret; 


	struct clk *pll_clk;
	long targetFreq, freq;



 	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);

    if (IS_ERR(vde_clk)){
        printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
        return 0;
    }


	targetFreq = new_rate*1000*1000;
	if (new_rate == VDE_FREQ_MULTI) {
		VDE_DBG("vde : targetFreq: %lu\n", targetFreq/30);
		VDE_DBG("CMU_DISPLAYPLL=%x,CMU_DEVPLL=%x, CMU_VDECLK=%x\n", act_readl(CMU_DISPLAYPLL), act_readl(CMU_DEVPLL),act_readl(CMU_VDECLK));
		
		pll_clk = clk_get_sys(NULL, (const char *)CLK_NAME_DISPLAYPLL);
		
		if (IS_ERR(pll_clk)){
	        printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
	        return 0;
	    }
	    
		clk_set_parent(vde_clk, pll_clk);
		VDE_DBG("vde : vde_do_set_freq: set parent pll OK!\n");
	}
	
	freq = clk_round_rate(vde_clk, targetFreq);  
	rate = clk_get_rate(vde_clk);
	
	if (rate == targetFreq) {
   	 	printk("vde : vde_do_set_freq: cur vde freq: %lu = %lu(target), no changed!\n", rate/30/1000/1000, targetFreq/30/1000/1000);    
		return (rate/(1000*1000));  
	}
	
	if (freq < targetFreq)
	{
		if (rate == freq) {
			printk("vde : vde_do_set_freq: cur vde freq: %lu = %ld(round), no changed!\n", rate/30/1000/1000,  freq/30/1000/1000); 	 
			return (rate/(1000*1000));	
		}
		ret = clk_set_rate(vde_clk, freq);   
	}
	else {
		ret = clk_set_rate(vde_clk, targetFreq);
	}
	
	VDE_DBG("CMU_DISPLAYPLL=%x,CMU_DEVPLL=%x, CMU_VDECLK=%x\n", act_readl(CMU_DISPLAYPLL), act_readl(CMU_DEVPLL),act_readl(CMU_VDECLK));
	clk_put(vde_clk);

	if(ret != 0) {
		printk("vde : clk_set_rate new_rate %dM,  failed! \n", (unsigned int)new_rate);
		//fixme : do we need handle vdd/parent?? 
		return 0;       
	}
	
    rate = clk_get_rate(vde_clk); 
    return (rate/(1000*1000));    
}

static VDE_FREQ_T vde_setFreq(VDE_FREQ_T freq_MHz) 
{   
 	unsigned long new_rate;
	
 	new_rate = freq_MHz;  

	if(new_rate>=600) { 
        /* 5206
				*D1  150M    dev_clk     4    1.05v VDD_core电压域
				*720P 240M   dev_clk     2.5  1.05v 
				*1080P 300M  dev_clk     2    1.05v 
				*multi 480M  DISPLAY_PLL 1    1.05v
				*4k 600M     dev_clk 	 1    1.2v
        */     
    	   VDE_DBG("vde : vde_setFreq: new_rate(%ld), 1.2V and devPLL \n", new_rate/30);
       if(vde_set_corevdd(gVDE->power,VDD_1_2V) != 0) {
            return 0;
        } 
		if(vde_set_parent_pll(CLK_NAME_DEVCLK) != 0) {            
				return 0;
        }        	
    }else if(new_rate>=480) { 
    		VDE_DBG("vde : vde_setFreq: new_rate(%ld), 1.05V and DISPLAYPLL \n", new_rate/30);
        if(vde_set_corevdd(gVDE->power,VDD_1_05V) != 0) {
            return 0;
        }        
        if(vde_set_parent_pll(CLK_NAME_DISPLAYPLL) != 0) { 
            return 0;
        }    	
    }else if(new_rate>=300){
    		VDE_DBG("vde : vde_setFreq: new_rate(%ld), 1.05V and devPLL \n", new_rate/30);
        if(vde_set_corevdd(gVDE->power,VDD_1_05V) != 0) {
            return 0;
        }       
        if(vde_set_parent_pll(CLK_NAME_DEVCLK) != 0) {
            return 0;
        }  
    }else{ 
    		VDE_DBG("vde : vde_setFreq: new_rate(%ld), 1.05V and devPLL \n", new_rate/30);
        if(vde_set_corevdd(gVDE->power,VDD_1_05V) != 0) {
            return 0;
        }
        if(vde_set_parent_pll(CLK_NAME_DEVCLK) != 0) {
            return 0;
        }            	
    }   
    
    return vde_do_set_freq(new_rate);   
}

static int vde_adjust_freq(void)
{
	int f, ret;
	unsigned int v; 
	unsigned int width;
	unsigned int height;
	unsigned int jpeg_mode;
	char * mode[8] = {"H264", "MPEG4", "H263", "JPEG", "VC-1", "RV89", "MPEG12", "Other"};
	VDE_DBG("vde : vde_adjust_freq()\n");
    v = vde_read(gVDE,0x10);//VDE_REG_4 
    width = ((v >> 23) & 0x1ff) << 4;
	height = ((v >> 15) & 0xff) << 4;
    jpeg_mode = (v >> 12) & 0x7;
    
    if(jpeg_mode == 0x11) {
        //jpeg_mode
        f = VDE_FREQ_1080P;
    }else {
	    if( width > (1280+1280)) {

		   f = VDE_FREQ_4Kx2K; 
       
	    }else if( width > 1280) {
	        f = VDE_FREQ_1080P;
	    } else if( width > 720) {
	        f = VDE_FREQ_720P;
	    } else {
	        f = VDE_FREQ_D1;
	    }
    }
    
    if(vde_cur_slot_num > 1 || multi_instance_mode == 1)
    {    
		VDE_DBG("vde : vde_adjust_freq: vde_cur_slot_num: %d multi_instance_mode: %d", vde_cur_slot_num, multi_instance_mode);
		
        if(f < VDE_FREQ_4Kx2K) {
            f = VDE_FREQ_4Kx2K;
        } 
    }
    
    if(autoFreq_enable==0) {
        f = VDE_FREQ_DEFAULT;                
    }
    
    if(autoFreq_enable==2) {
        f = VDE_FREQ_4Kx2K;
    }
    
    if(autoFreq_enable==3) {
        f = VDE_FREQ_MULTI;                
    }
    
    if(autoFreq_enable==4) {
        f = VDE_FREQ_EXTREME;                
    }

	if(vde_pre_freq != f)
	{
		vde_cur_need_freq = (unsigned long)f * 1000000;
		
    	// set frequency
    	ret = vde_setFreq(f);
    	
		if(ret != 0) {
			vde_pre_freq = f; 
			VDE_DBG("vde : adjust: [w, h, f] = [%d, %d, %s], autoFreq = %d, mulIns = %d, freq.[need, real] = [%d, %d] \n", 
				width, height, mode[jpeg_mode], autoFreq_enable, multi_instance_mode, ((int)(vde_cur_need_freq/1000000))/30, ret/30);
		}else{
			printk("vde : set %dM Failed,-- %d \n", f/30, VDE_FREQ_DEFAULT);
			vde_setFreq(VDE_FREQ_DEFAULT);
			vde_pre_freq = VDE_FREQ_DEFAULT;
		}   
	}
    return 0;
}

static long vde_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{  
	int left_time, id, s, slot_id, jpeg_on_going; 

	int tag = 0; // to save delay flag for muilti instance
	int check_pll_times;
	volatile int pll_in_change = owl_pll_in_change();
	check_pll_times = 0;
	while (pll_in_change){
		if (check_pll_times > 10) {
			printk("vde : warning, asoc pll in change, check_pll_times: %d(> 2)! \n", check_pll_times);
			break;
		}
		msleep(1);
		pll_in_change = owl_pll_in_change();
		check_pll_times ++;
	}


    switch (cmd)
    {
       case VDE_RUN:    

        if(arg!=0)
        {            
            jpeg_on_going=0;
            
            mutex_lock(&m_mutex);   
			
            if((vde_read(gVDE,0x4) & 0x4000) == 0x4000) {
				printk("vde : last frame stream empty, will delay 5ms\n");
			    tag = 1;
            }

            left_time = wait_event_interruptible_timeout(waitqueue, vde_idle_flag == 1, \
                                    msecs_to_jiffies(5*1000) + 1); 
            if (unlikely(left_time == 0))  {	
	            printk("vde : VDE_RUN --- wait_event_interruptible_timeout TIMEOUT 5s\n");
                vde_reset();
	            mutex_unlock(&m_mutex);
	            return VDE_STATUS_UNKNOWN_ERROR;
	        }  
	        
            if(vde_last_status==-1) {
                vde_reset();
            }
			
            if(vde_occupied == 1) {

                if(arg != slot[vde_slice_slot_id].clientRegPtr) 
                {         
					printk("vde : VDE_RUN --- UNKNOWN_ERROR \n");          
                    mutex_unlock(&m_mutex);
                    return VDE_STATUS_UNKNOWN_ERROR;
                }else {
                    jpeg_on_going = 1;
                }
            }
            
            if(jpeg_on_going) {
               id = vde_slice_slot_id;
            } else {            

                id = slot_get();
                if(id < 0) {
                    mutex_unlock(&m_mutex);
                    return VDE_STATUS_UNKNOWN_ERROR;
                }  

            }
						

            vde_cur_slot_id = id;       
            slot[id].clientRegPtr = arg;
            vde_drv_flushReg(id, (void __user *)arg);  
            
            vde_occupied = 1;
            vde_idle_flag = 0;  
            
            mutex_lock(&m_freq_adjust_mutex);           
            if(adjust_freq_flag == 1)                   
            		vde_adjust_freq();                         
            
            // let us go
            s = 0;//act_readl(VDE_REG_STATUS);
            s |= 1;   
            enable_vde_irq();


            if(tag==1) {
               usleep_range(5000, 5000);
            }

            
            vde_write(gVDE,s,0x4);//act_writel(s, VDE_REG_STATUS);    
	    mutex_unlock(&m_freq_adjust_mutex);            

		    usleep_range(100, 100); // make sure write success


					
            mutex_unlock(&m_mutex);
            return id;
            
        } else {
           printk("vde : Fail to execute VDE RUN, maybe arg (0x%x) wrong\n", (unsigned int)arg);                     
           return VDE_STATUS_UNKNOWN_ERROR;
        }
        
        break;        
        
        case VDE_QUERY:  
        {
            slot_id = (int)arg;
            VDE_DBG("vde : --- VDE_QUERY --- 0. query NO.%d slot\n", slot_id);
            //return VDE_STATUS_GOTFRAME; //cathy added for test vde decoder  if is normal
            if (vde_irq_registered) 
            {
                VDE_Status_t s;
                mutex_lock(&m_mutex);

    	        left_time = wait_for_completion_timeout(&(slot[slot_id].isr_complete), \
    	                                                msecs_to_jiffies(5*1000) + 1);
    	        
    	        if (unlikely(left_time == 0))  {	            
		    		printk("vde : VDE_QUERY: wait timeout, VDE_STATUS_DEAD ---> vde_reset(%d, %d)\n", slot_id, vde_cur_slot_id);
    	            vde_reset();
    	            
    	            s = VDE_STATUS_DEAD;
    	        } else {
				/* normal case */	       
				int rt;     
				rt = copy_to_user((void __user *)slot[slot_id].clientRegPtr, &(slot[slot_id].user_data.regs[0]), sizeof(vde_user_data_t));    
				if(rt != 0) {
					printk("vde : VDE_QUERY: ---> Warning: copy_to_user failed, rt=%d\n",rt);
				}
				s = vde_query_status(slot[slot_id].vde_status);   
    	        }
    	         

    	        if ((slot[vde_cur_slot_id].slice_mode == 0) ||  (s == VDE_STATUS_DEAD)){
    	            slot_reset(slot_id);	        
    	        }
		   
    	        mutex_unlock(&m_mutex);
    	        return s;
    	    } else {
    	        
    	        printk("vde : should not be here\n");
    	        return -1;
    	    }
   
            break;
       }     
       case VDE_DISABLE_CLK:
            break;
        case VDE_ENABLE_CLK:
            break;
        case VDE_SET_FREQ:  
            break;
        case VDE_GET_FREQ:
            break;
        case VDE_SET_MULTI:
            mutex_lock(&m_mutex);
            if(arg>1) {
                multi_instance_mode = 1;
            }else {
                multi_instance_mode = 0;
            }
            mutex_unlock(&m_mutex);
            break;
        case VDE_DUMP:
		  	printk("vde : vde VDE_DUMP..., but do nothing\n");
            vde_drv_showReg(-1);
            break;
                        
        default:
            printk("vde : no such cmd 0x%x\n",cmd);
            return -EIO;
    }
    
	//vde_reset_in_playing();
    return 0;
    
}

static int vde_open(struct inode *inode, struct file *file)
{
    mutex_lock(&m_mutex);  
		VDE_DBG("\nvde : vde_open: In, vde_open_count: %d\n", vde_open_count);
		
    vde_open_count++;
    if (vde_open_count >1){
        VDE_DBG("vde drv already open\n");
        mutex_unlock(&m_mutex);
        return 0;
    }
    vde_set_voltage_limit=0;

    if(ic_type == IC_TYPE_GL5206){
		share_mem_reg_enable();
	}
    vde_clk_enable();
    disable_vde_irq();   

    enable_irq(gVDE->irq);


    vde_idle_flag = 1;

	VDE_DBG("vde : vde_open: Out\n");
    mutex_unlock(&m_mutex);
    return 0;
}

static void vde_clear_parent_pll(void)
{
    VDE_DBG("vde : vde release now, set to default parent pll\n");
}

static int vde_release(struct inode *inode, struct file *file)
{
    int i;
    mutex_lock(&m_mutex);
	
		VDE_DBG("vde : vde_release: In, vde_open_count: %d\n", vde_open_count); 
    vde_open_count--;

    if (vde_open_count > 0) {
        VDE_DBG("vde : vde_release: count:%d pid(%d)\n", vde_open_count, task_tgid_vnr(current));       
        vde_waitForIdle();
        
        goto VDE_REL;
    } else if (vde_open_count < 0) {
        VDE_DBG("vde : vde_release: module is closed before opened\n");
        vde_open_count = 0;        
    }


	if (vde_open_count == 0)
	{
		VDE_DBG("vde : vde_release: disable vde irq \n");       
		disable_vde_irq();
		VDE_DBG("vde : vde_release: disable IRQ_VDE\n"); 
		disable_irq(gVDE->irq);	
		VDE_DBG("vde : vde_release: disable vde irq ok!\n"); 
		
 		if(ic_type == IC_TYPE_GL5206)
 		{
 			int regulator_return = 0;
			//gVDE->power = devm_regulator_get(&pdev->dev,"corevdd");
			if (IS_ERR(gVDE->power))
				printk("can not get corevdd regulator,may be this board not need, or lost in dts!\n");
			else{
		    	if(regulator_set_voltage(gVDE->power, VDD_1_0V, INT_MAX)) {
					printk("cannot set corevdd to 1000000uV !\n");
					return -EINVAL;
				}
				regulator_return=regulator_enable(gVDE->power);
			}
			share_mem_reg_disable();
		}
	}

    vde_clear_parent_pll();            
    vde_clk_disable();
    vde_pre_freq = 0;

VDE_REL:    
	for(i=0;i<MAX_INSTANCE_NUM;i++) {
        if(slot[i].pid == task_tgid_vnr(current)){
            VDE_DBG("vde : vde slot is leak by pid(%d), reset it\n", task_tgid_vnr(current));           
            if(slot[i].slice_mode == 1 && vde_occupied == 1)
                vde_occupied = 0;
           slot_reset(i);            
        }
    }
	VDE_DBG("vde : vde_release: Out. vde_cur_slot_num: %d vde_open_count: %d\n", vde_cur_slot_num, vde_open_count); 
	mutex_unlock(&m_mutex);
    return 0;
}


static int vde_is_enable_before_suspend = 0;


static int vde_suspend (struct platform_device * dev, pm_message_t state)
{    
	printk("vde : vde_suspend: In , vde_clk_isEnable=%d, RST0=0x%x\n", vde_clk_isEnable, readl_relaxed(VDE_CMU_DEVRST0));		    
    if(vde_clk_isEnable != 0) {
       mutex_lock(&m_mutex);
       vde_waitForIdle(); 
       disable_vde_irq();
	   vde_clear_parent_pll();  
       vde_clk_disable();
       vde_is_enable_before_suspend = 1;       
       
       mutex_unlock(&m_mutex);
    }
	

    disable_irq(gVDE->irq);	



    vde_pre_freq = 0;
    printk("vde : vde_suspend: Out\n");	
    return 0;
}

static int vde_resume (struct platform_device * dev)
{
	printk(KERN_DEBUG"vde : vde_resume: In, vde_is_enable_before_suspend=%d, RST0=0x%x\n", \
            vde_is_enable_before_suspend, readl_relaxed(VDE_CMU_DEVRST0));
    if(vde_is_enable_before_suspend == 1){
		if(ic_type == IC_TYPE_GL5206){
			share_mem_reg_enable();
		}
        vde_clk_enable();
        vde_is_enable_before_suspend = 0;
    }else {

        //vde_power_off();
    }


    enable_irq(gVDE->irq);	

    printk(KERN_DEBUG"vde : vde_resume: Out\n");
    return 0;
}

static struct notifier_block nb;

static int  vde_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct asoc_vde_dev *vde;
	struct resource		*iores;
 	int ret;

 	struct clk *pll;
	struct clk *vde_clk; 
	unsigned long freq;

    dev_info(&pdev->dev, "Probe vde device\n");
    
	id = of_match_device(gl520x_vde_of_match, &pdev->dev);
	
	if(id != NULL){
		struct ic_info * info = (struct ic_info *)id->data;	
		if(info != NULL){
			ic_type = info->ic_type;
			VDE_DBG("info ic_type 0x%x \n",ic_type);
		}else{
			VDE_DBG("info is null\n");
		}
	}
	
	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!iores)
		return -EINVAL;

	vde = devm_kzalloc(&pdev->dev, sizeof(*vde), GFP_KERNEL);
	
	VDE_DBG("vde =%p\n",(void*)vde);
	
	if (!vde)
		return -ENOMEM;
    
	vde->base = devm_ioremap_resource(&pdev->dev, iores);
	
	VDE_DBG("vde->base =%p\n",vde->base);
	
	if (IS_ERR(vde->base))
		return PTR_ERR(vde->base);

	vde->irq = platform_get_irq(pdev, 0);
	
	VDE_DBG("vde->irq =%x\n",vde->irq);
	if (vde->irq < 0)
		return vde->irq;

	ret = devm_request_irq(&pdev->dev, vde->irq, (void *)vde_ISR, 0, "vde_isr", 0);
	
	if (ret)
	{
		printk("vde_drv : register vde irq failed!\n");
		return ret;
	}
	else {
		vde_irq_registered = 1;   
	}

	disable_irq(vde->irq);	
	
	vde_cur_need_freq = (unsigned long)VDE_FREQ_DEFAULT * 1000000;
	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK); 
	
	if (IS_ERR(vde_clk))
	{
		printk("VDE DRV : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
		return 0; 		 
	} 

	nb.notifier_call = vde_clk_notify;
	
	clk_notifier_register(vde_clk, &nb);
	
	ret = owl_pllsub_set_putaway(CLOCK__VDE_CLK, CLOCK__DEV_CLK);	
	
	if(ret == -1) {
		printk("vde_init: 2. owl_pllsub_set_putaway failed(%d)\n", ret);
	}

	pll = clk_get(NULL, (const char *)CLK_NAME_DEVPLL);
	
	clk_set_parent(vde_clk, pll);
	
	freq = clk_round_rate(vde_clk, vde_cur_need_freq);
	
	vde_cur_need_freq = freq;

	clk_set_rate(vde_clk, freq);
	
	if(ic_type == IC_TYPE_GL5206){
		int regulator_return = 0;
		vde->power = devm_regulator_get(&pdev->dev,"corevdd");
		if (IS_ERR(vde->power))
			printk("can not get corevdd regulator,may be this board not need, or lost in dts!\n");
		else {
	    	if(regulator_set_voltage(vde->power, VDD_1_0V, INT_MAX)) {
				printk("cannot set corevdd to 1000000uV !\n");
				return -EINVAL;
			}
			regulator_return = regulator_enable(vde->power);
		}
	}
	mutex_init(&m_mutex);
	mutex_init(&m_freq_adjust_mutex);	
	
	vde_cur_slot_num = 0;
	vde_clk_isEnable = 0;

	dev_info(&pdev->dev, "resource: iomem: %pR mapping to %p, irq %d\n",
		iores, vde->base, vde->irq);
    
  	//ioremap sps and cmu
	sps_base = (unsigned int)ioremap_nocache(0xb01b0100, 0x10);
	cmu_base = (unsigned int)ioremap_nocache(0xb0160000, 0x400);
	Share_Mem_REG = ioremap_nocache(0xb0240004,0x10);
	VDE_SPS_PG_CTL = (void __iomem *)sps_base;
	VDE_CMU_COREPLL = (void __iomem *)cmu_base;
	VDE_CMU_VDE_CLK = (void __iomem *)(cmu_base + 0x40);
	VDE_CMU_DEVRST0 = (void __iomem *)(cmu_base + 0xa8);
	VDE_CMU_DEVCLKEN0 = (void __iomem *)(cmu_base + 0xa0);

	printk(KERN_DEBUG "sps base = 0x%x\n", (unsigned int)sps_base);
	printk(KERN_DEBUG "cmu base = 0x%x\n", (unsigned int)cmu_base);
	printk(KERN_DEBUG "SPS_PG_CTL = 0x%x\n", (unsigned int)VDE_SPS_PG_CTL);
	printk(KERN_DEBUG "Share_Mem_REG = 0x%x\n", (unsigned int)Share_Mem_REG);	
	printk(KERN_DEBUG "CMU_COREPLL = 0x%x\n", (unsigned int)VDE_CMU_COREPLL);
	printk(KERN_DEBUG "CMU_VDE_CLK = 0x%x\n", (unsigned int)VDE_CMU_VDE_CLK);
	printk(KERN_DEBUG "CMU_DEVRST0 = 0x%x\n", (unsigned int)VDE_CMU_DEVRST0);
	printk(KERN_DEBUG "CMU_DEVCLKEN0 = 0x%x\n", (unsigned int)VDE_CMU_DEVCLKEN0);
	
	gVDE = vde;
	
	return 0;
}

static int vde_remove(struct platform_device *pdev)
{
	struct clk *vde_clk;	 

	vde_clk = clk_get_sys(NULL, (const char *)CLK_NAME_VDE_CLK);
	
	if (IS_ERR(vde_clk))
	{
		printk("vde : clk_get_sys(CLK_NAME_VDE_CLK, NULL) failed\n");
		return -1; 		 
	}
	
	clk_notifier_unregister(vde_clk, &nb);
	
	if(sps_base != 0){iounmap((void *)sps_base);sps_base = 0;}
	
	if(cmu_base != 0){iounmap((void *)cmu_base);cmu_base = 0;};
	
	if(Share_Mem_REG != 0){iounmap((void *)Share_Mem_REG);Share_Mem_REG = 0;}

	return 0;
}

static const struct file_operations vde_fops = 
{
    .owner = THIS_MODULE,
    .unlocked_ioctl = vde_ioctl,
    .open = vde_open,
    .release = vde_release,
};

static struct platform_driver vde_platform_driver = {
    .driver = {
        .name = DEVDRV_NAME_VDE,
        .owner = THIS_MODULE,
        .of_match_table = gl520x_vde_of_match,
    },
    .probe = vde_probe,
    .remove = vde_remove,
    .suspend = vde_suspend,
    .resume = vde_resume,
};


static struct miscdevice vde_miscdevice = {
     .minor = MISC_DYNAMIC_MINOR,
     .name = DEVDRV_NAME_VDE,
      .fops = &vde_fops,
};

#define MAX_MESSAGE_LEN	256

static int vde_clk_isEnableControl = 0;

static void message_help(void) 
{
	printk("  3 : show help info \n");
	printk("  2 : show vde reg info \n");	
	printk("  1 : enable debug info\n");
	printk("  0 : disable debug info\n");
}

static ssize_t message_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	len = sprintf(buf,"%d\n", vde_clk_isEnable);
	return len;
}

static ssize_t message_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int n = simple_strtol(buf, NULL, 10);
	
	printk("message_store: n: 0x%x count: 0x%x\n", n, count);

	switch(n) { // level -> n
		case 0:
			if(vde_clk_isEnable != 0) {
				mutex_lock(&m_mutex);
				printk("will disable\n");	
				vde_waitForIdle(); 
				disable_vde_irq();
				vde_clear_parent_pll();  
				vde_clk_disable();
				vde_clk_isEnableControl = 1;
				mutex_unlock(&m_mutex);
			}else {
				printk("has disable\n");
			}
			break;
		case 1:
			if(vde_clk_isEnableControl == 1){
				printk("will enable\n");	
 		        vde_clk_enable();
 		        vde_clk_isEnableControl = 0;
 		    }else {
 		    	printk("need not enable\n");	
 		    }
			break;
		case 2:
			vde_dump();
			vde_dump_pre(vde_cur_slot_num);
			break;
		case 3:
			message_help();
			break;
		default:
			vde_dump();
			vde_dump_ddr(n);
			break;
	}

	return count;
}

DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, message_show, message_store); //0660

static int vde_init(void)
{
	int ret;
	
    ret = misc_register(&vde_miscdevice);
    
    if (ret)
    {
        printk("register vde misc device failed!\n");
        goto err0;
    }
    
    ret = platform_driver_register(&vde_platform_driver);
    
    if (ret)
    {
        printk("register gpu platform driver error!\n");
        goto err2;
    }
    
	// create sysfs
	
	ret = device_create_file(vde_miscdevice.this_device, &dev_attr_enable);
	
	if (ret)
	{
		pr_err("%s: failed to create sysfs file", __func__);
		goto err2;
	}
	
    return 0;

err2:
	misc_deregister(&vde_miscdevice);
	
err0:
    return ret;
}

static void vde_exit(void)
{
    device_remove_file(vde_miscdevice.this_device, &dev_attr_enable);
    
	misc_deregister(&vde_miscdevice);

	platform_driver_unregister(&vde_platform_driver);
}

module_init(vde_init);
module_exit(vde_exit);

