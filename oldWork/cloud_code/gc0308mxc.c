/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/fsl_devices.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-int-device.h>
#include "mxc_v4l2_capture.h"

/*+++ LJH begin +++*/
#include "gc0308mxc.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h> 
/*+++ LJH end +++*/

#define FPS_NUMERATOR    30
#define FPS_DENOMINATOR  1001

#define GC0308_XCLK_MIN 24000000
#define GC0308_XCLK_MAX 24000000

/*+++ LJH  begin +++ */
static struct sensor_data gc0308_data;
static int pwn_gpio, rst_gpio;

static void mx6q_csi0_cam_powerdown(int powerdown)
{
	printk("%s powerdown=%d\n",__func__,powerdown);
	if (powerdown)
		gpio_set_value(pwn_gpio, 1);
	else
		gpio_set_value(pwn_gpio, 0);

	msleep(100);
}

static void mx6q_csi0_io_init(void)
{
	printk("mx6q_csi0_io_init\n");

	gpio_set_value(rst_gpio, 1);

	gpio_set_value(pwn_gpio, 1);

	msleep(100);
	gpio_set_value(pwn_gpio, 0);
	msleep(100);
	gpio_set_value(rst_gpio, 0);
	msleep(100);
	gpio_set_value(rst_gpio, 1);
	msleep(100);
	gpio_set_value(pwn_gpio, 1);
	msleep(100);
        
}


/*+++ LJH  end+++ */

enum gc0308_mode {
	gc0308_mode_MIN = 0,
	gc0308_mode_VGA_640_480 = 0,
	gc0308_mode_MAX = 1
};

enum gc0308_frame_rate {
	gc0308_15_fps,
	gc0308_30_fps
};

struct reg_value {
	u8 u8RegAddr;
	u8 u8Val;
};

struct gc0308_mode_info {
	enum gc0308_mode mode;
	u32 width;
	u32 height;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};

/*!
 * Maintains the information on the current state of the sesor.
 */

static struct reg_value gc0308_setting_30fps_VGA_640_480[] = {

    {0xfe,0x80},
    {0xfe,0x00},
    {0x22,0x55},
    {0x03,0x02},
    {0x04,0x58},//12c
    {0x5a,0x56},
    {0x5b,0x40},
    {0x5c,0x4a},
    {0x22,0x57},

   // 50hz   24M MCLKauto
	{0x01 , 0x32},  //6a
	{0x02 , 0x70},
	{0x0f , 0x01},

	{0xe2 , 0x00},   //anti-flicker step [11:8]
	{0xe3 , 0x78},   //anti-flicker step [7:0]

	{0xe4 , 0x02},
	{0xe5 , 0x58},
	{0xe6 , 0x03},
	{0xe7 , 0x48},
	{0xe8 , 0x04},
	{0xe9 , 0xb0},
	{0xea , 0x05},
	{0xeb , 0xa0},

	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x02},
	{0x09,0x01},
	{0x0a,0xea},// ea
	{0x0b,0x02},
	{0x0c,0x88},
	{0x0d,0x02},
	{0x0e,0x02},
	{0x10,0x26},
	{0x11,0x0d},
	{0x12,0x2a},
	{0x13,0x00},
	{0x14,0x13}, //10 // h_v need mirror for preview
	{0x15,0x0a},
	{0x16,0x05},
	{0x17,0x01},
	{0x18,0x44},
	{0x19,0x44},
	{0x1a,0x2a},
	{0x1b,0x00},
	{0x1c,0x49},
	{0x1d,0x9a},
	{0x1e,0x61},
	{0x1f,0x16},
	{0x20,0xff},//ff
	{0x21,0xf8},//fa
	{0x22,0x57},
	{0x24,0xa2},	// output format
	{0x25,0x0f},
#ifdef CONFIG_ARCH_MESON3
	{0x26,0x01}, //03
#else
	{0x26,0x03}, //03
#endif
	{0x2f,0x01},
	{0x30,0xf7},
	{0x31,0x50},
	{0x32,0x00},
	{0x39,0x04},
	{0x3a,0x20},
	{0x3b,0x20},
	{0x3c,0x02},
	{0x3d,0x02}, //0x00
	{0x3e,0x02},
	{0x3f,0x02},
	{0x50,0x14}, // 0x14
	{0x53,0x80},
	{0x54,0x87},
	{0x55,0x87},
	{0x56,0x80},

	{0x57,0x7a},// r ratio
	{0x58,0x7e},// g ratio
	{0x59,0x84},//b ratio

	{0x8b,0x10},
	{0x8c,0x10},
	{0x8d,0x10},
	{0x8e,0x10},
	{0x8f,0x10},
	{0x90,0x10},
	{0x91,0x3c},
	{0x92,0x50},
	{0x5d,0x12},
	{0x5e,0x1a},
	{0x5f,0x24},
	{0x60,0x07},
	{0x61,0x15},
	{0x62,0x08}, // 0x08
	{0x64,0x03},  // 0x03
	{0x66,0xe8},
	{0x67,0x86},
	{0x68,0xa2},
	{0x69,0x18},
	{0x6a,0x0f},
	{0x6b,0x00},
	{0x6c,0x5f},
	{0x6d,0x8f},
	{0x6e,0x55},
	{0x6f,0x38},
	{0x70,0x15},
	{0x71,0x33}, // low light startion
	{0x72,0xdc},
	{0x73,0x80},
	{0x74,0x02},
	{0x75,0x3f},
	{0x76,0x02},
	{0x77,0x45}, // 0x47 //0x54
	{0x78,0x88},
	{0x79,0x81},
	{0x7a,0x81},
	{0x7b,0x22},
	{0x7c,0xff},
///////CC/////
	{0x93,0x42},  //0x48
	{0x94,0x00},
	{0x95,0x0c},//04
	{0x96,0xe0},
	{0x97,0x46},
	{0x98,0xf3},

	{0xb1,0x40},// startion
	{0xb2,0x40},
	{0xb3,0x3c}, //0x40  contrast
	{0xb5,0x00}, //
	{0xb6,0xe0},
	{0xbd,0x3C},
	{0xbe,0x36},
	{0xd0,0xCb},//c9
	{0xd1,0x10},
	{0xd2,0x90},
	{0xd3,0x50},//88
	{0xd5,0xF2},
	{0xd6,0x10},
	{0xdb,0x92},
	{0xdc,0xA5},
	{0xdf,0x23},
	{0xd9,0x00},
	{0xda,0x00},
	{0xe0,0x09},
	{0xed,0x04},
	{0xee,0xa0},
	{0xef,0x40},
	{0x80,0x03},

	{0x9F, 0x10},//		case 3:
	{0xA0, 0x20},
	{0xA1, 0x38},
	{0xA2, 0x4E},
	{0xA3, 0x63},
	{0xA4, 0x76},
	{0xA5, 0x87},
	{0xA6, 0xA2},
	{0xA7, 0xB8},
	{0xA8, 0xCA},
	{0xA9, 0xD8},
	{0xAA, 0xE3},
	{0xAB, 0xEB},
	{0xAC, 0xF0},
	{0xAD, 0xF8},
	{0xAE, 0xFD},
	{0xAF, 0xFF},

	{0xc0,0x00},
	{0xc1,0x14},
	{0xc2,0x21},
	{0xc3,0x36},
	{0xc4,0x49},
	{0xc5,0x5B},
	{0xc6,0x6B},
	{0xc7,0x7B},
	{0xc8,0x98},
	{0xc9,0xB4},
	{0xca,0xCE},
	{0xcb,0xE8},
	{0xcc,0xFF},
	{0xf0,0x02},
	{0xf1,0x01},
	{0xf2,0x01},
	{0xf3,0x30},
	{0xf9,0x9f},
	{0xfa,0x78},
	{0xfe,0x01},
	{0x00,0xf5},
	{0x02,0x20},
	{0x04,0x10},
	{0x05,0x10},
	{0x06,0x20},
	{0x08,0x15},
	{0x0a,0xa0},
	{0x0b,0x64},
	{0x0c,0x08},
	{0x0e,0x4C},
	{0x0f,0x39},
	{0x10,0x41},
	{0x11,0x37},
	{0x12,0x24},
	{0x13,0x39},
	{0x14,0x45},
	{0x15,0x45},
	{0x16,0xc2},
	{0x17,0xA8},
	{0x18,0x18},
	{0x19,0x55},
	{0x1a,0xd8},
	{0x1b,0xf5},

	{0x1c,0x60}, //r gain limit

	{0x70,0x40},
	{0x71,0x58},
	{0x72,0x30},
	{0x73,0x48},
	{0x74,0x20},
	{0x75,0x60},
	{0x77,0x20},
	{0x78,0x32},
	{0x30,0x03},
	{0x31,0x40},
	{0x32,0x10},
	{0x33,0xe0},
	{0x34,0xe0},
	{0x35,0x00},
	{0x36,0x80},
	{0x37,0x00},
	{0x38,0x04},
	{0x39,0x09},
	{0x3a,0x12},
	{0x3b,0x1C},
	{0x3c,0x28},
	{0x3d,0x31},
	{0x3e,0x44},
	{0x3f,0x57},
	{0x40,0x6C},
	{0x41,0x81},
	{0x42,0x94},
	{0x43,0xA7},
	{0x44,0xB8},
	{0x45,0xD6},
	{0x46,0xEE},
	{0x47,0x0d},
	{0xfe, 0x00},// update

	{0x10, 0x26},
	{0x11, 0x0d},  // fd,modified by mormo 2010/07/06
	{0x1a, 0x2a},  // 1e,modified by mormo 2010/07/06

	{0x1c, 0x49}, // c1,modified by mormo 2010/07/06
	{0x1d, 0x9a}, // 08,modified by mormo 2010/07/06
	{0x1e, 0x61}, // 60,modified by mormo 2010/07/06

	{0x3a, 0x20},

	{0x50, 0x14},  // 10,modified by mormo 2010/07/06
	{0x53, 0x80},
	{0x56, 0x80},

	{0x8b, 0x20}, //LSC
	{0x8c, 0x20},
	{0x8d, 0x20},
	{0x8e, 0x14},
	{0x8f, 0x10},
	{0x90, 0x14},

	{0x94, 0x02},
	{0x95, 0x0c}, //0x07
	{0x96, 0xe0},

	{0xb1, 0x40}, // YCPT
	{0xb2, 0x40},
	{0xb3, 0x3c},
	{0xb6, 0xe0},

	{0xd0, 0xcb}, // AECT  c9,modifed by mormo 2010/07/06
	{0xd3, 0x50}, // 80,modified by mormor 2010/07/06   58

	{0xf2, 0x02},
	{0xf7, 0x12},
	{0xf8, 0x0a},
	//Regsters of Page1
	{0xfe, 0x01},

	{0x02, 0x20},
	{0x04, 0x10},
	{0x05, 0x08},
	{0x06, 0x20},
	{0x08, 0x0a},

	{0x0e, 0x44},
	{0x0f, 0x32},
	{0x10, 0x41},
	{0x11, 0x37},
	{0x12, 0x22},
	{0x13, 0x19},
	{0x14, 0x44},
	{0x15, 0x44},

	{0x19, 0x50},
	{0x1a, 0xd8},

	{0x32, 0x10},

	{0x35, 0x00},
	{0x36, 0x80},
	{0x37, 0x00},
	//-----------Update the registers end---------//
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xff, 0xff},

};
/*
static struct reg_value gc0308_setting_30fps_QVGA_320_240[] = {
};
*/
static struct gc0308_mode_info gc0308_mode_info_data[2][gc0308_mode_MAX + 1] = {
	{
		{gc0308_mode_VGA_640_480, 640, 480, 
		 gc0308_setting_30fps_VGA_640_480, 
		 ARRAY_SIZE(gc0308_setting_30fps_VGA_640_480)},
	},
	{
		{gc0308_mode_VGA_640_480, 640, 480, 
		 gc0308_setting_30fps_VGA_640_480, 
		 ARRAY_SIZE(gc0308_setting_30fps_VGA_640_480)},
	},
};

//static struct fsl_mxc_camera_platform_data *camera_plat;

static int gc0308_probe(struct i2c_client *adapter,
				const struct i2c_device_id *device_id);
static int gc0308_remove(struct i2c_client *client);

static s32 gc0308_read_reg(u8 reg, u8 *val);
static s32 gc0308_write_reg(u8 reg, u8 val);

static const struct i2c_device_id gc0308_id[] = {
	{"gc0308-capture", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, gc0308_id);

static struct i2c_driver gc0308_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "gc0308-capture",
		  },
	.probe  = gc0308_probe,
	.remove = gc0308_remove,
	.id_table = gc0308_id,
};

static s32 gc0308_write_reg(u8 reg, u8 val)
{
	u8 au8Buf[3] = {0};

	au8Buf[0] = reg;
	au8Buf[1] = val;

	printk("%s %s %d\n", __FILE__,  __func__, __LINE__ );

	if (i2c_master_send(gc0308_data.i2c_client, au8Buf, 2) < 0) {
		pr_err("%s:write reg error:reg=%x,val=%x\n",
			__func__, reg, val);
		return -1;
	}


	return 0;
}

static s32 gc0308_read_reg(u8 reg, u8 *val)
{
	u8 au8RegBuf[2] = {0};
	u8 u8RdVal = 0;

	au8RegBuf[0] = reg;


	if (1 != i2c_master_send(gc0308_data.i2c_client, au8RegBuf, 1)) {
		pr_err("%s:write reg error:reg=%x\n",
				__func__, reg);
		return -1;
	}

	if (1 != i2c_master_recv(gc0308_data.i2c_client, &u8RdVal, 1)) {
		pr_err("%s:read reg error:reg=%x,val=%x\n",
				__func__, reg, u8RdVal);
		return -1;
	}

	*val = u8RdVal;

	printk("%s %s %d\n", __FILE__,  __func__, __LINE__ );

	return u8RdVal;
}

static int gc0308_init_mode(enum gc0308_frame_rate frame_rate,
			    enum gc0308_mode mode)
{
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u8 RegAddr = 0;
	register u8 Mask = 0;
	register u8 Val = 0;
	u8 RegVal = 0;
	int retval = 0;

	printk("gc0308_init_mode: >>>>>>>>>>>>>>>>>>>>>>>\n");
	if (mode > gc0308_mode_MAX || mode < gc0308_mode_MIN) {
		pr_err("Wrong gc0308 mode detected!\n");
		return -1;
	}

	pModeSetting = gc0308_mode_info_data[frame_rate][mode].init_data_ptr;
	iModeSettingArySize =
		gc0308_mode_info_data[frame_rate][mode].init_data_size;

	gc0308_data.pix.width = gc0308_mode_info_data[frame_rate][mode].width;
	gc0308_data.pix.height = gc0308_mode_info_data[frame_rate][mode].height;

	if (gc0308_data.pix.width == 0 || gc0308_data.pix.height == 0 ||
	    pModeSetting == NULL || iModeSettingArySize == 0)
		return -EINVAL;

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = 0;
		RegAddr = pModeSetting->u8RegAddr;
		Val = pModeSetting->u8Val;
		Mask = 0;

		printk("%s %s %d\n", __FILE__,  __func__, __LINE__ );

		if (Mask) {
			retval = gc0308_read_reg(RegAddr, &RegVal);
			if (retval < 0)
				goto err;

			RegVal &= ~(u8)Mask;
			Val &= Mask;
			Val |= RegVal;
		}

		
		retval = gc0308_write_reg(RegAddr, Val);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
	printk("gc0308_init_mode: <<<<<<<<<<<<<<<<<<<\n");
err:
	return retval;
}

/* --------------- IOCTL functions from v4l2_int_ioctl_desc --------------- */

static int ioctl_g_ifparm(struct v4l2_int_device *s, struct v4l2_ifparm *p)
{
	if (s == NULL) {
		pr_err("   ERROR!! no slave device set!\n");
		return -1;
	}

	memset(p, 0, sizeof(*p));
	p->u.bt656.clock_curr = gc0308_data.mclk;
	pr_debug("   clock_curr=mclk=%d\n", gc0308_data.mclk);
	p->if_type = V4L2_IF_TYPE_BT656;
	p->u.bt656.mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT;
	p->u.bt656.clock_min = GC0308_XCLK_MIN;
	p->u.bt656.clock_max = GC0308_XCLK_MAX;
	p->u.bt656.bt_sync_correct = 1;  /* Indicate external vsync */

	return 0;
}

/*!
 * ioctl_s_power - V4L2 sensor interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int ioctl_s_power(struct v4l2_int_device *s, int on)
{
	struct sensor_data *sensor = s->priv;
#if 0
	if (on && !sensor->on) {
		if (io_regulator)
			if (regulator_enable(io_regulator) != 0)
				return -EIO;
		if (core_regulator)
			if (regulator_enable(core_regulator) != 0)
				return -EIO;
		if (gpo_regulator)
			if (regulator_enable(gpo_regulator) != 0)
				return -EIO;
		if (analog_regulator)
			if (regulator_enable(analog_regulator) != 0)
				return -EIO;
		/* Make sure power on */
		if (camera_plat->pwdn)
			camera_plat->pwdn(0);

	} else if (!on && sensor->on) {
		if (analog_regulator)
			regulator_disable(analog_regulator);
		if (core_regulator)
			regulator_disable(core_regulator);
		if (io_regulator)
			regulator_disable(io_regulator);
		if (gpo_regulator)
			regulator_disable(gpo_regulator);

		if (camera_plat->pwdn)
			camera_plat->pwdn(1);
}
#endif
	sensor->on = on;

	return 0;
}

/*!
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct sensor_data *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		memset(a, 0, sizeof(*a));
		a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cparm->capability = sensor->streamcap.capability;
		cparm->timeperframe = sensor->streamcap.timeperframe;
		cparm->capturemode = sensor->streamcap.capturemode;
		ret = 0;
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*!
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct sensor_data *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	u32 tgt_fps;	/* target frames per secound */
	enum gc0308_frame_rate frame_rate;
	int ret = 0;

	/* Make sure power on */
	/*if (camera_plat->pwdn)
		camera_plat->pwdn(0);  ----LJH*/
	mx6q_csi0_cam_powerdown(0);

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		/* Check that the new frame rate is allowed. */
		if ((timeperframe->numerator == 0) ||
		    (timeperframe->denominator == 0)) {
			timeperframe->denominator = FPS_DENOMINATOR;
			timeperframe->numerator = FPS_NUMERATOR;
		}

		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;
/*
		if (tgt_fps > MAX_FPS) {
			timeperframe->denominator = MAX_FPS;
			timeperframe->numerator = 1;
		} else if (tgt_fps < MIN_FPS) {
			timeperframe->denominator = MIN_FPS;
			timeperframe->numerator = 1;
		}
*/
		/* Actual frame rate we use */
		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;

		frame_rate = gc0308_30_fps;

		sensor->streamcap.timeperframe = *timeperframe;
		sensor->streamcap.capturemode =
				(u32)a->parm.capture.capturemode;

		ret = gc0308_init_mode(frame_rate,
				       sensor->streamcap.capturemode);
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		pr_debug("   type is not " \
			"V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
			a->type);
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*!
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct sensor_data *sensor = s->priv;

	f->fmt.pix = sensor->pix;

	return 0;
}

/*!
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the video_control[] array.  Otherwise, returns -EINVAL
 * if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int ret = 0;

	switch (vc->id) {
	case V4L2_CID_BRIGHTNESS:
		vc->value = gc0308_data.brightness;
		break;
	case V4L2_CID_HUE:
		vc->value = gc0308_data.hue;
		break;
	case V4L2_CID_CONTRAST:
		vc->value = gc0308_data.contrast;
		break;
	case V4L2_CID_SATURATION:
		vc->value = gc0308_data.saturation;
		break;
	case V4L2_CID_RED_BALANCE:
		vc->value = gc0308_data.red;
		break;
	case V4L2_CID_BLUE_BALANCE:
		vc->value = gc0308_data.blue;
		break;
	case V4L2_CID_EXPOSURE:
		vc->value = gc0308_data.ae_mode;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/*!
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the video_control[] array).  Otherwise,
 * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int retval = 0;

	pr_debug("In gc0308:ioctl_s_ctrl %d\n",
		 vc->id);

	switch (vc->id) {
	case V4L2_CID_BRIGHTNESS:
		break;
	case V4L2_CID_CONTRAST:
		break;
	case V4L2_CID_SATURATION:
		break;
	case V4L2_CID_HUE:
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		break;
	case V4L2_CID_RED_BALANCE:
		break;
	case V4L2_CID_BLUE_BALANCE:
		break;
	case V4L2_CID_GAMMA:
		break;
	case V4L2_CID_EXPOSURE:
		break;
	case V4L2_CID_AUTOGAIN:
		break;
	case V4L2_CID_GAIN:
		break;
	case V4L2_CID_HFLIP:
		break;
	case V4L2_CID_VFLIP:
		break;
	default:
		retval = -EPERM;
		break;
	}

	return retval;
}

/*!
 * ioctl_enum_framesizes - V4L2 sensor interface handler for
 *			   VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
				 struct v4l2_frmsizeenum *fsize)
{
	printk("ioctl_enum_framesizes >>, fsize->index=%d\n", fsize->index);
	if (fsize->index > gc0308_mode_MAX)
		return -EINVAL;

	fsize->pixel_format = gc0308_data.pix.pixelformat;
	fsize->discrete.width =
			max(gc0308_mode_info_data[0][fsize->index].width,
			    gc0308_mode_info_data[1][fsize->index].width);
	fsize->discrete.height =
			max(gc0308_mode_info_data[0][fsize->index].height,
			    gc0308_mode_info_data[1][fsize->index].height);
	return 0;
}

/*!
 * ioctl_g_chip_ident - V4L2 sensor interface handler for
 *			VIDIOC_DBG_G_CHIP_IDENT ioctl
 * @s: pointer to standard V4L2 device structure
 * @id: pointer to int
 *
 * Return 0.
 */
static int ioctl_g_chip_ident(struct v4l2_int_device *s, int *id)
{
	((struct v4l2_dbg_chip_ident *)id)->match.type =
					V4L2_CHIP_MATCH_I2C_DRIVER;
	strcpy(((struct v4l2_dbg_chip_ident *)id)->match.name, "gc0308_camera");

	return 0;
}

/*!
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 */
static int ioctl_init(struct v4l2_int_device *s)
{

	return 0;
}

/*!
 * ioctl_enum_fmt_cap - V4L2 sensor interface handler for VIDIOC_ENUM_FMT
 * @s: pointer to standard V4L2 device structure
 * @fmt: pointer to standard V4L2 fmt description structure
 *
 * Return 0.
 */
static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
			      struct v4l2_fmtdesc *fmt)
{
	if (fmt->index > gc0308_mode_MAX)
		return -EINVAL;

	fmt->pixelformat = gc0308_data.pix.pixelformat;

	return 0;
}

/*!
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct sensor_data *sensor = s->priv;
	u32 tgt_xclk;	/* target xclk */
	u32 tgt_fps;	/* target frames per secound */
	enum gc0308_frame_rate frame_rate;

	gc0308_data.on = true;

	printk("ioctl_dev_init: >>>>>>>>>>>>>>>>>>>>\n");
	/* mclk */
	tgt_xclk = gc0308_data.mclk;
	tgt_xclk = min(tgt_xclk, (u32)GC0308_XCLK_MAX);
	tgt_xclk = max(tgt_xclk, (u32)GC0308_XCLK_MIN);
	gc0308_data.mclk = tgt_xclk;

	pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);
	//set_mclk_rate(&gc0308_data.mclk, gc0308_data.csi);
	clk_set_rate(gc0308_data.sensor_clk, gc0308_data.mclk);

	/* Default camera frame rate is set in probe */
	tgt_fps = sensor->streamcap.timeperframe.denominator /
		  sensor->streamcap.timeperframe.numerator;

	frame_rate = gc0308_30_fps;

	printk("ioctl_dev_init: <<<<<<<<<<<<<<<<<<<<<<<<\n");

	return gc0308_init_mode(frame_rate,
				sensor->streamcap.capturemode);
}

/*!
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the device when slave detaches to the master.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	return 0;
}

/*!
 * This structure defines all the ioctls for this module and links them to the
 * enumeration.
 */
static struct v4l2_int_ioctl_desc gc0308_ioctl_desc[] = {
	{vidioc_int_dev_init_num, (v4l2_int_ioctl_func *)ioctl_dev_init},
	{vidioc_int_dev_exit_num, ioctl_dev_exit},
	{vidioc_int_s_power_num, (v4l2_int_ioctl_func *)ioctl_s_power},
	{vidioc_int_g_ifparm_num, (v4l2_int_ioctl_func *)ioctl_g_ifparm},
/*	{vidioc_int_g_needs_reset_num,
				(v4l2_int_ioctl_func *)ioctl_g_needs_reset}, */
/*	{vidioc_int_reset_num, (v4l2_int_ioctl_func *)ioctl_reset}, */
	{vidioc_int_init_num, (v4l2_int_ioctl_func *)ioctl_init},
	{vidioc_int_enum_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_enum_fmt_cap},
/*	{vidioc_int_try_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_try_fmt_cap}, */
	{vidioc_int_g_fmt_cap_num, (v4l2_int_ioctl_func *)ioctl_g_fmt_cap},
/*	{vidioc_int_s_fmt_cap_num, (v4l2_int_ioctl_func *)ioctl_s_fmt_cap}, */
	{vidioc_int_g_parm_num, (v4l2_int_ioctl_func *)ioctl_g_parm},
	{vidioc_int_s_parm_num, (v4l2_int_ioctl_func *)ioctl_s_parm},
/*	{vidioc_int_queryctrl_num, (v4l2_int_ioctl_func *)ioctl_queryctrl}, */
	{vidioc_int_g_ctrl_num, (v4l2_int_ioctl_func *)ioctl_g_ctrl},
	{vidioc_int_s_ctrl_num, (v4l2_int_ioctl_func *)ioctl_s_ctrl},
	{vidioc_int_enum_framesizes_num,
				(v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{vidioc_int_g_chip_ident_num,
				(v4l2_int_ioctl_func *)ioctl_g_chip_ident},
};

static struct v4l2_int_slave gc0308_slave = {
	.ioctls = gc0308_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(gc0308_ioctl_desc),
};

static struct v4l2_int_device gc0308_int_device = {
	.module = THIS_MODULE,
	.name = "gc0308",
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &gc0308_slave,
	},
};


//LJH  static int get_device_id(struct i2c_client *client)
static int get_device_id(void)
{
	u8 au8RegBuf[2] = {0};
	u8 u8RdVal = 0;
	au8RegBuf[0] = 0x00;

	if (1 != i2c_master_send(gc0308_data.i2c_client, au8RegBuf, 1)) {
		pr_err("%s:write reg error:reg=%x\n",
				__func__, 0xfb);
		return -1;
	}

	if (1 != i2c_master_recv(gc0308_data.i2c_client, &u8RdVal, 1)) {
		pr_err("%s:read reg error:reg=%x,val=%x\n",
				__func__, 0xfb, u8RdVal);
		return -1;
	}

//	u8RdVal = i2c_smbus_read_byte_data(gc0308_data.i2c_client, 0x00);
	printk(KERN_INFO "u8RdVal=%x\n\n", u8RdVal);

	return u8RdVal;
}

/*!
 * gc0308 I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int gc0308_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int retval;
	struct device *dev = &client->dev;
	struct pinctrl *pinctrl;

	pinctrl = devm_pinctrl_get_select_default(dev);
	if(IS_ERR(pinctrl)){
		dev_err(dev, "setup pinctrl failed\n");
		return PTR_ERR(pinctrl);
	}
	
	/*request power down pin */
	pwn_gpio = of_get_named_gpio(dev->of_node, "pwn-gpios", 0);
	if(!gpio_is_valid(pwn_gpio)) {
		dev_err(dev, "no senor pwdn pin availbale\n");
		return -ENODEV;
	}
	retval = devm_gpio_request_one(dev, pwn_gpio, GPIOF_OUT_INIT_HIGH,
					"cam-pwdn");
	if (retval < 0)
		return retval;

	/* request reset pin */
	rst_gpio = of_get_named_gpio(dev->of_node, "rst-gpios", 0);
	if (!gpio_is_valid(rst_gpio)) {
		dev_err(dev, "no sensor reset pin available\n");
		return -EINVAL;
	}
	retval = devm_gpio_request_one(dev, rst_gpio, GPIOF_OUT_INIT_HIGH,
					"cam-reset");
	if (retval < 0)
		return retval;
	
	printk("gc0308_probe: >>>>>>>>>>>>>>>>>>>>>>>\n");

	/* Set initial values for the sensor struct. */
	memset(&gc0308_data, 0, sizeof(gc0308_data));
	/*++++LJH  begin+++++*/
	gc0308_data.sensor_clk = devm_clk_get(dev, "csi_mclk");
	if (IS_ERR(gc0308_data.sensor_clk)) {
		dev_err(dev, "get mclk failed\n");
		return PTR_ERR(gc0308_data.sensor_clk);
	}

	retval = of_property_read_u32(dev->of_node, "mclk", &gc0308_data.mclk);
	
	printk("%s %s %d mclk = %d\n", __FILE__,  __func__, __LINE__ , gc0308_data.mclk);
	
	if (retval) {
		dev_err(dev, "mclk frequency is invalid\n");
		return retval;
	}

	retval = of_property_read_u32(dev->of_node, "mclk_source",
					(u32 *) &(gc0308_data.mclk_source));
	if (retval) {
		dev_err(dev, "mclk_source invalid\n");
		return retval;
	}

	retval = of_property_read_u32(dev->of_node, "csi_id", &(gc0308_data.csi));

	if (retval) {
		dev_err(dev, "csi_id invalid\n");
		return retval;
	}

	printk("%s %s %d csi= %d\n", __FILE__,  __func__, __LINE__, gc0308_data.csi );

    clk_prepare_enable(gc0308_data.sensor_clk);

	/*++++LJH  end +++++*/

	gc0308_data.io_init = mx6q_csi0_io_init;

	gc0308_data.i2c_client = client;
	gc0308_data.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	gc0308_data.pix.width = 640;
	gc0308_data.pix.height = 480;
	gc0308_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	gc0308_data.streamcap.capturemode = 0;
	gc0308_data.streamcap.timeperframe.denominator = FPS_DENOMINATOR;
	gc0308_data.streamcap.timeperframe.numerator = FPS_NUMERATOR;

	/*+++  LJH  ++++*/
	//gc0308_regulator_enable(&client->dev);

	/*Set clock  ++++++++++   LJH*/
	//clk_set_rate(gc0308_data.sensor_clk, gc0308_data.mclk);
	mx6q_csi0_io_init();

	mx6q_csi0_cam_powerdown(0);
	if( get_device_id() == -1 )
	{
		clk_disable_unprepare(gc0308_data.sensor_clk);
		printk(KERN_ERR "get_device_id: error, not GC0308");
		return -1;
	}

//	mx6q_csi0_cam_powerdown(1);

//	clk_disable_unprepare(gc0308_data.sensor_clk);

#if 0
	if (plat_data->io_regulator) {
		io_regulator = regulator_get(&client->dev,
					     plat_data->io_regulator);
		if (!IS_ERR(io_regulator)) {
			regulator_set_voltage(io_regulator,
					      GC0308_VOLTAGE_DIGITAL_IO,
					      OV5640_VOLTAGE_DIGITAL_IO);
			if (regulator_enable(io_regulator) != 0) {
				pr_err("%s:io set voltage error\n", __func__);
				goto err1;
			} else {
				dev_dbg(&client->dev,
					"%s:io set voltage ok\n", __func__);
			}
		} else
			io_regulator = NULL;
	}

	if (plat_data->core_regulator) {
		core_regulator = regulator_get(&client->dev,
					       plat_data->core_regulator);
		if (!IS_ERR(core_regulator)) {
			regulator_set_voltage(core_regulator,
					      GC0308_VOLTAGE_DIGITAL_CORE,
					      GC0308_VOLTAGE_DIGITAL_CORE);
			if (regulator_enable(core_regulator) != 0) {
				pr_err("%s:core set voltage error\n", __func__);
				goto err2;
			} else {
				dev_dbg(&client->dev,
					"%s:core set voltage ok\n", __func__);
			}
		} else
			core_regulator = NULL;
	}

	if (plat_data->analog_regulator) {
		analog_regulator = regulator_get(&client->dev,
						 plat_data->analog_regulator);
		if (!IS_ERR(analog_regulator)) {
			regulator_set_voltage(analog_regulator,
					      GC0308_VOLTAGE_ANALOG,
					      GC0308_VOLTAGE_ANALOG);
			if (regulator_enable(analog_regulator) != 0) {
				pr_err("%s:analog set voltage error\n",
					__func__);
				goto err3;
			} else {
				dev_dbg(&client->dev,
					"%s:analog set voltage ok\n", __func__);
			}
		} else
			analog_regulator = NULL;
	}
#endif
	//if (plat_data->io_init)
		//plat_data->io_init();

	//camera_plat = plat_data;

	gc0308_int_device.priv = &gc0308_data;
	retval = v4l2_int_device_register(&gc0308_int_device);

	printk("gc0308_probe: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< retval=%d\n", retval);
	return retval;
/*
err3:
	if (core_regulator) {
		regulator_disable(core_regulator);
		regulator_put(core_regulator);
	}
err2:
	if (io_regulator) {
		regulator_disable(io_regulator);
		regulator_put(io_regulator);
	}
err1:
	return -1;
	*/
}

/*!
 * gc0308 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int gc0308_remove(struct i2c_client *client)
{
	v4l2_int_device_unregister(&gc0308_int_device);
/*
	if (gpo_regulator) {
		regulator_disable(gpo_regulator);
		regulator_put(gpo_regulator);
	}

	if (analog_regulator) {
		regulator_disable(analog_regulator);
		regulator_put(analog_regulator);
	}

	if (core_regulator) {
		regulator_disable(core_regulator);
		regulator_put(core_regulator);
	}

	if (io_regulator) {
		regulator_disable(io_regulator);
		regulator_put(io_regulator);
	}
*/
	return 0;
}

/*!
 * gc0308 init function
 * Called by insmod gc0308_camera.ko.
 *
 * @return  Error code indicating success or failure
 */
static __init int gc0308_init(void)
{
	u8 err;
	int mclk, csi;

	mclk = 24000000;
	csi = 0;

	//set_mclk_rate(&mclk, csi);
	printk("gc0308_init: mclk=%d\n", mclk);
	printk("gc0308_init: >>>>>>>>>>>>>>>>>>>>>>>\n");
	err = i2c_add_driver(&gc0308_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d \n",
			__func__, err);

	return err;
}

/*!
 * GC0308 cleanup function
 * Called on rmmod gc0308_camera.ko
 *
 * @return  Error code indicating success or failure
 */
static void __exit gc0308_clean(void)
{
	i2c_del_driver(&gc0308_i2c_driver);
}

module_init(gc0308_init);
module_exit(gc0308_clean);
//module_i2c_driver(gc0308_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("GC0308 Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");
