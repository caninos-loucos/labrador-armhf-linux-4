/*
    Filename: labrador-s500-thermal.c
    Module  : Caninos Labrador thermal Driver
    Author  : Igor Ruschi Andrade E Lima
    Company : LSITEC
    Date    : July 2019
*/


#include <linux/module.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/thermal.h>
#include <linux/cpu_cooling.h>
#include <linux/of.h>
#include <linux/io.h>

#define DEBUG_ON 0

/* In-kernel thermal framework related macros & definations */
#define SENSOR_NAME_LEN	sizeof("labrador-thermal")

#define MAX_COOLING_DEVICE 4

#define ACTIVE_INTERVAL 500
#define IDLE_INTERVAL 2000
#define MCELSIUS	1000

#define OWL_ZONE_COUNT	3

struct labrador_tmu_data {
	struct resource *mem;
	void __iomem *base;
	struct delayed_work thermal_work;
	struct mutex lock;
	u8 temp_error1, temp_error2;
	int temp_emu;
};



struct thermal_sensor_conf {
	char name[SENSOR_NAME_LEN];
	int (*read_temperature)(void *data);
	void *private_data;
};

struct labrador_thermal_zone {
	enum thermal_device_mode mode;
	struct thermal_zone_device *therm_dev;
	struct thermal_cooling_device *cool_dev[MAX_COOLING_DEVICE];
	unsigned int cool_dev_size;
	struct thermal_sensor_conf *sensor_conf;

};

static struct labrador_thermal_zone *th_zone;
static void labrador_unregister_thermal(void);
static int labrador_register_thermal(struct thermal_sensor_conf *sensor_conf);

/* Get mode callback functions for thermal zone */
static int labrador_get_mode(struct thermal_zone_device *thermal,
			enum thermal_device_mode *mode)
{
	if (th_zone)
		*mode = th_zone->mode;
	return 0;
}

/* Set mode callback functions for thermal zone */
static int labrador_set_mode(struct thermal_zone_device *thermal,
			enum thermal_device_mode mode)
{
	enum thermal_notify_event thermal_ev; 
	if (!th_zone->therm_dev) {
		pr_notice("thermal zone not registered\n");
		return 0;
	}

	mutex_lock(&th_zone->therm_dev->lock);

	if (mode == THERMAL_DEVICE_ENABLED){
		th_zone->therm_dev->polling_delay = IDLE_INTERVAL;
		thermal_ev = THERMAL_DEVICE_DOWN;
	}
	else{
		th_zone->therm_dev->polling_delay = 0;
		thermal_ev = THERMAL_DEVICE_UP;
	}

	mutex_unlock(&th_zone->therm_dev->lock);

	th_zone->mode = mode;
	thermal_zone_device_update(th_zone->therm_dev, thermal_ev);
	pr_info("thermal polling set for duration=%d msec\n",
				th_zone->therm_dev->polling_delay);
	return 0;
}





/* Get temperature callback functions for thermal zone */
static int labrador_get_temp(struct thermal_zone_device *thermal,
			int *temp)
{
	void *data;

	if (!th_zone->sensor_conf) {
		pr_info("Temperature sensor not initialised\n");
		return -EINVAL;
	}
	data = th_zone->sensor_conf->private_data;
	*temp = th_zone->sensor_conf->read_temperature(data);
	/* convert the temperature into millicelsius */
	*temp = *temp * MCELSIUS;
	return 0;
}


/* Operation callback functions for thermal zone */
static struct thermal_zone_device_ops labrador_dev_ops = {
	.get_mode = labrador_get_mode,
	.set_mode = labrador_set_mode,
	.get_temp = labrador_get_temp
};

/* Register with the in-kernel thermal management */
static int labrador_register_thermal(struct thermal_sensor_conf *sensor_conf)
{
	int ret;
	struct cpumask mask_val;

	if (!sensor_conf || !sensor_conf->read_temperature) {
		pr_err("Temperature sensor not initialised\n");
		return -EINVAL;
	}

	th_zone = kzalloc(sizeof(struct labrador_thermal_zone), GFP_KERNEL);
	if (!th_zone)
		return -ENOMEM;

	th_zone->sensor_conf = sensor_conf;
	cpumask_set_cpu(0, &mask_val);
	//th_zone->cool_dev[0] = cpufreq_cooling_register(&mask_val);
	//if (IS_ERR(th_zone->cool_dev[0])) {
	//	pr_err("Failed to register cpufreq cooling device\n");
	//	ret = -EINVAL;
	//	goto err_unregister;
	//}
	//th_zone->cool_dev_size++;

	th_zone->therm_dev = thermal_zone_device_register(sensor_conf->name,
			0, 0x0, NULL, &labrador_dev_ops, NULL, 0, IDLE_INTERVAL);

	if (IS_ERR(th_zone->therm_dev)) {
		pr_err("Failed to register thermal zone device\n");
		ret = PTR_ERR(th_zone->therm_dev);
		goto err_unregister;
	}
	th_zone->mode = THERMAL_DEVICE_ENABLED;

	pr_info("Labrador: Kernel Thermal management registered\n");

	return 0;

err_unregister:
	labrador_unregister_thermal();
	return ret;
}

/* Un-Register with the in-kernel thermal management */
static void labrador_unregister_thermal(void)
{
	int i;

	if (!th_zone)
		return;

	if (th_zone->therm_dev)
		thermal_zone_device_unregister(th_zone->therm_dev);

	for (i = 0; i < th_zone->cool_dev_size; i++) {
		if (th_zone->cool_dev[i])
			cpufreq_cooling_unregister(th_zone->cool_dev[i]);
	}

	kfree(th_zone);
	pr_info("Kernel Thermal management unregistered\n");
}

/*
 * Calculate a temperature value from a temperature code.
 * The unit of the temperature is degree Celsius.
 * T = (838.45*5.068/(1024*12/count+7.894)-162+offset
 */
static int offset=0;
static int code_to_temp(struct labrador_tmu_data *data, u32 temp_code)
{
	int tmp1,tmp2;
	tmp1 = 83845*5068;
	tmp2 = (1024*12*100000/temp_code)+789400;
	tmp1 = tmp1/tmp2;
	tmp1 = tmp1 - 162 + offset;
	//printk(KERN_DEBUG "temp:%d\n", tmp1);
	//printk(KERN_INFO "temp:%d\n", tmp1);
	return tmp1;
}


#define TRY_TEMP_CNT	5
static u32 labrador_tmu_temp_code_read(struct labrador_tmu_data *data)
{
	int i, j, ready;
	u32 tmp, temp_code=0, temp_arry[TRY_TEMP_CNT];
	void __iomem *TS_CTL;
	void __iomem *TS_OUT;

	TS_CTL = data->base;
	TS_OUT = data->base + 4;
	
	//printk(KERN_DEBUG "TS_CTL:%p, TS_OUT:%p\n", TS_CTL, TS_OUT);
	
	tmp = readl(TS_CTL);
	tmp = tmp & 0xfffcffff;	
	tmp = tmp | 0x00010000;		//11bit adc								
	writel(tmp, TS_CTL);
		
	writel(0x01000000, TS_OUT);		//enable alarm irq
	
	for(i=0;i<TRY_TEMP_CNT;i++) {
		tmp = readl(TS_CTL);
		tmp = tmp | 0x1;										//enable tsensor
		writel(tmp, TS_CTL);
		
		ready = 0;
		do{
			tmp = readl(TS_OUT);
			ready = tmp & 0x2000000;
		}while(ready==0);
		
		temp_arry[i] = tmp & 0xfff;
		//printk(KERN_INFO "temp_arry[%d]:%d\n", i, temp_arry[i]);
		
		tmp = readl(TS_CTL);
		tmp = tmp & 0xfffffffe;	//disable tsensor
		writel(tmp, TS_CTL);
		writel(0x03000000, TS_OUT);		//clr pending	
	}

	//sort temp arry
	for(i=0;i<TRY_TEMP_CNT-1;i++) {
		for(j=i+1; j<TRY_TEMP_CNT; j++) {
			if(temp_arry[j] < temp_arry[i]) {
				tmp = temp_arry[i];
				temp_arry[i] = temp_arry[j];
				temp_arry[j] = tmp;
			}
		}
	}

	// discard min & max, then take ther average
	for(i=1;i<TRY_TEMP_CNT-1;i++) {
		temp_code += temp_arry[i];
	}		
	temp_code = temp_code/(TRY_TEMP_CNT-2);
	//printk(KERN_INFO "temp_code:%d\n", temp_code);
	return temp_code;
}

static int labrador_tmu_read(struct labrador_tmu_data *data)
{
	int temp_code;
	int temp;

	mutex_lock(&data->lock);

	temp_code = labrador_tmu_temp_code_read(data);
	temp = code_to_temp(data, temp_code);

	mutex_unlock(&data->lock);
	
	return temp;
}

static struct thermal_sensor_conf labrador_sensor_conf = {
	.name			= "labrador-thermal",
	.read_temperature	= (int (*)(void *))labrador_tmu_read,
};


static const struct of_device_id labrador_tmu_match[] = {
	{
		.compatible = "caninos,labrador-thermal",
	},	
	{},
};
MODULE_DEVICE_TABLE(of, labrador_tmu_match);


static struct platform_device_id labrador_tmu_driver_ids[] = {
	{
		.name		= "labrador-tmu"
	},
	{ },
};
MODULE_DEVICE_TABLE(platform, labrador_tmu_driver_ids);

//worker
static void thermal_worker(struct work_struct *work)
{
	int ret = 0;
	struct labrador_tmu_data *data;

	data = (&labrador_sensor_conf)->private_data;

	ret = labrador_register_thermal(&labrador_sensor_conf);
	if (ret)
		pr_err("Failed to register thermal interface\n");
	return;
}


static int labrador_tmu_probe(struct platform_device *pdev)
{
	struct labrador_tmu_data *data;

	//int ret, i;
#if(DEBUG_ON)
	pr_info("probe init");
#endif

	data = devm_kzalloc(&pdev->dev, sizeof(struct labrador_tmu_data),
					GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "Failed to allocate driver structure\n");
		return -ENOMEM;
	}

	data->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//data->mem->start = 0xb01b00e8;
	//data->mem->end = 0xb01b00e8 + 0x8;
	printk("tmu-mem:start=0x%x\n",data->mem->start);
	data->base = devm_ioremap_resource(&pdev->dev, data->mem);
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	platform_set_drvdata(pdev, data);

	mutex_init(&data->lock);

#if(DEBUG_ON)
	pr_info("mutex init ok");
#endif

	/* Register the sensor with thermal management interface */
	(&labrador_sensor_conf)->private_data = data;

#if(DEBUG_ON)
	pr_info("interface sensor-thermal mng ok");
#endif

	INIT_DELAYED_WORK(&data->thermal_work, thermal_worker);
	schedule_delayed_work(&data->thermal_work, 500);
	return 0;
}


static int labrador_tmu_remove(struct platform_device *pdev)
{
	//labrador_tmu_control(pdev, false);

	labrador_unregister_thermal();

	platform_set_drvdata(pdev, NULL);

	return 0;
}


static struct platform_driver labrador_tmu_driver = {
	.driver = {
		.name   = "labrador-thermal",
		.owner  = THIS_MODULE,
		//.pm     = LABRADOR_TMU_PM,
		.of_match_table = of_match_ptr(labrador_tmu_match),
	},
	.probe = labrador_tmu_probe,
	.remove	= labrador_tmu_remove,
	.id_table = labrador_tmu_driver_ids,
};

module_platform_driver(labrador_tmu_driver);

MODULE_DESCRIPTION("Caninos-labrador-TMU-Driver");
MODULE_AUTHOR("LSITEC-CITI");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:labrador-thermal");
