/*
 *  pca953x.c - 4/8/16 bit I/O ports
 *
 *  Copyright (C) 2005 Ben Gardner <bgardner@wabtec.com>
 *  Copyright (C) 2007 Marvell International Ltd.
 *
 *  Derived from drivers/i2c/chips/pca9539.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <linux/slab.h>
/* LJH */
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/cdev.h>

#ifdef CONFIG_OF_GPIO
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#endif

#define PCA953X_INPUT		0
#define PCA953X_OUTPUT		1
#define PCA953X_INVERT		2
#define PCA953X_DIRECTION	3

#define PCA957X_IN		0
#define PCA957X_INVRT		1
#define PCA957X_BKEN		2
#define PCA957X_PUPD		3
#define PCA957X_CFG		4
#define PCA957X_OUT		5
#define PCA957X_MSK		6
#define PCA957X_INTS		7

#define PCA_GPIO_MASK		0x00FF
#define PCA_INT			0x0100
#define PCA953X_TYPE		0x1000
#define PCA957X_TYPE		0x2000

/*---LJH------begin-------*/

#define IOC_PCA1_RD 		0
#define IOC_PCA1_WR 		1
#define IOC_PCA1_SET_OUT 	8
#define IOC_PCA1_SET_IN 	3

#define IOC_PCA_BYTE_RD_1 	4
#define IOC_PCA_BYTE_WR_1 	5

#define IOC_PCA2_RD 		6
#define IOC_PCA2_WR 		7
#define IOC_PCA2_SET_OUT 	9
#define IOC_PCA2_SET_IN 	10

#define IOC_PCA_BYTE_RD_2 	11
#define IOC_PCA_BYTE_WR_2 	12

#define IOC_PCA_DBYTE_RD 	13
#define IOC_PCA_DBYTE_WD 	14

#define IOC_SET_B_OUT_1		15
#define IOC_SET_B_OUT_2		16
#define IOC_SET_B_IN_1		17
#define IOC_SET_B_IN_2		18
#define IOC_SET_DB_OUT		19
#define IOC_SET_DB_IN		20

static int major_1, major_2;
static struct class *class_1, *class_2;
//static struct cdev pca9535_dev;
static int flag = 0;
static struct pca953x_chip *chip_1, *chip_2;

struct control_info{
	int off;
	int val;
	int byte_data[8];
	int d_byte_data[16];
};
/*---LJH------begin-------*/

static const struct i2c_device_id pca953x_id[] = {
	{ "pca9534", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9535", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9536", 4  | PCA953X_TYPE, },
	{ "pca9537", 4  | PCA953X_TYPE | PCA_INT, },
	{ "pca9538", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9539", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9554", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9555", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9556", 8  | PCA953X_TYPE, },
	{ "pca9557", 8  | PCA953X_TYPE, },
	{ "pca9574", 8  | PCA957X_TYPE | PCA_INT, },
	{ "pca9575", 16 | PCA957X_TYPE | PCA_INT, },

	{ "max7310", 8  | PCA953X_TYPE, },
	{ "max7312", 16 | PCA953X_TYPE | PCA_INT, },
	{ "max7313", 16 | PCA953X_TYPE | PCA_INT, },
	{ "max7315", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca6107", 8  | PCA953X_TYPE | PCA_INT, },
	{ "tca6408", 8  | PCA953X_TYPE | PCA_INT, },
	{ "tca6416", 16 | PCA953X_TYPE | PCA_INT, },
	/* NYET:  { "tca6424", 24, }, */
	{ }
};
MODULE_DEVICE_TABLE(i2c, pca953x_id);

struct pca953x_chip {
	unsigned gpio_start;
	uint16_t reg_output;
	uint16_t reg_direction;
	struct mutex i2c_lock;

#ifdef CONFIG_GPIO_PCA953X_IRQ
	struct mutex irq_lock;
	uint16_t irq_mask;
	uint16_t irq_stat;
	uint16_t irq_trig_raise;
	uint16_t irq_trig_fall;
	int	 irq_base;
#endif

	struct i2c_client *client;
	struct pca953x_platform_data *dyn_pdata;
	struct gpio_chip gpio_chip;
	const char *const *names;
	int	chip_type;
};

static int pca953x_write_reg(struct pca953x_chip *chip, int reg, uint16_t val)
{
	int ret = 0;

	if (chip->gpio_chip.ngpio <= 8)
		ret = i2c_smbus_write_byte_data(chip->client, reg, val);
	else {
		switch (chip->chip_type) {
		case PCA953X_TYPE:
			ret = i2c_smbus_write_word_data(chip->client,
							reg << 1, val);
			break;
		case PCA957X_TYPE:
			ret = i2c_smbus_write_byte_data(chip->client, reg << 1,
							val & 0xff);
			if (ret < 0)
				break;
			ret = i2c_smbus_write_byte_data(chip->client,
							(reg << 1) + 1,
							(val & 0xff00) >> 8);
			break;
		}
	}

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed writing register\n");
		return ret;
	}

	return 0;
}

static int pca953x_read_reg(struct pca953x_chip *chip, int reg, uint16_t *val)
{
	int ret;

	if (chip->gpio_chip.ngpio <= 8)
		ret = i2c_smbus_read_byte_data(chip->client, reg);
	else
		ret = i2c_smbus_read_word_data(chip->client, reg << 1);

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed reading register\n");
		return ret;
	}

	*val = (uint16_t)ret;
	return 0;
}

static int pca953x_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip;
	uint16_t reg_val;
	int ret, offset = 0;

	chip = container_of(gc, struct pca953x_chip, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	reg_val = chip->reg_direction | (1u << off);

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_DIRECTION;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_CFG;
		break;
	}
	ret = pca953x_write_reg(chip, offset, reg_val);
	if (ret)
		goto exit;

	chip->reg_direction = reg_val;
	ret = 0;
exit:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int pca953x_gpio_direction_output(struct gpio_chip *gc,
		unsigned off, int val)
{
	struct pca953x_chip *chip;
	uint16_t reg_val;
	int ret, offset = 0;

	chip = container_of(gc, struct pca953x_chip, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	/* set output level */
	if (val)
		reg_val = chip->reg_output | (1u << off);
	else
		reg_val = chip->reg_output & ~(1u << off);

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_OUTPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_OUT;
		break;
	}
	ret = pca953x_write_reg(chip, offset, reg_val);
	if (ret)
		goto exit;

	chip->reg_output = reg_val;

	/* then direction */
	reg_val = chip->reg_direction & ~(1u << off);
	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_DIRECTION;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_CFG;
		break;
	}
	ret = pca953x_write_reg(chip, offset, reg_val);
	if (ret)
		goto exit;

	chip->reg_direction = reg_val;
	ret = 0;
exit:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int pca953x_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip;
	uint16_t reg_val;
	int ret, offset = 0;

	chip = container_of(gc, struct pca953x_chip, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_INPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_IN;
		break;
	}
	ret = pca953x_read_reg(chip, offset, &reg_val);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0) {
		/* NOTE:  diagnostic already emitted; that's all we should
		 * do unless gpio_*_value_cansleep() calls become different
		 * from their nonsleeping siblings (and report faults).
		 */
		return 0;
	}

	return (reg_val & (1u << off)) ? 1 : 0;
}

static void pca953x_gpio_set_value(struct gpio_chip *gc, unsigned off, int val)
{
	struct pca953x_chip *chip;
	uint16_t reg_val;
	int ret, offset = 0;

	chip = container_of(gc, struct pca953x_chip, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	if (val)
		reg_val = chip->reg_output | (1u << off);
	else
		reg_val = chip->reg_output & ~(1u << off);

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_OUTPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_OUT;
		break;
	}
	ret = pca953x_write_reg(chip, offset, reg_val);
	if (ret)
		goto exit;

	chip->reg_output = reg_val;
exit:
	mutex_unlock(&chip->i2c_lock);
}

static void pca953x_setup_gpio(struct pca953x_chip *chip, int gpios)
{

	static struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->direction_input  = pca953x_gpio_direction_input;
	gc->direction_output = pca953x_gpio_direction_output;
	gc->get = pca953x_gpio_get_value;
	gc->set = pca953x_gpio_set_value;
	gc->can_sleep = 1;

	gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->label = chip->client->name;
	gc->dev = &chip->client->dev;
	gc->owner = THIS_MODULE;
	gc->names = chip->names;
}

#ifdef CONFIG_GPIO_PCA953X_IRQ
static int pca953x_gpio_to_irq(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip;

	chip = container_of(gc, struct pca953x_chip, gpio_chip);
	return chip->irq_base + off;
}

static void pca953x_irq_mask(struct irq_data *d)
{
	struct pca953x_chip *chip = irq_data_get_irq_chip_data(d);

	chip->irq_mask &= ~(1 << (d->irq - chip->irq_base));
}

static void pca953x_irq_unmask(struct irq_data *d)
{
	struct pca953x_chip *chip = irq_data_get_irq_chip_data(d);

	chip->irq_mask |= 1 << (d->irq - chip->irq_base);
}

static void pca953x_irq_bus_lock(struct irq_data *d)
{
	struct pca953x_chip *chip = irq_data_get_irq_chip_data(d);

	mutex_lock(&chip->irq_lock);
}

static void pca953x_irq_bus_sync_unlock(struct irq_data *d)
{
	struct pca953x_chip *chip = irq_data_get_irq_chip_data(d);
	uint16_t new_irqs;
	uint16_t level;

	/* Look for any newly setup interrupt */
	new_irqs = chip->irq_trig_fall | chip->irq_trig_raise;
	new_irqs &= ~chip->reg_direction;

	while (new_irqs) {
		level = __ffs(new_irqs);
		pca953x_gpio_direction_input(&chip->gpio_chip, level);
		new_irqs &= ~(1 << level);
	}

	mutex_unlock(&chip->irq_lock);
}

static int pca953x_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct pca953x_chip *chip = irq_data_get_irq_chip_data(d);
	uint16_t level = d->irq - chip->irq_base;
	uint16_t mask = 1 << level;

	if (!(type & IRQ_TYPE_EDGE_BOTH)) {
		dev_err(&chip->client->dev, "irq %d: unsupported type %d\n",
			d->irq, type);
		return -EINVAL;
	}

	if (type & IRQ_TYPE_EDGE_FALLING)
		chip->irq_trig_fall |= mask;
	else
		chip->irq_trig_fall &= ~mask;

	if (type & IRQ_TYPE_EDGE_RISING)
		chip->irq_trig_raise |= mask;
	else
		chip->irq_trig_raise &= ~mask;

	return 0;
}

static struct irq_chip pca953x_irq_chip = {
	.name			= "pca953x",
	.irq_mask		= pca953x_irq_mask,
	.irq_unmask		= pca953x_irq_unmask,
	.irq_bus_lock		= pca953x_irq_bus_lock,
	.irq_bus_sync_unlock	= pca953x_irq_bus_sync_unlock,
	.irq_set_type		= pca953x_irq_set_type,
};

static uint16_t pca953x_irq_pending(struct pca953x_chip *chip)
{
	uint16_t cur_stat;
	uint16_t old_stat;
	uint16_t pending;
	uint16_t trigger;
	int ret, offset = 0;

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_INPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_IN;
		break;
	}
	ret = pca953x_read_reg(chip, offset, &cur_stat);
	if (ret)
		return 0;

	/* Remove output pins from the equation */
	cur_stat &= chip->reg_direction;

	old_stat = chip->irq_stat;
	trigger = (cur_stat ^ old_stat) & chip->irq_mask;

	if (!trigger)
		return 0;

	chip->irq_stat = cur_stat;

	pending = (old_stat & chip->irq_trig_fall) |
		  (cur_stat & chip->irq_trig_raise);
	pending &= trigger;

	return pending;
}

static irqreturn_t pca953x_irq_handler(int irq, void *devid)
{
	struct pca953x_chip *chip = devid;
	uint16_t pending;
	uint16_t level;

	pending = pca953x_irq_pending(chip);

	if (!pending)
		return IRQ_HANDLED;

	do {
		level = __ffs(pending);
		handle_nested_irq(level + chip->irq_base);

		pending &= ~(1 << level);
	} while (pending);

	return IRQ_HANDLED;
}

static int pca953x_irq_setup(struct pca953x_chip *chip,
			     const struct i2c_device_id *id)
{
	struct i2c_client *client = chip->client;
	struct pca953x_platform_data *pdata = client->dev.platform_data;
	int ret, offset = 0;

	if (pdata->irq_base != -1
			&& (id->driver_data & PCA_INT)) {
		int lvl;

		switch (chip->chip_type) {
		case PCA953X_TYPE:
			offset = PCA953X_INPUT;
			break;
		case PCA957X_TYPE:
			offset = PCA957X_IN;
			break;
		}
		ret = pca953x_read_reg(chip, offset, &chip->irq_stat);
		if (ret)
			goto out_failed;

		/*
		 * There is no way to know which GPIO line generated the
		 * interrupt.  We have to rely on the previous read for
		 * this purpose.
		 */
		chip->irq_stat &= chip->reg_direction;
		chip->irq_base = pdata->irq_base;
		mutex_init(&chip->irq_lock);

		for (lvl = 0; lvl < chip->gpio_chip.ngpio; lvl++) {
			int irq = lvl + chip->irq_base;

			irq_set_chip_data(irq, chip);
			irq_set_chip(irq, &pca953x_irq_chip);
			irq_set_nested_thread(irq, true);
#ifdef CONFIG_ARM
			set_irq_flags(irq, IRQF_VALID);
#else
			irq_set_noprobe(irq);
#endif
		}

		ret = request_threaded_irq(client->irq,
					   NULL,
					   pca953x_irq_handler,
					   IRQF_TRIGGER_RISING |
					   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					   dev_name(&client->dev), chip);
		if (ret) {
			dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
			goto out_failed;
		}

		chip->gpio_chip.to_irq = pca953x_gpio_to_irq;
	}

	return 0;

out_failed:
	chip->irq_base = -1;
	return ret;
}

static void pca953x_irq_teardown(struct pca953x_chip *chip)
{
	if (chip->irq_base != -1)
		free_irq(chip->client->irq, chip);
}
#else /* CONFIG_GPIO_PCA953X_IRQ */
static int pca953x_irq_setup(struct pca953x_chip *chip,
			     const struct i2c_device_id *id)
{
	struct i2c_client *client = chip->client;
	struct pca953x_platform_data *pdata = client->dev.platform_data;

	if (pdata->irq_base != -1 && (id->driver_data & PCA_INT))
		dev_warn(&client->dev, "interrupt support not compiled in\n");

	return 0;
}

static void pca953x_irq_teardown(struct pca953x_chip *chip)
{
}
#endif

/*
 * Handlers for alternative sources of platform_data
 */
#ifdef CONFIG_OF_GPIO
/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct pca953x_platform_data *
pca953x_get_alt_pdata(struct i2c_client *client)
{
	struct pca953x_platform_data *pdata;
	struct device_node *node;
	const __be32 *val;
	int size;

	node = client->dev.of_node;
	if (node == NULL)
		return NULL;

	pdata = kzalloc(sizeof(struct pca953x_platform_data), GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(&client->dev, "Unable to allocate platform_data\n");
		return NULL;
	}

	pdata->gpio_base = -1;
	val = of_get_property(node, "linux,gpio-base", &size);
	if (val) {
		if (size != sizeof(*val))
			dev_warn(&client->dev, "%s: wrong linux,gpio-base\n",
				 node->full_name);
		else
			pdata->gpio_base = be32_to_cpup(val);
	}

	val = of_get_property(node, "polarity", NULL);
	if (val)
		pdata->invert = *val;

	return pdata;
}
#else
static struct pca953x_platform_data *
pca953x_get_alt_pdata(struct i2c_client *client)
{
	return NULL;
}
#endif

static int __devinit device_pca953x_init(struct pca953x_chip *chip, int invert)
{
	int ret;

	ret = pca953x_read_reg(chip, PCA953X_OUTPUT, &chip->reg_output);
	if (ret)
		goto out;

	ret = pca953x_read_reg(chip, PCA953X_DIRECTION,
			       &chip->reg_direction);
	if (ret)
		goto out;

	/* set platform specific polarity inversion */
	ret = pca953x_write_reg(chip, PCA953X_INVERT, invert);
	if (ret)
		goto out;
	return 0;
out:
	return ret;
}

static int __devinit device_pca957x_init(struct pca953x_chip *chip, int invert)
{
	int ret;
	uint16_t val = 00;

	/* Let every port in proper state, that could save power */
	pca953x_write_reg(chip, PCA957X_PUPD, 0x0);
	pca953x_write_reg(chip, PCA957X_CFG, 0xffff);
	pca953x_write_reg(chip, PCA957X_OUT, 0x0);

	ret = pca953x_read_reg(chip, PCA957X_IN, &val);
	if (ret)
		goto out;
	ret = pca953x_read_reg(chip, PCA957X_OUT, &chip->reg_output);
	if (ret)
		goto out;
	ret = pca953x_read_reg(chip, PCA957X_CFG, &chip->reg_direction);
	if (ret)
		goto out;

	/* set platform specific polarity inversion */
	pca953x_write_reg(chip, PCA957X_INVRT, invert);

	/* To enable register 6, 7 to controll pull up and pull down */
	pca953x_write_reg(chip, PCA957X_BKEN, 0x202);

	return 0;
out:
	return ret;
}

/* LJH */              

static int pca9535_wr_byte(struct control_info *pctl_info)
{
	int i = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	
	switch (pctl_info->off)
	{
		case 0:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_set_value((&chip_1->gpio_chip), i, pctl_info->byte_data[i]);

				printk("\nfor chip_1:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 1:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_set_value((&chip_1->gpio_chip), i+8, pctl_info->byte_data[i]);
				printk("\nfor chip_1:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 2:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_set_value((&chip_2->gpio_chip), i, pctl_info->byte_data[i]);

				printk("\nfor chip_2:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 3:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_set_value((&chip_2->gpio_chip), i+8, pctl_info->byte_data[i]);
				printk("\nfor chip_2:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}

	return 0;
}


static int pca9535_rd_byte(struct control_info *pctl_info, struct control_info * parg)
{
	int i;
	int err;
	int ret = 0;
	int ret_data[8];
	memset(ret_data, 0, 8);

	printk("%s %s %d", __FILE__, __func__, __LINE__);

	switch(pctl_info->off)
	{
		case 0:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_1->gpio_chip), i);	
				ret_data[i] = ret;
			}

			printk("data:\n");
			for(i = 0; i < 8; i++)
			{
				printk("%d  ", ret_data[i]);
			}
			printk("\n");

			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_1 access write failed\n");
			if (copy_to_user(parg->byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			
			break;
		case 1:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_1->gpio_chip), i+8);	
				ret_data[i] = ret;
			}
			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_1 access write failed\n");
			if (copy_to_user(parg->byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			break;
		case 2:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_2->gpio_chip), i);	
				ret_data[i] = ret;
			}
			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_1 access write failed\n");
			if (copy_to_user(parg->byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			break;
		case 3:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_2->gpio_chip), i+8);	
				ret_data[i] = ret;
			}
			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_1 access write failed\n");
			if (copy_to_user(parg->byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			break;
		default:
			printk("error values of arg\n");
				return -EINVAL;
			break;
	}
	return 0;
}

static int pca9535_wr_double_byte(struct control_info *pctl_info)
{
	int i = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	
	switch (pctl_info->off)
	{
		case 0:
			for(i = 0; i < 16; i++)
			{
				pca953x_gpio_set_value((&chip_1->gpio_chip), i, pctl_info->d_byte_data[i]);

				printk("\nfor chip_1:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->d_byte_data[i]);
			}
			break;
		case 1:
			for(i = 0; i < 16; i++)
			{
				pca953x_gpio_set_value((&chip_2->gpio_chip), i, pctl_info->d_byte_data[i]);
				printk("\nfor chip_1:\n");
				printk("Set bit %d's data is %d\n", i, pctl_info->d_byte_data[i]);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}

	return 0;
}


static int pca9535_rd_double_byte(struct control_info *pctl_info, struct control_info * parg)
{
	int i;
	int err;
	int ret = 0;
	int ret_data[16];
	memset(ret_data, 0, 16);

	printk("%s %s %d", __FILE__, __func__, __LINE__);

	switch(pctl_info->off)
	{
		case 0:
			for(i = 0; i < 16; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_1->gpio_chip), i);	
				ret_data[i] = ret;
			}

			printk("data:\n");
			for(i = 0; i < 16; i++)
			{
				printk("%d  ", ret_data[i]);
			}
			printk("\n");

			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_1 access write failed\n");
			if (copy_to_user(parg->d_byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			
			break;
		case 1:
			for(i = 0; i < 16; i++)
			{
				ret = pca953x_gpio_get_value(&(chip_2->gpio_chip), i);	
				ret_data[i] = ret;
			}
			err = access_ok(VERIFY_WRITE, parg, sizeof(struct control_info));
			if(err == 0)
				printk("chip_2 access write failed\n");
			if (copy_to_user(parg->d_byte_data, ret_data, sizeof(struct control_info)))
				return -EFAULT;
			break;
		default:
			printk("error values of arg\n");
				return -EINVAL;
			break;
	}
	return 0;
}

static int pca9535_set_byte_out(struct control_info *pctl_info, struct control_info *parg)  //设置8位输出
{
	int i = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	
	switch (pctl_info->off)
	{
		case 0:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_direction_output(&(chip_1->gpio_chip), i, pctl_info->byte_data[i]);

				printk("\nfor chip_1:\n");
				printk("Diretcion_Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 1:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_direction_output(&(chip_1->gpio_chip), i+8, pctl_info->byte_data[i]);
				printk("\nfor chip_1:\n");
				printk("Directionn_Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 2:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_direction_output(&(chip_2->gpio_chip), i, pctl_info->byte_data[i]);

				printk("\nfor chip_2:\n");
				printk("Directionn_Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		case 3:
			for(i = 0; i < 8; i++)
			{
				pca953x_gpio_direction_output(&(chip_2->gpio_chip), i+8, pctl_info->byte_data[i]);
				printk("\nfor chip_2:\n");
				printk("Directionn_Set bit %d's data is %d\n", i, pctl_info->byte_data[i]);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}

	return 0;

}
				
static int pca9535_set_byte_in(struct control_info *pctl_info, struct control_info * parg)
{
	int i;
	int ret = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	switch(pctl_info->off)
	{
		case 0:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_1->gpio_chip), i);
				if(ret == 0)
					printk("chip1 %d first 8 byte SET_IN successufl\n", i);
			}
						
			break;
		case 1:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_1->gpio_chip), i+8);
				if(ret == 0)
					printk("chip1 %d second 8byte SET_IN successufl\n", i);
			}
			break;
		case 2:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_2->gpio_chip), i);
				if(ret == 0)
					printk("chip2 %d first 8 byte SET_IN successufl\n", i);
			}
			break;
		case 3:
			for(i = 0; i < 8; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_2->gpio_chip), i+8);
				if(ret == 0)
					printk("chip2 %d second 8 byte SET_IN successufl\n", i);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}
	return 0;
}

static int pca9535_set_double_byte_out(struct control_info *pctl_info, struct control_info *parg)  //设置8位输出
{
	int i = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	
	switch (pctl_info->off)
	{
		case 0:
			for(i = 0; i < 16; i++)
			{
				pca953x_gpio_direction_output(&(chip_1->gpio_chip), i, pctl_info->d_byte_data[i]);

				printk("\nfor chip_1:\n");
				printk("Diretcion_Set bit %d's data is %d\n", i, pctl_info->d_byte_data[i]);
			}
			break;
		case 1:
			for(i = 0; i < 16; i++)
			{
				pca953x_gpio_direction_output(&(chip_2->gpio_chip), i, pctl_info->d_byte_data[i]);

				printk("\nfor chip_2:\n");
				printk("Directionn_Set bit %d's data is %d\n", i, pctl_info->d_byte_data[i]);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}

	return 0;
}

static int pca9535_set_double_byte_in(struct control_info *pctl_info, struct control_info * parg)
{
	int i;
	int ret = 0;
	printk("%s %s %d", __FILE__, __func__, __LINE__);
	switch(pctl_info->off)
	{
		case 0:
			for(i = 0; i < 16; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_1->gpio_chip), i);
				if(ret == 0)
					printk("chip1 %d double byte SET_IN successufl\n", i);
			}
						
			break;
		case 2:
			for(i = 0; i < 16; i++)
			{
				ret = pca953x_gpio_direction_input(&(chip_2->gpio_chip), i);
				if(ret == 0)
					printk("chip2 %d double byteSET_IN successufl\n", i);
			}
			break;
		default:
			printk("error values of arg\n");
			return -EINVAL;
			break;
	}
	return 0;
}



static ssize_t pca9535_write(struct file *file, const char __user *buf, size_t count, loff_t *off)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 1;
}

static ssize_t pca9535_read(struct file * file, char __user *buf, size_t count, loff_t *off)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 1;
}

static long pca9535_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct control_info  __user *argp = (struct control_info __user *)arg;
	//int __user *argp = (int  __user *)arg;
	int err;
	struct control_info ctl_info;
	//unsigned off = 0;

	//if(copy_from_user(&off, argp, sizeof(int)))  //从用户空间argp拷贝到off中
	if(copy_from_user(&ctl_info, argp, sizeof(struct control_info)))  //从用户空间argp拷贝到off中
		 ret = - EFAULT;
	printk("copy_from_user ctl_info.off is %d\n", ctl_info.off);
	printk("copy_from_user ctl_info.val is %d\n", ctl_info.val);

	printk("this is %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	switch(cmd)
	{
		case IOC_PCA1_RD:
			{
				ret = pca953x_gpio_get_value(&(chip_1->gpio_chip), ctl_info.off);	
				//printk("chip1 %d IOC_PCA1_RD value is %d\n", off, ret);
				err = access_ok(VERIFY_WRITE, argp, sizeof(struct control_info));
				if(err == 0)
					printk("chip_1 access write failed\n");
				if (copy_to_user(&(argp->val), &ret, sizeof(struct control_info)))
					return -EFAULT;
				break;
			}
		case IOC_PCA1_WR:
			{
				pca953x_gpio_set_value(&(chip_1->gpio_chip), ctl_info.off, ctl_info.val);
				printk("In IOC_PCA1_WR  receive from user space off is %d  val is %d\n", ctl_info.off, ctl_info.val);
				break;
			}
		case IOC_PCA1_SET_OUT:
			{
				pca953x_gpio_direction_output(&(chip_1->gpio_chip), ctl_info.off, ctl_info.val);
				printk("In IOC_PCA1_SET_OUT  receive from user space off is %d  val is %d\n", ctl_info.off, ctl_info.val);
				break;
			}
		case IOC_PCA1_SET_IN:
			{
				ret = pca953x_gpio_direction_input(&(chip_1->gpio_chip), ctl_info.off);
				if(ret == 0)
					printk("chip1 %d IOC_PCA1_SET_IN successufl\n", ctl_info.off);
				break;
			}


		case IOC_PCA2_RD:
			{
				ret = pca953x_gpio_get_value(&(chip_2->gpio_chip), ctl_info.off);	
				//printk("chip2 %d IOC_PCA1_RD value is %d\n", off, ret);
				err = access_ok(VERIFY_WRITE, argp, sizeof(unsigned));
				if(err == 0)
					printk("chip_2 access write failed\n");
				if (copy_to_user(&(argp->val), &ret, sizeof(struct control_info)))
					return -EFAULT;
				break;
			}
		case IOC_PCA2_WR:
			{
				pca953x_gpio_set_value(&(chip_2->gpio_chip), ctl_info.off, ctl_info.val);
				printk("In IOC_PCA2_WR  receive from user space off is %d val is %d\n", ctl_info.off, ctl_info.val);
				break;
			}
		case IOC_PCA2_SET_OUT:
			{
				pca953x_gpio_direction_output(&(chip_2->gpio_chip), ctl_info.off, ctl_info.val);
				printk("In IOC_PCA2_SET_OUT  receive from user space off is %d val is %d\n", ctl_info.off, ctl_info.val);
				break;
			}
		case IOC_PCA2_SET_IN:
			{
				ret = pca953x_gpio_direction_input(&(chip_2->gpio_chip), ctl_info.off);
				if(ret == 0)
					printk("chip2 %d IOC_PCA2_SET_IN successufl\n", ctl_info.off);
				break;
			}
		case IOC_PCA_BYTE_WR_1:
		case IOC_PCA_BYTE_WR_2:
			{
				ret = pca9535_wr_byte(&ctl_info);
				break;
				
			}
		case IOC_PCA_BYTE_RD_1:
		case IOC_PCA_BYTE_RD_2:
			{
				ret = pca9535_rd_byte(&ctl_info, argp);
				break;
			}
		case IOC_PCA_DBYTE_RD:
			{
				ret = pca9535_rd_double_byte(&ctl_info, argp);
				break;
			}
		case IOC_PCA_DBYTE_WD:
			{
				ret = pca9535_wr_double_byte(&ctl_info);
				break;
			}
		case IOC_SET_B_OUT_1:
		case IOC_SET_B_OUT_2:
			{
				ret = pca9535_set_byte_out(&ctl_info, argp);
				break;
			}
		case IOC_SET_B_IN_1:
		case IOC_SET_B_IN_2:
			{
				ret = pca9535_set_byte_in(&ctl_info, argp);
				break;
			}
		case IOC_SET_DB_OUT:
			{
				ret = pca9535_set_double_byte_out(&ctl_info, argp);	
				break;
			}
		case IOC_SET_DB_IN:
			{
				ret = pca9535_set_double_byte_in(&ctl_info, argp);	
				break;
			}

		defalut:
			printk("error values of arg\n");
				return -EINVAL;
			break;
	
	}
	return 0;
}


static struct file_operations pca9535_fops = {
	.owner 		= THIS_MODULE,
	.read  		= pca9535_read,
	.write 		= pca9535_write,
	.unlocked_ioctl = pca9535_ioctl,
};


static int __devinit pca953x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct pca953x_platform_data *pdata;
	//struct pca953x_chip *chip;
	int ret = 0;

#if 0
/* ----------LJH------*/
	dev_t dev = MKDEV(major, 0);
	int result = 0;

	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	if(major)
		result = register_chrdev_region(dev, 2, "pca9535");
	else
	{
		result = alloc_chrdev_region(&dev, 0, 2, "pca9535");
		major = MAJOR(dev);
	}
	if(result < 0 )
	{
		printk("unbable to get major %d\n", major);
	}
	printk("get major is %d\n", major);

	cdev_init(&pca9535_dev, &pca9535_fops);
	ret=cdev_add(&pca9535_dev, dev, 1);
	if(ret < 0)
		printk("add device failure\n");

/*------LJH-------------*/
#endif
		


/*  --------------LJH ----------begin------------- */

	if(flag == 0)
	{
		chip_1 = kzalloc(sizeof(struct pca953x_chip), GFP_KERNEL);
		if (chip_1 == NULL)
			return -ENOMEM;

		pdata = client->dev.platform_data;
		if (pdata == NULL) {
			pdata = pca953x_get_alt_pdata(client);
			/*
			 * Unlike normal platform_data, this is allocated
			 * dynaand must be freed in the driver
			 */
			chip_1->dyn_pdata = pdata;
		}

		if (pdata == NULL) {
			dev_dbg(&client->dev, "no platform data\n");
			ret = -EINVAL;
			goto out_failed_1;
		}

		chip_1->client = client;

		chip_1->gpio_start = pdata->gpio_base;

		chip_1->names = pdata->names;
		chip_1->chip_type = id->driver_data & (PCA953X_TYPE | PCA957X_TYPE);

		mutex_init(&chip_1->i2c_lock);

		/* initialize cached registers from their original values.
		 * we can't share this chip with another i2c master.
		 */
		pca953x_setup_gpio(chip_1, id->driver_data & PCA_GPIO_MASK);

		if (chip_1->chip_type == PCA953X_TYPE)
			device_pca953x_init(chip_1, pdata->invert);
		else if (chip_1->chip_type == PCA957X_TYPE)
			device_pca957x_init(chip_1, pdata->invert);
		else
			goto out_failed_1;

		ret = pca953x_irq_setup(chip_1, id);
		if (ret)
			goto out_failed_1;

		ret = gpiochip_add(&chip_1->gpio_chip);
		if (ret)
			goto out_failed_irq_1;

		if (pdata->setup) {
			ret = pdata->setup(client, chip_1->gpio_chip.base,
					chip_1->gpio_chip.ngpio, pdata->context);
			if (ret < 0)
				dev_warn(&client->dev, "setup failed, %d\n", ret);
		}


		i2c_set_clientdata(client, chip_1);

		class_1 = class_create(THIS_MODULE, "pca9535_1");
		if(IS_ERR(class_1))
			return PTR_ERR(class_1);

		major_1 = register_chrdev(0, "pca9535_1", &pca9535_fops);
		if(major_1 < 0)
		{
			printk(KERN_WARNING "pca9535_1"":could not get major_1 number\n");
		}

		device_create(class_1, NULL, MKDEV(major_1, 0), NULL, "pca9535_1");
		printk("pca9535_1 device crerated\n");
		flag = 1;

    		printk("this is %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	}
	else if(flag == 1)
	{
		chip_2 = kzalloc(sizeof(struct pca953x_chip), GFP_KERNEL);
		if (chip_2 == NULL)
			return -ENOMEM;

		pdata = client->dev.platform_data;
		if (pdata == NULL) {
			pdata = pca953x_get_alt_pdata(client);
			/*
			 * Unlike normal platform_data, this is allocated
			 * dynaand must be freed in the driver
			 */
			chip_2->dyn_pdata = pdata;
		}

		if (pdata == NULL) {
			dev_dbg(&client->dev, "no platform data\n");
			ret = -EINVAL;
			goto out_failed_2;
		}

		chip_2->client = client;

		chip_2->gpio_start = pdata->gpio_base;

		chip_2->names = pdata->names;
		chip_2->chip_type = id->driver_data & (PCA953X_TYPE | PCA957X_TYPE);

		mutex_init(&chip_2->i2c_lock);

		/* initialize cached registers from their original values.
		 * we can't share this chip with another i2c master.
		 */
		pca953x_setup_gpio(chip_2, id->driver_data & PCA_GPIO_MASK);

		if (chip_2->chip_type == PCA953X_TYPE)
			device_pca953x_init(chip_2, pdata->invert);
		else if (chip_2->chip_type == PCA957X_TYPE)
			device_pca957x_init(chip_2, pdata->invert);
		else
			goto out_failed_2;

		ret = pca953x_irq_setup(chip_2, id);
		if (ret)
			goto out_failed_2;

		ret = gpiochip_add(&chip_2->gpio_chip);
		if (ret)
			goto out_failed_irq_2;

		if (pdata->setup) {
			ret = pdata->setup(client, chip_2->gpio_chip.base,
					chip_2->gpio_chip.ngpio, pdata->context);
			if (ret < 0)
				dev_warn(&client->dev, "setup failed, %d\n", ret);
		}


		i2c_set_clientdata(client, chip_2);

		class_2 = class_create(THIS_MODULE, "pca9535_2");
		if(IS_ERR(class_2))
			return PTR_ERR(class_2);

		major_2 = register_chrdev(0, "pca9535_2", &pca9535_fops);
		if(major_2 < 0)
		{
			printk(KERN_WARNING "pca9535_2"":could not get major_2 number\n");
		}

		device_create(class_2, NULL, MKDEV(major_2, 0), NULL, "pca9535_2");

		printk("pca9535_2 device crerated\n");

		printk("this is %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
		flag = 0;

	}
	else
	{
		printk("Could not create device!\n");
		return -1;
	}

/*  ---------------LJH ---end--------------- */
		return 0;

out_failed_irq_1:
	pca953x_irq_teardown(chip_1);
out_failed_1:
	kfree(chip_1->dyn_pdata);
	kfree(chip_1);
	return ret;

out_failed_irq_2:
	pca953x_irq_teardown(chip_2);
out_failed_2:
	kfree(chip_2->dyn_pdata);
	kfree(chip_2);
	return ret;
}

static int pca953x_remove(struct i2c_client *client)
{
	struct pca953x_platform_data *pdata = client->dev.platform_data;
	struct pca953x_chip *chip = i2c_get_clientdata(client);
	int ret = 0;

	if (pdata->teardown) {
		ret = pdata->teardown(client, chip->gpio_chip.base,
				chip->gpio_chip.ngpio, pdata->context);
		if (ret < 0) {
			dev_err(&client->dev, "%s failed, %d\n",
					"teardown", ret);
			return ret;
		}
	}

	ret = gpiochip_remove(&chip->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "%s failed, %d\n",
				"gpiochip_remove()", ret);
		return ret;
	}

	pca953x_irq_teardown(chip);
	kfree(chip->dyn_pdata);
	kfree(chip);

	device_destroy(class_1, MKDEV(major_1, 0));
	device_destroy(class_2, MKDEV(major_2, 0));
	class_destroy(class_1);
	class_destroy(class_2);

//	cdev_del(&pca9535_dev);
   	unregister_chrdev(major_1, "pca9535_1");
   	unregister_chrdev(major_2, "pca9535_2");

    	printk("everything is removed!\n");

	return 0;
}

static struct i2c_driver pca953x_driver = {
	.driver = {
		.name	= "pca953x",
	},
	.probe		= pca953x_probe,
	.remove		= pca953x_remove,
	.id_table	= pca953x_id,
};

static int __init pca953x_init(void)
{
	return i2c_add_driver(&pca953x_driver);
}
/* register after i2c postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(pca953x_init);

static void __exit pca953x_exit(void)
{
	i2c_del_driver(&pca953x_driver);
}
module_exit(pca953x_exit);

MODULE_AUTHOR("eric miao <eric.miao@marvell.com>");
MODULE_DESCRIPTION("GPIO expander driver for PCA953x");
MODULE_LICENSE("GPL");
