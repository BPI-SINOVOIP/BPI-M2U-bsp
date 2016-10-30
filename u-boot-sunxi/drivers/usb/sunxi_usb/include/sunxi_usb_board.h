/*
 * drivers/usb/sunxi_usb/include/sunxi_usb_board.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb board config.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_USB_BOARD_H__
#define  __SUNXI_USB_BOARD_H__

#define  SET_USB_PARA				"usb_para"
#define  SET_USB0				"usbc0"
#define  SET_USB1				"usbc1"
#define  SET_USB2				"usbc2"

#define  KEY_USB_GLOBAL_ENABLE			"usb_global_enable"
#define  KEY_USBC_NUM				"usbc_num"

#define  KEY_USB_ENABLE				"usb_used"
#define  KEY_USB_PORT_TYPE			"usb_port_type"
#define  KEY_USB_DETECT_TYPE			"usb_detect_type"
#define  KEY_USB_ID_GPIO			"usb_id_gpio"
#define  KEY_USB_DETVBUS_GPIO			"usb_det_vbus_gpio"
#define  KEY_USB_DRVVBUS_GPIO			"usb_drv_vbus_gpio"
#define  KEY_USB_RESTRICT_GPIO			"usb_restrict_gpio"
#define  KEY_USB_REGULATOR_IO			"usb_regulator_io"
#define  KEY_USB_REGULATOR_IO_VOL		"usb_regulator_vol"

#define  KEY_USB_HOST_INIT_STATE    		"usb_host_init_state"
#define  KEY_USB_USB_RESTRICT_FLAG  		"usb_restric_flag"
#define  KEY_USB_USB_RESTRICT_VOLTAGE   	"usb_restric_voltage"
#define  KEY_USB_USB_RESTRICT_CAPACITY  	"usb_restric_capacity"
#define  KEY_USB_USB_NOT_SUSPEND		"usb_not_suspend"

/* USB config info */
enum usb_gpio_group_type{
	GPIO_GROUP_TYPE_PIO = 0,
	GPIO_GROUP_TYPE_POWER,
};

/* 0: device only; 1: host only; 2: otg */
enum usb_port_type{
	USB_PORT_TYPE_DEVICE = 0,
	USB_PORT_TYPE_HOST,
	USB_PORT_TYPE_OTG,
};

/* 0: dp/dm detect, 1: vbus/id detect */
enum usb_detect_type{
	USB_DETECT_TYPE_DP_DM = 0,
	USB_DETECT_TYPE_VBUS_ID,
};

enum usb_det_vbus_type{
	USB_DET_VBUS_TYPE_NULL = 0,
	USB_DET_VBUS_TYPE_GIPO,
	USB_DET_VBUS_TYPE_AXP,
};

typedef struct usb_port_info{
	__u32 enable;          			/* port valid			*/

	__u32 port_no;				/* usb port number		*/
	enum usb_port_type port_type;    	/* usb port type		*/
	enum usb_detect_type detect_type; 	/* usb detect type		*/
	enum usb_det_vbus_type det_vbus_type;

	__u32 usb_restrict_flag;		/* usb port number(?)		*/
	__u32 voltage;				/* usb port number(?)		*/
	__u32 capacity;				/* usb port number(?)		*/
	__u32 host_init_state;			/* usb controller initial state. 0 - not work, 1 - work */
}usb_port_info_t;

typedef struct usb_cfg{
	u32 usb_global_enable;
	u32 usbc_num;

	struct usb_port_info port[USBC_MAX_CTL_NUM];
}usb_cfg_t;

#endif   //__SUNXI_USB_BOARD_H__

