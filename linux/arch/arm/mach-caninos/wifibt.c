
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

typedef struct {
	int wifi_power_gpio;
	int wifi_enable_gpio;
	int bt_enable_gpio;
} wifibt_data_t;

static wifibt_data_t *global_data = NULL;

void wifibt_turn_on(void)
{
	wifibt_data_t *data = global_data;

	if (data)
	{
		gpio_set_value(data->wifi_power_gpio, 1);
		//25ms is required by wilc3000, 15ms for RTL8723BS
    	mdelay(25);
		//ch_en
    	gpio_set_value(data->wifi_enable_gpio, 1);
		mdelay(10);
		//reset_n for wilc3000, bt for RTL8723BS
		gpio_set_value(data->bt_enable_gpio, 1);
    	
    	pr_info("wifi/bt module turned on\n");
    }
}
EXPORT_SYMBOL(wifibt_turn_on);

void wifibt_turn_off(void)
{
	wifibt_data_t *data = global_data;
	
	if (data)
	{
		gpio_set_value(data->wifi_enable_gpio, 0);
    
    	gpio_set_value(data->wifi_power_gpio, 0);

		gpio_set_value(data->bt_enable_gpio, 0);
    
    	mdelay(15);
    	
    	pr_info("wifi/bt module turned off\n");
    }
}
EXPORT_SYMBOL(wifibt_turn_off);

static int __init wifibt_driver_probe(struct platform_device * pdev)
{
	wifibt_data_t *data = NULL;
	struct device *dev = &pdev->dev;
	int ret;
	
	platform_set_drvdata(pdev, NULL);
	
	data = devm_kzalloc(dev, sizeof(wifibt_data_t), GFP_KERNEL);
	
	if (!data)
	{
		dev_err(dev, "could not alloc memory\n");
		return -ENOMEM;
	}
	
	data->wifi_power_gpio = 
		of_get_named_gpio(dev->of_node, "wifi_power_gpio", 0);
	
	if (!gpio_is_valid(data->wifi_power_gpio))
	{
		dev_err(dev, "could not get wifi power gpio\n");
		return -ENODEV;
	}
	
	data->wifi_enable_gpio = 
		of_get_named_gpio(dev->of_node, "wifi_enable_gpio", 0);
	
	if (!gpio_is_valid(data->wifi_enable_gpio))
	{
		dev_err(dev, "could not get wifi enable gpio\n");
		return -ENODEV;
	}
	
	data->bt_enable_gpio = 
		of_get_named_gpio(dev->of_node, "bt_enable_gpio", 0);
	
	if (!gpio_is_valid(data->bt_enable_gpio))
	{
		dev_err(dev, "could not get bt enable gpio\n");
		return -ENODEV;
	}
	
	ret = devm_gpio_request(dev, data->wifi_power_gpio, "wifi_power");
	
	if (ret)
	{
		dev_err(dev, "could not request wifi power gpio\n");
		return ret;
	}
	
	ret = devm_gpio_request(dev, data->wifi_enable_gpio, "wifi_enable");
	
	if (ret)
	{
		dev_err(dev, "could not request wifi enable gpio\n");
		return ret;
	}
	
	ret = devm_gpio_request(dev, data->bt_enable_gpio, "bt_enable_gpio");
	
	if (ret)
	{
		dev_err(dev, "could not request bt enable gpio\n");
		return ret;
	}
	
	gpio_direction_output(data->wifi_power_gpio, 0);
    
    gpio_direction_output(data->wifi_enable_gpio, 0);
    
    gpio_direction_output(data->bt_enable_gpio, 0);
    
    mdelay(20);
    
    global_data = data;
	
	platform_set_drvdata(pdev, data);
	
	dev_info(dev, "driver probe ok\n");
	
	return 0;
}

static int __exit wifibt_driver_remove(struct platform_device *pdev)
{
	wifibt_data_t *data = platform_get_drvdata(pdev);
	
	if (data)
	{
		wifibt_turn_off();
		
		global_data = NULL;
		
    	dev_info(&pdev->dev, "driver remove ok\n");
	}
	
	return 0;
}

static const struct of_device_id wifibt_dt_match[]  = {
	{.compatible = "caninos,labrador-wifibt"},
	{}
};

MODULE_DEVICE_TABLE(of, wifibt_dt_match);

static struct platform_driver __refdata wifibt_driver = {
	.probe = wifibt_driver_probe,
	.remove = wifibt_driver_remove,
	.driver = {
		.name = "caninos-labrador-wifibt",
		.owner = THIS_MODULE,
		.of_match_table = wifibt_dt_match,
	},
};

static int __init wifibt_driver_init(void)
{
	return platform_driver_register(&wifibt_driver);
}

device_initcall(wifibt_driver_init);

