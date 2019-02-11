#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <mach/hardware.h>

static u64 owl_read_timer(struct clocksource *cs)
{
	return (u64)act_readl(T0_VAL);
}

static struct clocksource owl_clksrc = {
	.name   = "timer0",
	.rating	= 200,
	.read	= owl_read_timer,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 20,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static u64 notrace owl_read_sched_clock(void)
{
	return act_readl(T0_VAL);
}

// Clockevent device: use one-shot mode

static int owl_clk_state_oneshot(struct clock_event_device * dev)
{
    act_writel(0, T1_CTL);
	act_writel(0, T1_VAL);
	act_writel(0, T1_CMP);
    return 0;
}

static int owl_clk_state_shutdown(struct clock_event_device * dev)
{
    act_writel(0, T1_CTL);
    return 0;
}

static int owl_clkevt_next(unsigned long evt, struct clock_event_device *ev)
{
	// disable timer
	act_writel(0x0, T1_CTL);

	// writing the value has immediate effect
	act_writel(0, T1_VAL);
	act_writel(evt, T1_CMP);

	// enable timer & IRQ
	act_writel(0x6, T1_CTL);

	return 0;
}

static struct clock_event_device owl_clkevt = {
	.name		= "timer1",
	.features	= CLOCK_EVT_FEAT_ONESHOT,
	.rating	 = 200,
	.set_state_oneshot = owl_clk_state_oneshot,
	.set_state_shutdown = owl_clk_state_shutdown,
	.set_next_event	= owl_clkevt_next,
};

/*
 * IRQ Handler for timer 1 of the MTU block.
 */
static irqreturn_t owl_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evdev = dev_id;

	act_writel(1 << 0, T1_CTL); /* Interrupt clear reg */
	
	evdev->event_handler(evdev);

	return IRQ_HANDLED;
}

static struct irqaction owl_timer_irq = {
	.name		= "timer1_tick",
	.flags		= IRQF_TIMER,
	.handler	= owl_timer_interrupt,
	.dev_id		= &owl_clkevt,
};

static int __init caninos_gp_timer_init(struct device_node *np)
{
	unsigned long rate;
	int irq;

	// enable the clock of timer
	act_setl(1 << 27, CMU_DEVCLKEN1);

	rate = 24000000;

	// Timer 0 is the free running clocksource
	act_writel(0, T0_CTL);
	act_writel(0, T0_VAL);
	act_writel(0, T0_CMP);
	act_writel(4, T0_CTL);

	sched_clock_register(owl_read_sched_clock, 32, rate);
	
	clocksource_register_hz(&owl_clksrc, rate);

	// Timer 1 is used for events, fix according to rate
	act_writel(0, T1_CTL);
	act_writel(0, T1_VAL);
	act_writel(0, T1_CMP);

	irq = irq_of_parse_and_map(np, 0);
	
	setup_irq(irq, &owl_timer_irq);
	
	owl_clkevt.cpumask = cpumask_of(0);
	
	clockevents_config_and_register(&owl_clkevt, rate, 0xf, 0xffffffff);
	
	return 0;
}

TIMER_OF_DECLARE(caninos_labrador, "caninos,labrador-gpt", caninos_gp_timer_init);

