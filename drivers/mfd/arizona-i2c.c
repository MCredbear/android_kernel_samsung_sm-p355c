/*
 * Arizona-i2c.c  --  Arizona I2C bus interface
 *
 * Copyright 2012 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#ifdef CONFIG_ARCH_MSM8916
#include <linux/qdsp6v2/apr.h>
#endif /* CONFIG_ARCH_MSM8916 */
#include <linux/mfd/arizona/core.h>

#include "arizona.h"

static int arizona_i2c_probe(struct i2c_client *i2c,
					  const struct i2c_device_id *id)
{
	struct arizona *arizona;
	const struct regmap_config *regmap_config;
	int ret, type;
	
#ifdef CONFIG_ARCH_MSM8916
	int modem_state;
	modem_state = apr_get_modem_state();
	if (modem_state != APR_SUBSYS_LOADED) {
		dev_info(&i2c->dev, "Modem is not loaded yet %d\n",
				modem_state);
		return -EPROBE_DEFER;
	} else 
		dev_info(&i2c->dev, "Modem is loaded %d\n", modem_state);
#endif /* CONFIG_ARCH_MSM8916 */
	
	if (i2c->dev.of_node)
		type = arizona_of_get_type(&i2c->dev);
	else
		type = id->driver_data;

	switch (type) {
#ifdef CONFIG_MFD_WM5102
	case WM5102:
		regmap_config = &wm5102_i2c_regmap;
		break;
#endif
#ifdef CONFIG_MFD_FLORIDA
	case WM8280:
	case WM5110:
		regmap_config = &florida_i2c_regmap;
		break;
#endif
#ifdef CONFIG_MFD_WM8997
	case WM8997:
		regmap_config = &wm8997_i2c_regmap;
		break;
#endif
#ifdef CONFIG_MFD_WM8998
	case WM8998:
	case WM1814:
		regmap_config = &wm8998_i2c_regmap;
		break;
#endif
	default:
		dev_err(&i2c->dev, "Unknown device type %ld\n",
			id->driver_data);
		return -EINVAL;
	}

	arizona = devm_kzalloc(&i2c->dev, sizeof(*arizona), GFP_KERNEL);
	if (arizona == NULL)
		return -ENOMEM;

	arizona->regmap = devm_regmap_init_i2c(i2c, regmap_config);
	if (IS_ERR(arizona->regmap)) {
		ret = PTR_ERR(arizona->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	arizona->type = id->driver_data;
	arizona->dev = &i2c->dev;
	arizona->irq = i2c->irq;

	return arizona_dev_init(arizona);
}

static int arizona_i2c_remove(struct i2c_client *i2c)
{
	struct arizona *arizona = dev_get_drvdata(&i2c->dev);
	arizona_dev_exit(arizona);
	return 0;
}

static const struct i2c_device_id arizona_i2c_id[] = {
	{ "wm5102", WM5102 },
	{ "wm8280", WM8280 },
	{ "wm8281", WM8280 },
	{ "wm5110", WM5110 },
	{ "wm8997", WM8997 },
	{ "wm8998", WM8998 },
	{ "wm1814", WM1814 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, arizona_i2c_id);

static struct i2c_driver arizona_i2c_driver = {
	.driver = {
		.name	= "arizona",
		.owner	= THIS_MODULE,
		.pm	= &arizona_pm_ops,
		.of_match_table	= of_match_ptr(arizona_of_match),
	},
	.probe		= arizona_i2c_probe,
	.remove		= arizona_i2c_remove,
	.id_table	= arizona_i2c_id,
};

module_i2c_driver(arizona_i2c_driver);

MODULE_DESCRIPTION("Arizona I2C bus interface");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");