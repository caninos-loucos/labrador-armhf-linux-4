#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>
#include <linux/timer.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/../../fs/proc/internal.h>

#include <mach/hardware.h>
#include <linux/gpio.h>

#include "aotg_hcd.h"
#include "aotg_regs.h"
#include "aotg_plat_data.h"
#include "aotg_hcd_debug.h"

char aotg_hcd_proc_sign = 'n';
unsigned int aotg_trace_onff = 0;

void aotg_dbg_proc_output_ep(void)
{
	return;
}

void aotg_dbg_output_info(void)
{
	return;
}

void aotg_dbg_put_q(struct aotg_queue *q, unsigned int num, unsigned int type, unsigned int len)
{
	return;
}

void aotg_dbg_finish_q(struct aotg_queue *q)
{
	return;
}

void aotg_dump_ep_reg(struct aotg_hcd *acthcd, int ep_index, int is_out)
{
	int index_multi = ep_index - 1;
	if (NULL == acthcd) {
		ACT_HCD_ERR
		return;
	}

	if (ep_index > 15) {
		printk("ep_index : %d too big, err!\n", ep_index);
		return;
	}	
	
	printk("=== dump hc-%s ep%d reg info ===\n",
		is_out ? "out" : "in", ep_index);

	if (ep_index == 0) {
		printk(" HCIN0BC(0x%p) : 0x%X\n",
	            acthcd->base + HCIN0BC, usb_readb(acthcd->base + HCIN0BC));
		printk(" EP0CS(0x%p) : 0x%X\n",
	            acthcd->base + EP0CS, usb_readb(acthcd->base + EP0CS));
		printk(" HCOUT0BC(0x%p) : 0x%X\n",
	            acthcd->base + HCOUT0BC, usb_readb(acthcd->base + HCOUT0BC));
		printk(" HCEP0CTRL(0x%p) : 0x%X\n",
	            acthcd->base + HCEP0CTRL, usb_readb(acthcd->base + HCEP0CTRL));
		printk(" HCIN0ERR(0x%p) : 0x%X\n",
				acthcd->base + HCIN0ERR, usb_readb(acthcd->base + HCIN0ERR));
		return;
	}
	
	if (is_out) {
		printk(" HCOUT%dBC(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1BC + index_multi * 0x8,
			usb_readw(acthcd->base + HCOUT1BC+ index_multi *0x8));	
		printk(" HCOUT%dCON(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CON + index_multi * 0x8,
			usb_readb(acthcd->base + HCOUT1CON + index_multi *0x8));
		printk(" HCOUT%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CS + index_multi * 0x8,
			usb_readb(acthcd->base + HCOUT1CS + index_multi *0x8));
		printk(" HCOUT%dCTRL(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CTRL + index_multi * 0x4,
			usb_readb(acthcd->base + HCOUT1CTRL + index_multi *0x4));
		printk(" HCOUT%dERR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1ERR + index_multi * 0x4,
			usb_readb(acthcd->base + HCOUT1ERR + index_multi *0x4));
		printk(" HCOUT%dSTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1STADDR + index_multi * 0x4,
			usb_readl(acthcd->base + HCOUT1STADDR + index_multi * 0x4));
		printk(" HCOUT%dMAXPCK(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1MAXPCK + index_multi * 0x2,
			usb_readw(acthcd->base + HCOUT1MAXPCK + index_multi * 0x2));
		
		printk(" HCOUT%dDMASTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1DMASTADDR + index_multi * 0x8,
			usb_readl(acthcd->base + HCOUT1DMASTADDR + index_multi * 0x8));
		printk(" HCOUT%dDMACOUNTER(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1DMACOUNTER + index_multi * 0x8,
			usb_readl(acthcd->base + HCOUT1DMACOUNTER + index_multi * 0x8));
	} else {
		printk(" HCIN%dBC(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1BC + index_multi * 0x8,
			usb_readw(acthcd->base + HCIN1BC + index_multi *0x8));
		printk(" HCIN%dCON(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CON+ index_multi * 0x8,
			usb_readb(acthcd->base + HCIN1CON+ index_multi *0x8));
		printk(" HCIN%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CS + index_multi * 0x8,
			usb_readb(acthcd->base + HCIN1CS + index_multi *0x8));
		printk(" HCIN%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CTRL + index_multi * 0x4,
			usb_readb(acthcd->base + HCIN1CTRL+ index_multi *0x4));
		printk(" HCIN%dERR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1ERR + index_multi * 0x4,
			usb_readb(acthcd->base + HCIN1ERR + index_multi *0x4));
		printk(" HCIN%dSTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1STADDR + index_multi * 0x4,
			usb_readl(acthcd->base + HCIN1STADDR + index_multi *0x4));
		printk(" HCIN%dMAXPCK(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1MAXPCK + index_multi * 0x2,
			usb_readw(acthcd->base + HCIN1MAXPCK + index_multi * 0x2));

		printk(" HCIN%dDMASTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1DMASTADDR + index_multi * 0x8,
			usb_readl(acthcd->base + HCIN1DMASTADDR + index_multi * 0x8));
		printk(" HCIN%dDMACOUNTER(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1DMACOUNTER + index_multi * 0x8,
			usb_readl(acthcd->base + HCIN1DMACOUNTER + index_multi * 0x8));
	}

}

void create_debug_file(struct aotg_hcd *acthcd)
{
	return;
}

void remove_debug_file(struct aotg_hcd *acthcd)
{
	return;
}

void aotg_print_xmit_cnt(char * info, int cnt)
{
	if (aotg_hcd_proc_sign == 'e') {
		printk("%s cnt:%d\n", info, cnt);
	}
}

static struct proc_dir_entry *acts_hub_pde = NULL; 

int acts_hcd_proc_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "hcd_ports_en_ctrl: %d\n", hcd_ports_en_ctrl);
	return 0;
}

static int acts_hub_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, acts_hcd_proc_show, PDE(inode)->data);
}

static ssize_t acts_hub_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char c = 'n';

	if (count) {
		if (get_user(c, buf))
			return -EFAULT;
	}
	if ((c >= '0') && (c <= '3')) { 
		hcd_ports_en_ctrl = c - '0';
		printk("hcd_hub en:%d\n", hcd_ports_en_ctrl);
	}
	if (c == 'h') {
		printk(" num ---- 0-all enable, 1-usb0 enable, 2-usb1 enable, 3-reversed. \n");
		printk("o ---- hcd_hub power on\n");
		printk("f ---- hcd_hub power off\n");
		printk("a ---- hcd_hub aotg0 add\n");
		printk("b ---- hcd_hub aotg0 remove\n");
		printk("c ---- hcd_hub aotg1 add\n");
		printk("d ---- hcd_hub aotg1 remove\n");
	}

	if (c == 'a') {
		printk("hcd_hub aotg0 add\n");
		//aotg0_device_init(0);
		aotg_hub_register(0);
	}
	if (c == 'b') {
		printk("hcd_hub aotg0 remove\n");
		//aotg0_device_exit(0);
		aotg_hub_unregister(0);
	}

	if (c == 'c') {
		printk("hcd_hub aotg1 add\n");
		//aotg1_device_init(0);
		aotg_hub_register(1);
	}
	if (c == 'd') {
		printk("hcd_hub aotg1 remove\n");
		//aotg1_device_exit(0);
		aotg_hub_unregister(1);
	}

	if (c == 'e') {
		aotg_trace_onff = 1;
	}
	if (c == 'f') {
		aotg_trace_onff = 0;
	}
	

		return count;
}

static const struct file_operations acts_hub_proc_ops = {
	.open		= acts_hub_proc_open,
	.read		= seq_read,
	.write		= acts_hub_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void create_acts_hcd_proc(void)
{
	acts_hub_pde = proc_create_data("acts_hub", S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH, NULL, &acts_hub_proc_ops, acts_hub_pde);
	return;
}

void remove_acts_hcd_proc(void)
{
	if (acts_hub_pde) {
		remove_proc_entry("acts_hub", NULL);
		acts_hub_pde = NULL;
	}
	return;
}

