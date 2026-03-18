/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "a1102wxfrontsc521cs_mipi_raw.h"

#define PFX "a1102wxfrontsc521cs_camera"
#define LOG_INF(format, args...)    \
	pr_info(PFX "[%s] " format, __func__, ##args)

#define sc521cs_LY_SENSOR_GAIN_MAX_VALID_INDEX  6
#define sc521cs_LY_SENSOR_GAIN_MAP_SIZE         6

#define SC521CS_BASEGAIN           64
#define SC521CS_MAX_GAIN          (15.5 * SC521CS_BASEGAIN)  // 992

#define MODULE_GROUP_FLAG 0x8866
#define MODULE_GROUP2_FLAG 0x80B6
#define AWB_GROUP_FLAG 0x8885
#define AWB_GROUP2_FLAG 0x80D5
#define LSC_GROUP_FLAG 0x88AD

#define MODULE_INFO_FLAG 0x8867
#define AWB_INFO_FLAG 0x8886
#define LSC_INFO_FLAG 0x88AE

#define MODULE_LENGTH 10
#define AWB_LENGTH 15
#define LSC_LENGTH 1869

#define SC521CS_SYX_RET_SUCCESS		0
#define SC521CS_SYX_RET_FAIL		1
#define SC521CS_SYX_DEBUG_ON		0

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static char otpData[MODULE_LENGTH+AWB_LENGTH+LSC_LENGTH]={0};

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = A1102WXFRONTSC521CS_SENSOR_ID,
	.checksum_value = 0x55e2a82f,
	.pre = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 172800000,
	},
	.cap = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 172800000,
	},
	.cap1 = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 172800000,
	},
	.normal_video = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 180000000,
	},
	.hs_video = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1458,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 172800000,
	},
	.slim_video = {
		.pclk = 90000000,
		.linelength = 1488,
		.framelength = 2016,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
		.mipi_pixel_rate = 172800000,
	},

	.margin = 8,            //sensor framelength & shutter margin
	.min_shutter = 4,
    .min_gain = 64,
    .max_gain = 992,//15.5
    .min_gain_iso = 100,
    .gain_step = 1,
    .gain_type = 3,
	.max_frame_length = 0x7FFFF,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,     // //1, support; 0,not support
	.ihdr_le_firstline = 0, // //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num

    .pre_delay_frame = 2,    /* enter preview delay frame num */
    .cap_delay_frame = 3,    /* enter capture delay frame num */
    .video_delay_frame = 2,    /* enter normal_video delay frame num */
    .hs_video_delay_frame = 2,    /* enter high_speed_video delay frame num */
    .slim_video_delay_frame = 2,    /* enter slim_video delay frame num */
    .frame_time_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 0,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
    .mclk = 24, /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_2_LANE,
	.i2c_addr_table = {0x20,0x6c,0xff},
	.i2c_speed = 400,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3d0,                   //current shutter   // Danbo ??
	.gain = 0x40,                      //current gain     // Danbo ??
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE, 
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .i2c_write_id = 0x20,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{
 { 2592, 1944,	  0,	0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944,    0,	0, 2592, 1944}, // Preview 
 { 2592, 1944,	  0,	0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944,    0,	0, 2592, 1944}, // capture 
 { 2592, 1944,	  0,	0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944,    0,	0, 2592, 1944}, // video
 { 2592, 1944,	  0,  243, 2592, 1458, 2592, 1458, 0, 0, 2592, 1458,    0,	0, 2592, 1458}, //hight speed video 
 { 2592, 1944,	  0,	0, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944,    0,	0, 2592, 1944}};// slim video

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	
    //kdSetI2CSpeed(400); 

	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("frame length = %d\n", imgsensor.frame_length);
	write_cmos_sensor(0x320e, (imgsensor.frame_length >> 8) & 0xff);
	write_cmos_sensor(0x320f, imgsensor.frame_length & 0xff);
}

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x3107) << 8) | read_cmos_sensor(0x3108)); //0xeb15
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    LOG_INF("framerate = %d, min framelength should enable? \n", framerate);
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
    imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}

/*************************************************************************
 * FUNCTION
 *	set_shutter
 *
 * DESCRIPTION
 *	This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *	iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
    kal_uint16 realtime_fps = 0;

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
	
	// if shutter bigger than frame_length, should extend frame length first
	//printk("pangfei shutter %d line %d\n",shutter,__LINE__);
	spin_lock(&imgsensor_drv_lock);

	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)		
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
  
    if (imgsensor.autoflicker_en) { 
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        // Extend frame length
		write_cmos_sensor(0x320e, (imgsensor.frame_length >> 8) & 0xff);
		write_cmos_sensor(0x320f, imgsensor.frame_length & 0xFF);
            }
    } else {
        // Extend frame length
		write_cmos_sensor(0x320e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x320f, imgsensor.frame_length & 0xFF);
    }
    // Update Shutter
    shutter = shutter *2;
	write_cmos_sensor(0x3e00, (shutter >> 12) & 0xFF);
	write_cmos_sensor(0x3e01, (shutter >> 4)&0xFF);
	write_cmos_sensor(0x3e02, (shutter<<4) & 0xF0);	
	LOG_INF("Exit! shutter = %d, framelength = %d\n", shutter, imgsensor.frame_length);
}
/*
static kal_uint16 gain2reg(const kal_uint16 gain)
{
	LOG_INF(" gain2reg_in= %d\n", gain);
	kal_uint16 reg_gain = gain << 4;
	LOG_INF(" gain2reg_out= %d\n", gain);

	return (kal_uint16)reg_gain;
}
*/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 ana_gain;
	kal_uint16 dig_gain;
	kal_uint16 ana_real_gain;

	// 增益范围限制 (1x ~ 15.5)
	if (gain < SC521CS_BASEGAIN) {
		gain = SC521CS_BASEGAIN;
	} else if (gain > SC521CS_MAX_GAIN) {
		gain = SC521CS_MAX_GAIN;
	}

	LOG_INF(" set_gain_in= %d\n", gain);

	if (gain < 2 * SC521CS_BASEGAIN) {
		ana_gain = 0x00;
		ana_real_gain = gain * 16 / SC521CS_BASEGAIN;
		dig_gain = gain * 32 / ana_real_gain;
	} else if (gain < 4 * SC521CS_BASEGAIN) {
		ana_gain = 0x01;
		ana_real_gain = gain * 16 / (2 * SC521CS_BASEGAIN);
		dig_gain = gain * 16 / ana_real_gain;
	} else if (gain < 8 * SC521CS_BASEGAIN) {
		ana_gain = 0x03;
		ana_real_gain = gain * 16 / (4 * SC521CS_BASEGAIN);
		dig_gain = gain * 8 / ana_real_gain;
	} else if (gain < SC521CS_MAX_GAIN) {
		ana_gain = 0x07;
		ana_real_gain = gain * 16 / (8 * SC521CS_BASEGAIN);
		dig_gain = gain * 4 / ana_real_gain;
	} else {
		ana_gain = 0x07;
		ana_real_gain = 31;
		dig_gain = 128;
	}
	write_cmos_sensor(0x3e08, ana_gain);
	write_cmos_sensor(0x3e09, ana_real_gain);
	write_cmos_sensor(0x3e07, dig_gain & 0xFF);

	LOG_INF("gain = %d, ana_gain = 0x%02x, ana_real_gain = 0x%02x, dig_gain = 0x%02x, "
			"read: 0x3e06=0x%02x, 0x3e09=0x%02x, 0x3e07=0x%02x\n",
			gain,ana_gain,ana_real_gain,dig_gain,
			read_cmos_sensor(0x3e06),read_cmos_sensor(0x3e09),read_cmos_sensor(0x3e07));

	return gain;
}

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le: 0x%x, se: 0x%x, gain: 0x%x\n", le, se, gain);
}


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);
}
#endif
/*************************************************************************
 * FUNCTION
 *	night_mode
 *
 * DESCRIPTION
 *	This function night mode of sensor.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(kal_bool enable)
{
	/* No Need to implement this function */
}

static void sensor_init(void)
{
	printk("rongyi");
	LOG_INF(">> %s()\n", __func__);
//Sensor Information////////////////////////////
//Image size	  : 2592 x 1944
//MCLK/PCLK	  : 24MHz /90MHZ
//MIPI speed(Mbps): 900Mbps x 2Lane
//Frame Length	  :  2016
//Line Length 	  : 1488
//line Time       :13333
//Max Fps 	  : 30.00fps
//Pixel order 	  : Green 1st (=B)
//X/Y-flip        : X-flip
//BLC offset	    : 64code
//Firmware Ver.   : v1.0
////////////////////////////////////////////////
	write_cmos_sensor(0x0103, 0x01);
	write_cmos_sensor(0x37e9, 0x80);
	write_cmos_sensor(0x37f9, 0x80);
	write_cmos_sensor(0x37ea, 0x12);
	write_cmos_sensor(0x37eb, 0x0c);
	write_cmos_sensor(0x37ec, 0x4a);
	write_cmos_sensor(0x37ed, 0x18);
	write_cmos_sensor(0x301f, 0x07);
	write_cmos_sensor(0x3058, 0x10);
	write_cmos_sensor(0x3059, 0x32);
	write_cmos_sensor(0x30b8, 0x22);
	write_cmos_sensor(0x3106, 0x01);
	write_cmos_sensor(0x320c, 0x05);
	write_cmos_sensor(0x320d, 0xd0);
	write_cmos_sensor(0x320e, 0x07);
	write_cmos_sensor(0x320f, 0xe0);
	write_cmos_sensor(0x3221, 0x66);
	write_cmos_sensor(0x5005, 0x08);
	write_cmos_sensor(0x3253, 0x08);//
	write_cmos_sensor(0x325f, 0x18);//
	write_cmos_sensor(0x3301, 0x08);
	write_cmos_sensor(0x3304, 0x80);
	write_cmos_sensor(0x3305, 0x00);
	write_cmos_sensor(0x3306, 0x48);
	write_cmos_sensor(0x3309, 0x80);
	write_cmos_sensor(0x330a, 0x00);
	write_cmos_sensor(0x330b, 0xb8);
	write_cmos_sensor(0x330e, 0x30);
	write_cmos_sensor(0x331e, 0x71);
	write_cmos_sensor(0x331f, 0x71);
	write_cmos_sensor(0x3333, 0x10);
	write_cmos_sensor(0x3334, 0x40);
	write_cmos_sensor(0x335d, 0x60);
	write_cmos_sensor(0x3364, 0x5e);
	write_cmos_sensor(0x3390, 0x08);
	write_cmos_sensor(0x3391, 0x18);
	write_cmos_sensor(0x3393, 0x10);
	write_cmos_sensor(0x3394, 0x16);
	write_cmos_sensor(0x33ad, 0x2c);
	write_cmos_sensor(0x33b1, 0x80);
	write_cmos_sensor(0x33b2, 0x58);
	write_cmos_sensor(0x33b3, 0x48);
	write_cmos_sensor(0x33f8, 0x00);
	write_cmos_sensor(0x33f9, 0x48);
	write_cmos_sensor(0x33fa, 0x00);
	write_cmos_sensor(0x33fb, 0x48);
	write_cmos_sensor(0x33fc, 0x18);
	write_cmos_sensor(0x33fd, 0x38);
	write_cmos_sensor(0x349f, 0x03);
	write_cmos_sensor(0x34a6, 0x18);
	write_cmos_sensor(0x34a7, 0x38);
	write_cmos_sensor(0x34a8, 0x48);
	write_cmos_sensor(0x34a9, 0x48);
	write_cmos_sensor(0x34aa, 0x00);
	write_cmos_sensor(0x34ab, 0xb8);
	write_cmos_sensor(0x34ac, 0x00);
	write_cmos_sensor(0x34ad, 0xb8);
	write_cmos_sensor(0x34fa, 0x18);
	write_cmos_sensor(0x34fb, 0x38);
	write_cmos_sensor(0x3618, 0x24);
	write_cmos_sensor(0x3630, 0x53);
	write_cmos_sensor(0x3632, 0x33);
	write_cmos_sensor(0x3633, 0x06);
	write_cmos_sensor(0x3636, 0x0c);
	write_cmos_sensor(0x3660, 0xc5);
	write_cmos_sensor(0x3661, 0xcd);
	write_cmos_sensor(0x3662, 0xe3);
	write_cmos_sensor(0x3667, 0x18);
	write_cmos_sensor(0x3668, 0x38);
	write_cmos_sensor(0x3686, 0x37);
	write_cmos_sensor(0x3687, 0x37);
	write_cmos_sensor(0x3688, 0x37);
	write_cmos_sensor(0x36cc, 0x08);
	write_cmos_sensor(0x36cd, 0x38);
	write_cmos_sensor(0x3718, 0x04);
	write_cmos_sensor(0x3722, 0x03);
	write_cmos_sensor(0x3724, 0xb1);
	write_cmos_sensor(0x3770, 0x07);
	write_cmos_sensor(0x3771, 0x43);
	write_cmos_sensor(0x3772, 0xc3);
	write_cmos_sensor(0x37c0, 0x18);
	write_cmos_sensor(0x37c1, 0x38);
	write_cmos_sensor(0x3900, 0x05);
	write_cmos_sensor(0x3901, 0x04);
	write_cmos_sensor(0x3907, 0x00);
	write_cmos_sensor(0x3908, 0x40);
	write_cmos_sensor(0x391a, 0x60);
	write_cmos_sensor(0x391b, 0x30);
	write_cmos_sensor(0x391c, 0x1d);
	write_cmos_sensor(0x391d, 0x00);
	write_cmos_sensor(0x391f, 0x41);
	write_cmos_sensor(0x3926, 0xe0);
	write_cmos_sensor(0x39b2, 0x00);
	write_cmos_sensor(0x39dd, 0x00);
	write_cmos_sensor(0x39de, 0x04);
	write_cmos_sensor(0x39e7, 0x04);
	write_cmos_sensor(0x39e8, 0x06);
	write_cmos_sensor(0x39e9, 0x80);
	write_cmos_sensor(0x3e00, 0x00);
	write_cmos_sensor(0x3e01, 0xf9);
	write_cmos_sensor(0x3e02, 0x80);
	write_cmos_sensor(0x4401, 0x11);
	write_cmos_sensor(0x4402, 0x02);
	write_cmos_sensor(0x4403, 0x09);
	write_cmos_sensor(0x4404, 0x1b);
	write_cmos_sensor(0x4405, 0x24);
	write_cmos_sensor(0x4407, 0x10);
	write_cmos_sensor(0x440c, 0x2d);
	write_cmos_sensor(0x440d, 0x2d);
	write_cmos_sensor(0x440e, 0x22);
	write_cmos_sensor(0x440f, 0x39);
	write_cmos_sensor(0x4412, 0x01);
	write_cmos_sensor(0x4424, 0x01);
	write_cmos_sensor(0x4509, 0x20);
	write_cmos_sensor(0x450d, 0x61);
	write_cmos_sensor(0x4816, 0x21);
	write_cmos_sensor(0x4819, 0x0d);
	write_cmos_sensor(0x481b, 0x07);
	write_cmos_sensor(0x481d, 0x1d);
	write_cmos_sensor(0x481f, 0x06);
	write_cmos_sensor(0x4821, 0x0d);
	write_cmos_sensor(0x4823, 0x07);
	write_cmos_sensor(0x4825, 0x06);
	write_cmos_sensor(0x4827, 0x06);
	write_cmos_sensor(0x4829, 0x0b);
	write_cmos_sensor(0x5000, 0x0e);
	write_cmos_sensor(0x550e, 0x00);
	write_cmos_sensor(0x550f, 0x42);
	write_cmos_sensor(0x37e9, 0x24);
	write_cmos_sensor(0x37f9, 0x23);		
//	write_cmos_sensor(0x3c01, 0x14);
//	write_cmos_sensor(0x3c09, 0xcb);
//	write_cmos_sensor(0x3752, 0x33);
//	write_cmos_sensor(0x3754, 0x02);	

}

static void preview_setting(void)
{
	write_cmos_sensor(0x0103, 0x01);
	write_cmos_sensor(0x37e9, 0x80);
	write_cmos_sensor(0x37f9, 0x80);
	write_cmos_sensor(0x37ea, 0x12);
	write_cmos_sensor(0x37eb, 0x0c);
	write_cmos_sensor(0x37ec, 0x4a);
	write_cmos_sensor(0x37ed, 0x18);
	write_cmos_sensor(0x301f, 0x07);
	write_cmos_sensor(0x3058, 0x10);
	write_cmos_sensor(0x3059, 0x32);
	write_cmos_sensor(0x30b8, 0x22);
	write_cmos_sensor(0x3106, 0x01);
	write_cmos_sensor(0x320c, 0x05);
	write_cmos_sensor(0x320d, 0xd0);
	write_cmos_sensor(0x320e, 0x07);
	write_cmos_sensor(0x320f, 0xe0);
	write_cmos_sensor(0x3221, 0x66);
	write_cmos_sensor(0x5005, 0x08);
	write_cmos_sensor(0x3253, 0x08);
	write_cmos_sensor(0x325f, 0x18);
	write_cmos_sensor(0x3301, 0x08);
	write_cmos_sensor(0x3304, 0x80);
	write_cmos_sensor(0x3305, 0x00);
	write_cmos_sensor(0x3306, 0x48);
	write_cmos_sensor(0x3309, 0x80);
	write_cmos_sensor(0x330a, 0x00);
	write_cmos_sensor(0x330b, 0xb8);
	write_cmos_sensor(0x330e, 0x30);
	write_cmos_sensor(0x331e, 0x71);
	write_cmos_sensor(0x331f, 0x71);
	write_cmos_sensor(0x3333, 0x10);
	write_cmos_sensor(0x3334, 0x40);
	write_cmos_sensor(0x335d, 0x60);
	write_cmos_sensor(0x3364, 0x5e);
	write_cmos_sensor(0x3390, 0x08);
	write_cmos_sensor(0x3391, 0x18);
	write_cmos_sensor(0x3393, 0x10);
	write_cmos_sensor(0x3394, 0x16);
	write_cmos_sensor(0x33ad, 0x2c);
	write_cmos_sensor(0x33b1, 0x80);
	write_cmos_sensor(0x33b2, 0x58);
	write_cmos_sensor(0x33b3, 0x48);
	write_cmos_sensor(0x33f8, 0x00);
	write_cmos_sensor(0x33f9, 0x48);
	write_cmos_sensor(0x33fa, 0x00);
	write_cmos_sensor(0x33fb, 0x48);
	write_cmos_sensor(0x33fc, 0x18);
	write_cmos_sensor(0x33fd, 0x38);
	write_cmos_sensor(0x349f, 0x03);
	write_cmos_sensor(0x34a6, 0x18);
	write_cmos_sensor(0x34a7, 0x38);
	write_cmos_sensor(0x34a8, 0x48);
	write_cmos_sensor(0x34a9, 0x48);
	write_cmos_sensor(0x34aa, 0x00);
	write_cmos_sensor(0x34ab, 0xb8);
	write_cmos_sensor(0x34ac, 0x00);
	write_cmos_sensor(0x34ad, 0xb8);
	write_cmos_sensor(0x34fa, 0x18);
	write_cmos_sensor(0x34fb, 0x38);
	write_cmos_sensor(0x3618, 0x24);
	write_cmos_sensor(0x3630, 0x53);
	write_cmos_sensor(0x3632, 0x33);
	write_cmos_sensor(0x3633, 0x06);
	write_cmos_sensor(0x3636, 0x0c);
	write_cmos_sensor(0x3660, 0xc5);
	write_cmos_sensor(0x3661, 0xcd);
	write_cmos_sensor(0x3662, 0xe3);
	write_cmos_sensor(0x3667, 0x18);
	write_cmos_sensor(0x3668, 0x38);
	write_cmos_sensor(0x3686, 0x37);
	write_cmos_sensor(0x3687, 0x37);
	write_cmos_sensor(0x3688, 0x37);
	write_cmos_sensor(0x36cc, 0x08);
	write_cmos_sensor(0x36cd, 0x38);
	write_cmos_sensor(0x3718, 0x04);
	write_cmos_sensor(0x3722, 0x03);
	write_cmos_sensor(0x3724, 0xb1);
	write_cmos_sensor(0x3770, 0x07);
	write_cmos_sensor(0x3771, 0x43);
	write_cmos_sensor(0x3772, 0xc3);
	write_cmos_sensor(0x37c0, 0x18);
	write_cmos_sensor(0x37c1, 0x38);
	write_cmos_sensor(0x3900, 0x05);
	write_cmos_sensor(0x3901, 0x04);
	write_cmos_sensor(0x3907, 0x00);
	write_cmos_sensor(0x3908, 0x40);
	write_cmos_sensor(0x391a, 0x60);
	write_cmos_sensor(0x391b, 0x30);
	write_cmos_sensor(0x391c, 0x1d);
	write_cmos_sensor(0x391d, 0x00);
	write_cmos_sensor(0x391f, 0x41);
	write_cmos_sensor(0x3926, 0xe0);
	write_cmos_sensor(0x39b2, 0x00);
	write_cmos_sensor(0x39dd, 0x00);
	write_cmos_sensor(0x39de, 0x04);
	write_cmos_sensor(0x39e7, 0x04);
	write_cmos_sensor(0x39e8, 0x06);
	write_cmos_sensor(0x39e9, 0x80);
	write_cmos_sensor(0x3e00, 0x00);
	write_cmos_sensor(0x3e01, 0xf9);
	write_cmos_sensor(0x3e02, 0x80);
	write_cmos_sensor(0x4401, 0x11);
	write_cmos_sensor(0x4402, 0x02);
	write_cmos_sensor(0x4403, 0x09);
	write_cmos_sensor(0x4404, 0x1b);
	write_cmos_sensor(0x4405, 0x24);
	write_cmos_sensor(0x4407, 0x10);
	write_cmos_sensor(0x440c, 0x2d);
	write_cmos_sensor(0x440d, 0x2d);
	write_cmos_sensor(0x440e, 0x22);
	write_cmos_sensor(0x440f, 0x39);
	write_cmos_sensor(0x4412, 0x01);
	write_cmos_sensor(0x4424, 0x01);
	write_cmos_sensor(0x4509, 0x20);
	write_cmos_sensor(0x450d, 0x61);
	write_cmos_sensor(0x4816, 0x21);
	write_cmos_sensor(0x4819, 0x0d);
	write_cmos_sensor(0x5000, 0x0e);
	write_cmos_sensor(0x550e, 0x00);
	write_cmos_sensor(0x550f, 0x42);
	write_cmos_sensor(0x37e9, 0x24);
	write_cmos_sensor(0x37f9, 0x23);		
//	write_cmos_sensor(0x3c01, 0x14);
//	write_cmos_sensor(0x3c09, 0xcb);
//	write_cmos_sensor(0x3752, 0x33);
//	write_cmos_sensor(0x3754, 0x02);	

}

static void capture_setting(void)
{
	preview_setting();
}

static void normal_video_setting(void)
{
	/*V02P08_20210628*/
	preview_setting();
}

static void hs_video_setting(void)
{
	write_cmos_sensor(0x0100,0x00);
	write_cmos_sensor(0x301f,0x04);
	write_cmos_sensor(0x3200,0x00);
	write_cmos_sensor(0x3201,0x00);
	write_cmos_sensor(0x3202,0x00);
	write_cmos_sensor(0x3203,0x00);
	write_cmos_sensor(0x3204,0x0a);
	write_cmos_sensor(0x3205,0x27);
	write_cmos_sensor(0x3206,0x07);
	write_cmos_sensor(0x3207,0x9f);
	write_cmos_sensor(0x3208,0x0a);
	write_cmos_sensor(0x3209,0x20);
	write_cmos_sensor(0x320a,0x07);
	write_cmos_sensor(0x320b,0x98);
	write_cmos_sensor(0x320c,0x05);
	write_cmos_sensor(0x320d,0xd0);
	write_cmos_sensor(0x320e,0x07);
	write_cmos_sensor(0x320f,0xe0);
	write_cmos_sensor(0x3210,0x00);
	write_cmos_sensor(0x3211,0x04);
	write_cmos_sensor(0x3212,0x00);
	write_cmos_sensor(0x3213,0x04);
	write_cmos_sensor(0x3220,0x00);
	write_cmos_sensor(0x321a,0x11);
	write_cmos_sensor(0x3215,0x11);
	write_cmos_sensor(0x5000,0x0e);
	write_cmos_sensor(0x5900,0x01);
	write_cmos_sensor(0x5901,0x00);
	write_cmos_sensor(0x3c01,0x14);
	write_cmos_sensor(0x3c09,0xcb);
}


static void slim_video_setting(void)
{
//Sensor Information////////////////////////////
	preview_setting();
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		write_cmos_sensor(0x301f, 0x04);
		/*write_cmos_sensor(0x0100, 0x00);
		write_cmos_sensor(0x3902, 0x80);
		write_cmos_sensor(0x3909, 0xff);
		write_cmos_sensor(0x390a, 0xff);
		write_cmos_sensor(0x391f, 0xc8);
		write_cmos_sensor(0x0100, 0x01);*/	
	}
	else {
		write_cmos_sensor(0x301f, 0x04);
		/*write_cmos_sensor(0x0100, 0x00);
		write_cmos_sensor(0x3902, 0xc0);
		write_cmos_sensor(0x3909, 0x00);
		write_cmos_sensor(0x390a, 0x00);
		write_cmos_sensor(0x391f, 0xc9);
		write_cmos_sensor(0x0100, 0x01);*/
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static int sc521cs_set_threshold(u8 threshold)
{
	int threshold_reg1[3] ={ 0x90,0x90,0x90};
	int threshold_reg2[3] ={ 0x28,0x18,0x38};
	int threshold_reg3[3] ={ 0x42,0x72,0x02};
	write_cmos_sensor(0x36b0,threshold_reg1[threshold]);
	write_cmos_sensor(0x36b1,threshold_reg2[threshold]);
	write_cmos_sensor(0x36b2,threshold_reg3[threshold]);
	LOG_INF("sc521cs_set_threshold %d\n", threshold);
	return SC521CS_SYX_RET_SUCCESS;
}

static int sc521cs_load_data(unsigned int start_addr,unsigned int end_addr)
{
	int delay = 0;

	LOG_INF("sc521cs_load_data start_addr=0x%x, end_addr=0x%x\n", start_addr, end_addr);

	write_cmos_sensor(0x4408,(start_addr>>8)&0x0f);
	write_cmos_sensor(0x4409,start_addr&0xff);
	write_cmos_sensor(0x440a,(end_addr>>8)&0x0f);
	write_cmos_sensor(0x440b,end_addr&0xff);

	write_cmos_sensor(0x4401,0x11);

	if(start_addr == 0x8000)
	{
		write_cmos_sensor(0x4412,0x01);
		write_cmos_sensor(0x4407,0x10);
	}else if(start_addr == 0x8400)
	{
		write_cmos_sensor(0x4412,0x03);
		write_cmos_sensor(0x4407,0x00);
	}else if(start_addr == 0x8800)
	{
		write_cmos_sensor(0x4412,0x05);
		write_cmos_sensor(0x4407,0x00);
	}else if(start_addr == 0x8C00)
	{
		write_cmos_sensor(0x4412,0x07);
		write_cmos_sensor(0x4407,0x00);
	}

	write_cmos_sensor(0x4400,0x11);
	mdelay(15);

	while((read_cmos_sensor(0x4420)&0x01) == 0x01)
	{
		delay++;
		LOG_INF("sc521cs_load_data waitting, OTP is still busy for loading %d times\n", delay);
		if(delay == 10)
		{
			LOG_INF("sc521cs_load_data fail, load timeout!!!\n");
			return SC521CS_SYX_RET_FAIL;
		}
		mdelay(10);
	}
	LOG_INF("sc521cs_load_data success\n");
	return SC521CS_SYX_RET_SUCCESS;
}

static int sc521cs_sensor_otp_read_data(unsigned int ui4_offset,unsigned int ui4_length, unsigned char *pinputdata)
{
	int i;
	unsigned int checksum_cal = 0;
	unsigned int checksum = 0;

	for(i=0; i<ui4_length; i++)
	{
		pinputdata[i] = read_cmos_sensor(ui4_offset + i);
		checksum_cal += pinputdata[i];
		 #if SC521CS_SYX_DEBUG_ON
		 LOG_INF("sc521cs_sensor_otp_read_data addr=0x%x, data=0x%x\n", ui4_offset + i, pinputdata[i]);
		 #endif
	}

	checksum = pinputdata[i-1];
	checksum_cal -= checksum;
	checksum_cal = checksum_cal%255+1;

	if(checksum_cal != checksum)
	{
		LOG_INF("sc521cs_sensor_otp_read_data checksum fail, checksum_cal=0x%x, checksum=0x%x\n", checksum_cal, checksum);
		return SC521CS_SYX_RET_FAIL;
	}

	LOG_INF("sc521cs_sensor_otp_read_data success\n");
	return SC521CS_SYX_RET_SUCCESS;
}
static void sc521cs_sensor_otp_read_lscdata(unsigned int ui4_offset,unsigned int ui4_length, unsigned char *pinputdata)
{
	int i;
	for(i=0; i<ui4_length; i++)
	{
		pinputdata[i] = read_cmos_sensor(ui4_offset + i);
		//checksum_cal += pinputdata[i];
		 #if SC521CS_SYX_DEBUG_ON
		 LOG_INF("sc521cs_sensor_otp_read_data addr=0x%x, data=0x%x\n", ui4_offset + i, pinputdata[i]);
		 #endif
	}
}
static int sc521cs_sensor_otp_lscdata_checksum(unsigned char *pinputdata)
{
	int i;
	unsigned int checksum_cal = 0;
	unsigned int checksum = 0;

	for(i=0; i<LSC_LENGTH; i++)
	{
		checksum_cal += pinputdata[i];
		 //#if SC521CS_SYX_DEBUG_ON
		 //LOG_INF("sc521cs_sensor_otp_read_data addr=0x%x, data=0x%x\n", ui4_offset + i, pinputdata[i]);
		 //#endif
	}

	checksum = pinputdata[i-1];
	checksum_cal -= checksum;
	checksum_cal = checksum_cal%255+1;

	if(checksum_cal != checksum)
	{
		LOG_INF("sc521cs_sensor_otp_read_data checksum fail, checksum_cal=0x%x, checksum=0x%x\n", checksum_cal, checksum);
		return SC521CS_SYX_RET_FAIL;
	}

	LOG_INF("sc521cs_sensor_otp_read_data success\n");
	return SC521CS_SYX_RET_SUCCESS;

}
static int sc521cs_sensor_otp_read_module_info(unsigned char *pinputdata)
{
	int ret = SC521CS_SYX_RET_FAIL;
	LOG_INF("sc521cs_sensor_otp_read_module_info begin!\n");

	if(read_cmos_sensor(0x808E) == 0x01 && read_cmos_sensor(0x808F) == 0x01)
	{
		LOG_INF("+! modele_info group1+++\n");
		ret = sc521cs_sensor_otp_read_data(0x8090, 10, pinputdata);
		return ret;
	}
	sc521cs_load_data(0x8000, 0x83FF);
	if(read_cmos_sensor(0x808E) == 0x07 && read_cmos_sensor(0x809A) == 0x01)
	{
		LOG_INF("++! modele_info group2\n");
		ret = sc521cs_sensor_otp_read_data(0x809B, 10, pinputdata);
		return ret;
	}
	return ret;
}

static int sc521cs_sensor_otp_read_awb_info(unsigned char *pinputdata)
{

	int ret = SC521CS_SYX_RET_FAIL;
	LOG_INF("sc521cs_sensor_otp_read_awb_info begin!\n");

	if(read_cmos_sensor(0x808E) == 0x01 && read_cmos_sensor(0x80A5) == 0x01)
	{
		LOG_INF("awb+! awb_info group1\n");
		ret = sc521cs_sensor_otp_read_data(0x80A6, 15, pinputdata);
		return ret;
	}
	sc521cs_load_data(0x8000, 0x83FF);
	if(read_cmos_sensor(0x808E) == 0x07 && read_cmos_sensor(0x80B5) == 0x01)
	{
		LOG_INF("awb++! awb_info group2\n");
		ret = sc521cs_sensor_otp_read_data(0x80B6, 15, pinputdata);
		return ret;
	}

	return ret;
}

static int sc521cs_sensor_otp_read_lsc_info(unsigned char *pinputdata)
{

	int ret = SC521CS_SYX_RET_FAIL;
	LOG_INF("sc521cs_sensor_otp_read_lsc_info begin!\n");

	if(read_cmos_sensor(0x808E) == 0x01 && read_cmos_sensor(0x80C5) == 0x01)
	{
		LOG_INF("lsc+! lsc_info group1\n");
		sc521cs_sensor_otp_read_lscdata(0x80C6, 826, pinputdata);

		sc521cs_load_data(0x8400, 0x87FF);
		sc521cs_sensor_otp_read_lscdata(0x8422, 990, pinputdata+826);

		sc521cs_load_data(0x8800, 0x8bFF);
		sc521cs_sensor_otp_read_lscdata(0x8822, 53, pinputdata+826+990);
		ret = sc521cs_sensor_otp_lscdata_checksum(pinputdata);

		return ret;
	}

	sc521cs_load_data(0x8000, 0x83FF);
	if(read_cmos_sensor(0x808E) == 0x07)
	{
		sc521cs_load_data(0x8800, 0x8bFF);
		if(read_cmos_sensor(0X8857) == 0x01)
		{
			LOG_INF("awb++! lsc_info group2\n");
			sc521cs_sensor_otp_read_lscdata(0x8858, 936, pinputdata);

			sc521cs_load_data(0x8C00, 0x8FFF);
			sc521cs_sensor_otp_read_lscdata(0X8C22, 933, pinputdata+936);
			ret = sc521cs_sensor_otp_lscdata_checksum(pinputdata);

			return ret;
		}
	}

	return ret;
}

static int sc521cs_sensor_otp_read_all_data(unsigned char *pinputdata)
{
	int ret = SC521CS_SYX_RET_FAIL;
	int threshold = 0;

	write_cmos_sensor(0x3106, 0x05);

	for(threshold=0; threshold<3; threshold++)
	{
		sc521cs_set_threshold(threshold);
		sc521cs_load_data(0x8000, 0x83FF);

		LOG_INF("read module info in treshold R%d\n", threshold);
		ret = sc521cs_sensor_otp_read_module_info(pinputdata);
		if(ret == SC521CS_SYX_RET_FAIL)
		{
			LOG_INF("read module info in treshold R%d fail\n", threshold);
		}
		else
		{
			LOG_INF("read module info in treshold R%d success\n", threshold);
			break;
		}
	}

	if(ret == SC521CS_SYX_RET_FAIL)
	{
		LOG_INF("read module info in treshold R1 R2 R3 all failed!!!\n");
		return ret;
	}

	LOG_INF("read awb info in treshold R%d\n", threshold);
	ret = sc521cs_sensor_otp_read_awb_info(pinputdata+MODULE_LENGTH);
	if(ret == SC521CS_SYX_RET_FAIL)
	{
		LOG_INF("read awb info in treshold R%d fail\n", threshold);
		for(threshold=0; threshold<3; threshold++)
		{
			sc521cs_set_threshold(threshold);
			sc521cs_load_data(0x8000, 0x83FF);

			LOG_INF("read awb info in treshold R%d\n", threshold);
			ret = sc521cs_sensor_otp_read_awb_info(pinputdata+MODULE_LENGTH);
			if(ret == SC521CS_SYX_RET_FAIL)
			{
				LOG_INF("read awb info in treshold R%d fail\n", threshold);
			}
			else
			{
				LOG_INF("read awb info in treshold R%d success\n", threshold);
				break;
			}
		}
	}

	if(ret == SC521CS_SYX_RET_FAIL)
	{
		LOG_INF("read awb info in treshold R1 R2 R3 all failed!!!\n");
		return ret;
	}
	LOG_INF("read awb info in treshold R%d success\n", threshold);
	LOG_INF("read lsc info in treshold R%d\n", threshold);
	ret = sc521cs_sensor_otp_read_lsc_info(pinputdata+MODULE_LENGTH+AWB_LENGTH);
	if(ret == SC521CS_SYX_RET_FAIL)
	{
		LOG_INF("read lsc info in treshold R%d fail\n", threshold);
		for(threshold=0; threshold<3; threshold++)
		{
			sc521cs_set_threshold(threshold);
			sc521cs_load_data(0x8000, 0x83FF);

			LOG_INF("read lsc info in treshold R%d\n", threshold);
			ret = sc521cs_sensor_otp_read_lsc_info(pinputdata+MODULE_LENGTH+AWB_LENGTH);
			if(ret == SC521CS_SYX_RET_FAIL)
			{
				LOG_INF("read lsc info in treshold R%d fail\n", threshold);
			}
			else
			{
				LOG_INF("read lsc info in treshold R%d success\n", threshold);
				break;
			}
		}
	}

	if(ret == SC521CS_SYX_RET_FAIL)
	{
		LOG_INF("read lsc info in treshold R1 R2 R3 all failed!!!\n");
		return ret;
	}
	LOG_INF("read lsc info in treshold R%d success\n", threshold);

	//load page 1
	write_cmos_sensor(0x4408,0x00);
	write_cmos_sensor(0x4409,0x00);
	write_cmos_sensor(0x440a,0x07);
	write_cmos_sensor(0x440b,0xff);

	write_cmos_sensor(0x3106, 0x01);

	return ret;
}

void check_sc521cs_otp(void)
{
	sc521cs_sensor_otp_read_all_data(otpData);
	sensor_init();
}

unsigned int a1102wxfrontsc521cs_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	int index = 0;
	pr_debug("addr:0x%x ,size:%d",addr, size);
	if(size <= MODULE_LENGTH + AWB_LENGTH + LSC_LENGTH){
		for(index = 0;index<size;index++){
			data[index] = otpData[addr];
				//pr_debug("data[%d]=0x%x,otpData[%d]=0x%x",index, data[index],addr,otpData[addr]);
                  		addr++;
		}
		return size;
	}
  	return 0;
}
EXPORT_SYMBOL(a1102wxfrontsc521cs_read_region);

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	LOG_INF("[get_imgsensor_id]");
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
			spin_lock(&imgsensor_drv_lock);
			imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
			spin_unlock(&imgsensor_drv_lock);
			do {
				*sensor_id =return_sensor_id();
				if (*sensor_id == imgsensor_info.sensor_id) {
					LOG_INF("[a1102wxfrontsc521cs_camera_sensor][get_imgsensor_id] i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
					check_sc521cs_otp();
					return ERROR_NONE;
				}
				LOG_INF("[a1102wxfrontsc521cs_camera_sensor][get_imgsensor_id] Read sensor id fail, i2c write id: 0x%x,sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				retry--;
			} while(retry > 0);
			i++;
			retry = 2;
}
	if (*sensor_id != imgsensor_info.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	 kal_uint32 sensor_id = 0;
	printk("[open]: PLATFORM:MT6789,MIPI 24LANE\n");
	printk("preview 1632*1224@30fps,360Mbps/lane; capture 3264*2448@30fps,880Mbps/lane\n");
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
				break;
			}
			printk("open:Read sensor id fail open i2c write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
	/* initail sequence write in  */
    sensor_init();

    spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}	/*	open  */
static kal_uint32 close(void)
{
	LOG_INF("close E");
	/*No Need to implement this function*/ 
	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength; 
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
//	capture_setting(300);
	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	printk("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            printk("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
    
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    normal_video_setting();
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
    imgsensor.pclk = imgsensor_info.hs_video.pclk;
    //imgsensor.video_mode = KAL_TRUE;
    imgsensor.line_length = imgsensor_info.hs_video.linelength;
    imgsensor.frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    hs_video_setting();
//	slim_video_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
    imgsensor.pclk = imgsensor_info.slim_video.pclk;
    imgsensor.line_length = imgsensor_info.slim_video.linelength;
    imgsensor.frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    slim_video_setting();
//	hs_video_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);

    return ERROR_NONE;
}    /*    slim_video     */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    LOG_INF("E\n");
    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth     = imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight     = imgsensor_info.hs_video.grabwindow_height;

    sensor_resolution->SensorSlimVideoWidth     = imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight     = imgsensor_info.slim_video.grabwindow_height;

    return ERROR_NONE;
}    /*    get_resolution    */


static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
                      MSDK_SENSOR_INFO_STRUCT *sensor_info,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("scenario_id = %d\n", scenario_id);

    sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
    sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
    sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorInterruptDelayLines = 4; /* not use */
    sensor_info->SensorResetActiveHigh = FALSE; /* not use */
    sensor_info->SensorResetDelayCount = 5; /* not use */

    sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
    sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
    sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
    sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

    sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
    sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
    sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
    sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
    sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;


    sensor_info->SensorMasterClockSwitch = 0; /* not use */
    sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

    sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
    sensor_info->SensorClockFreq = imgsensor_info.mclk;
    sensor_info->SensorClockDividCount = 3; /* not use */
    sensor_info->SensorClockRisingCount = 0;
    sensor_info->SensorClockFallingCount = 2; /* not use */
    sensor_info->SensorPixelClockCount = 3; /* not use */
    sensor_info->SensorDataLatchCount = 2; /* not use */

    sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
    sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
    sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
    sensor_info->SensorHightSampling = 0;    // 0 is default 1x
    sensor_info->SensorPacketECCOrder = 1;

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

            sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

            break;
        default:
            sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
            break;
    }

    return ERROR_NONE;
}    /*    get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:

			LOG_INF("preview\n");
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			//case MSDK_SCENARIO_ID_CAMERA_ZSD:
			capture(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            hs_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            slim_video(image_window, sensor_config_data);
            break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);

	if ((framerate == 30) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 15) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = 10 * framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);
	set_dummy();
	return ERROR_NONE;
}


static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d ", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
    kal_uint32 frame_length;

    LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            if(framerate == 0)
                return ERROR_NONE;
            frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
            imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
            set_dummy();
            LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
            break;
    }
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
    LOG_INF("scenario_id = %d\n", scenario_id);

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            *framerate = imgsensor_info.pre.max_framerate;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            *framerate = imgsensor_info.normal_video.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            *framerate = imgsensor_info.cap.max_framerate;
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            *framerate = imgsensor_info.hs_video.max_framerate;
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            *framerate = imgsensor_info.slim_video.max_framerate;
            break;
        default:
            break;
    }

    return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	printk("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
	{
		write_cmos_sensor(0x0100,0x01);
	}
	else
	{
		write_cmos_sensor(0x0100, 0x00);
	}

	mdelay(15);
	return ERROR_NONE;
}
static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
                             UINT8 *feature_para,UINT32 *feature_para_len)
{
    UINT16 *feature_return_para_16=(UINT16 *) feature_para;
    UINT16 *feature_data_16=(UINT16 *) feature_para;
    UINT32 *feature_return_para_32=(UINT32 *) feature_para;
    UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
//    unsigned long long *feature_return_para=(unsigned long long *) feature_para;

    struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    printk("feature_id = %d\n", feature_id);
    switch (feature_id) {
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
    *(feature_data + 1) = imgsensor_info.min_shutter;
        switch (*feature_data) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            *(feature_data + 2) = 2;
            break;
        default:
            *(feature_data + 2) = 1;
            break;
        }
        break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
	    *feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
			kal_uint32 rate;

			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				rate = imgsensor_info.cap.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				rate =
				    imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				rate = imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				rate =
				    imgsensor_info.slim_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				rate = imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = rate;
	}
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16)*feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
		printk("adb_i2c_read 0x%x = 0x%x\n", sensor_reg_data->RegAddr,
			sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode((BOOL)*feature_data_16, *(feature_data_16 + 1));
            break;
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
// A06 code for SR-AL7160A-01-501 by wangtao at 20240530 start
	case SENSOR_FEATURE_SET_FRAMERATE:
		printk("current fps: %d\n", (UINT32)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		printk("ihdr enable: %d\n", (BOOL)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (BOOL)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
// A06 code for SR-AL7160A-01-501 by wangtao at 20240530 end
	case SENSOR_FEATURE_GET_CROP_INFO:
		printk("SENSOR_FEATURE_GET_CROP_INFO scenarioId: %d\n", (UINT32)*feature_data);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		printk("SENSOR_SET_SENSOR_IHDR LE = %d, SE = %d, Gain = %d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data + 1),
			(UINT16)*(feature_data + 2));
		ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data + 1), (UINT16)*(feature_data + 2));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		printk("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		printk("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	default:
		break;
	}
    return ERROR_NONE;
}    /*    feature_control()  */
// A06 code for SR-AL7160A-01-539 by xuyunhui at 20240626 end
static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 A1102WXFRONTSC521CS_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	return ERROR_NONE;
}	/*	SC500CS_MIPI_RAW_SensorInit	*/
