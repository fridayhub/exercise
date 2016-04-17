#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <mach/gpio-fns.h>

#define DEVICE_NAME "led"

/*define pins*/
#define S3C2410_GPF6_OUTP   (0x01 << 6*2) //red led
#define S3C2410_GPF0_OUTP   (0x01 << 0*2) //beep 
#define S3C2410_GPH9_OUTP   (0x01 << 9*2) //green led
#define S3C2410_GPF5_INPUT  (0x00 << 5*2) //key

/* 应用程序执行ioctl(fd, cmd, arg)时的第2个参数 */
#define IOCTL_SET_ON	1
#define IOCTL_SET_OFF	0

static unsigned char key_val;
int err = 0;

/* 用来指定LED/BEEP所用的GPIO引脚 */
static unsigned long gpio_table [] =
{
	S3C2410_GPF(6),
	S3C2410_GPF(0),
    S3C2410_GPH(9),
    S3C2410_GPF(5),
};

/* 用来指定GPIO引脚的功能：输出 */
static unsigned int gpio_cfg_table [] =
{
	S3C2410_GPF6_OUTP, //red led set output
    S3C2410_GPF0_OUTP, //beep 
    S3C2410_GPH9_OUTP, //green led output
    S3C2410_GPF5_INPUT, //red key value
};

static long cl2416_gpio_ioctl(
	struct file *file, 
	unsigned int cmd, 
	unsigned long arg)
{
	if (arg > 3)
	{
		return -EINVAL;
	}
    
	switch(cmd)
	{
		case IOCTL_SET_ON:
			// 设置指定引脚的输出电平为0,red led
            if(arg == 0)
            {
                s3c2410_gpio_setpin(gpio_table[arg], 0);
                return 0;
            }else
            {
		    	s3c2410_gpio_setpin(gpio_table[arg], 1);
			    return 0;
            }

		case IOCTL_SET_OFF:
			// 设置指定引脚的输出电平为1
            if(arg == 0)
            {
                s3c2410_gpio_setpin(gpio_table[arg], 1);
                return 0;
            }else
            {
			    s3c2410_gpio_setpin(gpio_table[arg], 0);
			    return 0;
            }

		default:
			return -EINVAL;
	}
}

ssize_t button_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    if (size != 1)
        return -EINVAL;

    key_val = s3c2410_gpio_getpin(gpio_table[3]);

    err = copy_to_user(buf, &key_val, 1);
    if( err < 0 )
    {
        printk("fail to copy_to_user\n");
    }

    return 1;
}

static struct file_operations dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = cl2416_gpio_ioctl,
    .read           = button_drv_read,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int ret;

	int i;
	
	for (i = 0; i < 3; i++)
	{
		s3c2410_gpio_cfgpin(gpio_table[i], gpio_cfg_table[i]);
		s3c2410_gpio_setpin(gpio_table[i], 0);
	}

	ret = misc_register(&misc);

	printk (DEVICE_NAME" initialized\n");

	return ret;
}

static void __exit dev_exit(void)
{
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liujinghang");
MODULE_DESCRIPTION("GPIO control for Hang");
