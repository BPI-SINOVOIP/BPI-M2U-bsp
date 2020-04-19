/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn85xx.c
 Abstract:
               input driver.
 Author:       Zhimin Tian
 Date :        08,14,2013
 Version:      1.0
 History :
     2012,10,30, V0.1 first version  
 --*/

#include "icn85xx.h"
#include "icn85xx_fw.h"



#define CTP_IRQ_NUMBER          (config_info.int_number)
#define CTP_IRQ_MODE		    (IRQF_TRIGGER_FALLING)
#define CTP_RST_PORT 		(config_info.wakeup_gpio.gpio)

struct ctp_config_info config_info = {
	    .input_type = CTP_TYPE,
	    .name = NULL,
};

static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static __u32 twi_id = 0;

static char irq_pin_name[8];
static const unsigned short normal_i2c[2] = {0x48,I2C_CLIENT_END};

#if COMPILE_FW_WITH_DRIVER
       static char firmware[128] = "icn85xx_firmware";
#else
    #if SUPPORT_SENSOR_ID 
        static char firmware[128] = {0};
    #else
       static char firmware[128] = {"/misc/modules/ICN87xx.bin"};
    #endif
#endif

#if SUPPORT_SENSOR_ID
   char cursensor_id,tarsensor_id,id_match;
   char invalid_id = 0;

   struct sensor_id {
                char value;
                const char bin_name[128];
                unsigned char *fw_name;
                int size;
        };

static struct sensor_id sensor_id_table[] = {
                   { 0x22, "/misc/modules/ICN8505_22_name9.BIN",fw_22_name9,sizeof(fw_22_name9)},//default bin or fw
                   { 0x00, "/misc/modules/ICN8505_00_name1.BIN",fw_00_name1,sizeof(fw_00_name1)},
                   { 0x20, "/misc/modules/ICN8505_20_name7.BIN",fw_20_name7,sizeof(fw_20_name7)},
                   { 0x10, "/misc/modules/ICN8505_10_name4.BIN",fw_10_name4,sizeof(fw_10_name4)},
                   { 0x11, "/misc/modules/ICN8505_11_name5.BIN",fw_11_name5,sizeof(fw_11_name5)},
                   { 0x12, "/misc/modules/ICN8505_12_name6.BIN",fw_12_name6,sizeof(fw_12_name6)},
                   { 0x01, "/misc/modules/ICN8505_01_name2.BIN",fw_01_name2,sizeof(fw_01_name2)},
                   { 0x21, "/misc/modules/ICN8505_21_name8.BIN",fw_21_name8,sizeof(fw_21_name8)},
                   { 0x02, "/misc/modules/ICN8505_02_name3.BIN",fw_02_name3,sizeof(fw_02_name3)},
                 // if you want support other sensor id value ,please add here
                 };
#endif

static struct regulator *tp_regulator = NULL;
static struct i2c_client *this_client;
short log_basedata[COL_NUM][ROW_NUM] = {{0,0}};
short log_rawdata[COL_NUM][ROW_NUM] = {{0,0}};
short log_diffdata[COL_NUM][ROW_NUM] = {{0,0}};
unsigned int log_on_off = 0;
static void icn85xx_log(char diff);
static int icn85xx_testTP(char type, int para1, int para2, int para3);
extern void setbootmode(char bmode);

static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer);

#if SUPPORT_PROC_FS
pack_head cmd_head;
static struct proc_dir_entry *icn85xx_proc_entry;
int  DATA_LENGTH = 0;
GESTURE_DATA structGestureData;
STRUCT_PANEL_PARA_H g_structPanelPara;
#endif



static int ctp_get_system_config(void)
{
    //    ctp_print_info(config_info,DEBUG_INIT);
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}

/**
 * ctp_wakeup - function
 *
 */

int ctp_wakeup(int status,int ms)
{
    //    dprintk(DEBUG_INIT,"***CTP*** %s:status:%d,ms = %d\n",__func__,status,ms);

	if (status == 0) {
		if (ms == 0) {
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 0);
		} else {
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 1);
		}
	}
	if (status == 1) {
		if (ms == 0) {
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 1);
		} else {
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			CTP_GPIO_SET_VALUE(config_info.wakeup_gpio.gpio, 0);
		}
	}
	msleep(5);

	return 0;
}

#if SUPPORT_SYSFS

//static ssize_t icn85xx_show_update(struct device* cd,struct device_attribute *attr, char* buf);
//static ssize_t icn85xx_store_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
int icn85xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length);
static ssize_t icn85xx_store_update(struct device_driver *drv,const char *buf,size_t count);
static ssize_t icn85xx_show_update(struct device_driver *drv,char* buf);
static ssize_t icn85xx_show_process(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t icn85xx_store_process(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
//static DEVICE_ATTR(update, S_IRUGO | S_IWUSR, icn85xx_show_update, icn85xx_store_update);
//static DEVICE_ATTR(process, S_IRUGO | S_IWUSR, icn85xx_show_process, icn85xx_store_process);

static ssize_t icn85xx_show_process(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t ret = 0;
	sprintf(buf, "icn85xx process\n");
	ret = strlen(buf) + 1;
	return ret;
}

static ssize_t icn85xx_store_process(struct device* cd, struct device_attribute *attr,
               const char* buf, size_t len)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    unsigned long on_off = simple_strtoul(buf, NULL, 10); 
    log_on_off = on_off;
    memset(&log_basedata[0][0], 0, COL_NUM*ROW_NUM*2);
    if(on_off == 0)
    {
        icn85xx_ts->work_mode = 0;
    }
    else if((on_off == 1) || (on_off == 2) || (on_off == 3))
    {
        if((icn85xx_ts->work_mode == 0) && (icn85xx_ts->use_irq == 1))
        {
            hrtimer_init(&icn85xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            icn85xx_ts->timer.function = chipone_timer_func;
            hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }
        icn85xx_ts->work_mode = on_off;
    }
    else if(on_off == 10)
    {
        icn85xx_ts->work_mode = 4;
        mdelay(10);
        printk("update baseline\n");
        icn85xx_write_reg(4, 0x30); 
        icn85xx_ts->work_mode = 0;
    }
    else
    {
        icn85xx_ts->work_mode = 0;
    }
    
    
    return len;
}

#define PARA_STATE_CMD             0X00
#define PARA_STATE_DATA1           0X01
#define PARA_STATE_DATA2           0X02
#define PARA_STATE_DATA3           0X03

#define UPDATE_CMD_NONE            0x00

static char update_cmd = UPDATE_CMD_NONE;
static int  update_data1 = 0;
static int  update_data2 = 0;
static int  update_data3 = 0;

static ssize_t icn85xx_show_update(struct device_driver *drv,
                                        char* buf)
{
    ssize_t ret = 0;       
   
    switch(update_cmd)
    {
        case 'u':
        case 'U':
            sprintf(buf, firmware);
            ret = strlen(buf) + 1;
            printk("firmware: %s\n", firmware);  
   
            break;
        case 't':
        case 'T':
            icn85xx_trace("cmd t,T\n");
            
            break;
        case 'r':
        case 'R':
            icn85xx_trace("cmd r,R\n");
             
            break;
        case 'd':
        case 'D':
            icn85xx_trace("cmd d,D\n");
            
            break;
        case 'c':
        case 'C':
            icn85xx_trace("cmd c,C, %d, %d, %d\n", update_data1, update_data2, update_data3);
            sprintf(buf, "%02x:%08x:%08x\n",update_data1,update_data2,update_data3);
            ret = strlen(buf) + 1;
            break;
        case 'e':
        case 'E':
            icn85xx_trace("cmd e,E, %d, %d, %d\n", update_data1, update_data2, update_data3);
            sprintf(buf, "%02x:%08x:%08x\n",update_data1,update_data2,update_data3);
            ret = strlen(buf) + 1;
            break;
        default:
            printk("this conmand is unknow!!\n");
            break;                

    }

    return ret;
}
/*  update cmd:
*u:/mnt/aaa/bbb/ccc   or U:/mnt/aaa/bbb/ccc, update firmware
*t or T, reset tp
*r or R, printf rawdata
*d or D, printf diffdata
*c:100 or C:100, Consistence test
*e:Vol:Max:Min or E:Vol:Max:Min, diffdata test
*        you can get the diffdata test result, and you should send e/E cmd before this cmd 
*        result formate:   result:Max:Min=%02x:%08x:%08x
*        when cmd e/E before this cmd and Max=Min=0, you can get Max/Min diffdata
*/

static ssize_t icn85xx_store_update(struct device_driver *drv,const char *buf,size_t count)
{
   printk("count: %d, update: %s\n", count, buf);
    int err=0;
    int i=0,j=0;
    char para_state = PARA_STATE_CMD;
    char cmd[16] = {0};
    char data1[128] = {0};
    char data2[16] = {0};
    char data3[16] = {0};
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    char * p;
    p = cmd;
    for(i=0; i<count; i++)
    {
        if(buf[i] == ':')
        {
            if(PARA_STATE_CMD == para_state)
            {
                p[j] = '\0';
                para_state = PARA_STATE_DATA1;
                p = data1;
                j = 0;
            }
            else if(PARA_STATE_DATA1 == para_state)
            {
                p[j] = '\0';
                para_state = PARA_STATE_DATA2;
                p = data2;
                j = 0;
            }
            else if(PARA_STATE_DATA2 == para_state)
            {
                p[j] = '\0';
                para_state = PARA_STATE_DATA3;
                p = data3;
                j = 0;
            }

        }
        else
        {
            p[j++] =  buf[i];
        }
    }
    p[j] = '\0';
    
    update_cmd = cmd[0];
    switch(update_cmd)
    {
        case 'u':
        case 'U':
            icn85xx_trace("firmware: %s, %d\n", firmware, strlen(firmware));
            for(i=0; i<40; i++)
                printk("0x%2x ", firmware[i]);
            printk("\n");
            memset(firmware, 0, 128);
            memcpy(firmware, data1, strlen(data1)-1);
            icn85xx_trace("firmware: %s, %d\n", firmware, strlen(firmware));            
            icn85xx_trace("fwVersion : 0x%x\n", icn85xx_read_fw_Ver(firmware)); 
            icn85xx_trace("current version: 0x%x\n", icn85xx_readVersion());
            if((icn85xx_ts->ictype == ICN85XX_WITH_FLASH_87) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87))
            {
                if(R_OK == icn87xx_fw_update(firmware))
                {   
                   printk("87 update ok\n");
                }
                else
                {
                   printk("87 update error\n");   
                }

            }
            else
            {
                if(R_OK == icn85xx_fw_update(firmware))
                {   
                    printk("update ok\n");
                }
                else
                {
                    printk("update error\n");   
                }
            }
            break;
        case 't':
        case 'T':
            icn85xx_trace("cmd t,T\n");
            icn85xx_ts_reset();
            break;
        case 'r':
        case 'R':
            icn85xx_trace("cmd r,R\n");
            icn85xx_log(0); 
            break;
        case 'd':
        case 'D':
            icn85xx_trace("cmd d,D\n");
            icn85xx_log(1);
            break;
        case 'c':
        case 'C':            
            update_data1 = simple_strtoul(data1, NULL, 10);
            icn85xx_trace("cmd c,C, %d\n", update_data1);
            update_data1 = icn85xx_testTP(0, update_data1, 0, 0);
            update_data2 = 0;
            update_data3 = 0;
            icn85xx_trace("cmd c,C, result: %d\n", update_data1);
            break;
        case 'e':
        case 'E':
            update_data1 = simple_strtoul(data1, NULL, 10);
            update_data2 = simple_strtoul(data2, NULL, 10);
            update_data3 = simple_strtoul(data3, NULL, 10);
            icn85xx_trace("cmd e,E, %d, %d, %d, %d\n", update_data1, update_data2, update_data3);
            update_data1 = icn85xx_testTP(1, update_data1, update_data2, update_data3);
            break;
        default:
            printk("this conmand is unknow!!\n");
            break;                

    }
    
    return count;
}

static int icn85xx_create_sysfs(struct i2c_client *client)
{
    int err;
    struct device *dev = &(client->dev);
    icn85xx_trace("%s: \n",__func__);    
  //  err = device_create_file(dev, &dev_attr_update);
   // err = device_create_file(dev, &dev_attr_process);
    return err;
}

static void icn85xx_remove_sysfs(struct i2c_client *client)
{
    struct device *dev = &(client->dev);
    icn85xx_trace("%s: \n",__func__);    
    //device_remove_file(dev, &dev_attr_update);
    //device_remove_file(dev, &dev_attr_process);
}


//static DEVICE_ATTR(icn_update, S_IRUGO | S_IWUSR, icn85xx_show_update, icn85xx_store_update);
//static DEVICE_ATTR(process, S_IRUGO | S_IWUSR, icn85xx_show_process, icn85xx_store_process);
static DRIVER_ATTR(icn_update, S_IRUGO | S_IWUSR, icn85xx_show_update, icn85xx_store_update);
static DRIVER_ATTR(icn_process, S_IRUGO | S_IWUSR, icn85xx_show_process, icn85xx_store_process);

static struct attribute *icn_drv_attr[] = {
    &driver_attr_icn_update.attr,
    &driver_attr_icn_process.attr,  
    NULL
};
static struct attribute_group icn_drv_attr_grp = {
    .attrs =icn_drv_attr,
};
static const struct attribute_group *icn_drv_grp[] = {
    &icn_drv_attr_grp,
    NULL
};
#endif


#if SUPPORT_PROC_FS

static ssize_t icn85xx_tool_write(struct file *file, const char __user * buffer, size_t count, loff_t * ppos)
{
     int ret = 0;
    int i;
    unsigned short addr;
    unsigned int prog_addr;
    char retvalue;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    proc_info("%s \n",__func__);  
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return -1;   
    }     
    ret = copy_from_user(&cmd_head, buffer, CMD_HEAD_LENGTH);
    if(ret)
    {
        proc_error("copy_from_user failed.\n");
        goto write_out;
    }  
    else
    {
        ret = CMD_HEAD_LENGTH;
    }
    
    proc_info("wr  :0x%02x.\n", cmd_head.wr);
    proc_info("flag:0x%02x.\n", cmd_head.flag);
    proc_info("circle  :%d.\n", (int)cmd_head.circle);
    proc_info("times   :%d.\n", (int)cmd_head.times);
    proc_info("retry   :%d.\n", (int)cmd_head.retry);
    proc_info("data len:%d.\n", (int)cmd_head.data_len);
    proc_info("addr len:%d.\n", (int)cmd_head.addr_len);
    proc_info("addr:0x%02x%02x.\n", cmd_head.addr[0], cmd_head.addr[1]);
    proc_info("len:%d.\n", (int)count);
    proc_info("data:0x%02x%02x.\n", buffer[CMD_HEAD_LENGTH], buffer[CMD_HEAD_LENGTH+1]);
    if (1 == cmd_head.wr)  // write para
    {
        proc_info("cmd_head_.wr == 1  \n");
        ret = copy_from_user(&cmd_head.data[0], &buffer[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        //need copy to g_structPanelPara

        memcpy(&g_structPanelPara, &cmd_head.data[0], cmd_head.data_len);
        //write para to tp
        for(i=0; i<cmd_head.data_len; )
        {
            int size = ((i+64) > cmd_head.data_len)?(cmd_head.data_len-i):64;
            ret = icn85xx_i2c_txdata(0x8000+i, &cmd_head.data[i], size);
            if (ret < 0) {
                proc_error("write para failed!\n");
                goto write_out;
            }   
            i = i + 64;     
        }
        ret = cmd_head.data_len + CMD_HEAD_LENGTH;
        icn85xx_ts->work_mode = 4; //reinit
        printk("reinit tp\n");
        icn85xx_write_reg(0, 1); 
        mdelay(100);
        icn85xx_write_reg(0, 0);
        mdelay(100);
        icn85xx_ts->work_mode = 0;
        goto write_out;

    }
    else if(3 == cmd_head.wr)   //set update file
    {
        proc_info("cmd_head_.wr == 3  \n");
        ret = copy_from_user(&cmd_head.data[0], &buffer[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        ret = cmd_head.data_len + CMD_HEAD_LENGTH;
        memset(firmware, 0, 128);
        memcpy(firmware, &cmd_head.data[0], cmd_head.data_len);
        proc_info("firmware : %s\n", firmware);
    }
    else if(5 == cmd_head.wr)  //start update 
    {        
        proc_info("cmd_head_.wr == 5 \n");
        icn85xx_update_status(1); 

        if((icn85xx_ts->ictype == ICN85XX_WITH_FLASH_87) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87)) 
        {
            ret = kernel_thread(icn87xx_fw_update,firmware,CLONE_KERNEL);
        }
        else
        {
            ret = kernel_thread(icn85xx_fw_update,firmware,CLONE_KERNEL);
        }
        
        icn85xx_trace("the kernel_thread result is:%d\n", ret);    
    }
    else if(11 == cmd_head.wr) //write hostcomm
    { 
        icn85xx_ts->work_mode = cmd_head.flag; //for gesture test,you should set flag=6
        structGestureData.u8Status = 0;

        ret = copy_from_user(&cmd_head.data[0], &buffer[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }               
        addr = (cmd_head.addr[1]<<8) | cmd_head.addr[0];
        icn85xx_write_reg(addr, cmd_head.data[0]);            
        
    }
    else if(13 == cmd_head.wr) //adc enable
    { 
        proc_info("cmd_head_.wr == 13  \n");
        icn85xx_ts->work_mode = 4;
        mdelay(10);
        //set col
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8ColNum), 1); 
        //u8RXOrder[0] = u8RXOrder[cmd_head.addr[0]];
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8RXOrder[0]), g_structPanelPara.u8RXOrder[cmd_head.addr[0]]); 
        //set row
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8RowNum), 1);
        //u8TXOrder[0] = u8TXOrder[cmd_head.addr[1]];        
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8TXOrder[0]), g_structPanelPara.u8TXOrder[cmd_head.addr[1]]); 
        //scan mode
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8ScanMode), 0);
        //bit
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u16BitFreq), 0xD0);
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u16BitFreq)+1, 0x07);
        //freq
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u16FreqCycleNum[0]), 0x64);
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u16FreqCycleNum[0])+1, 0x00);
        //pga
        icn85xx_write_reg(0x8000+STRUCT_OFFSET(STRUCT_PANEL_PARA_H, u8PgaGain), 0x0);

        //config mode
        icn85xx_write_reg(0, 0x2);
        
        mdelay(1);
        icn85xx_read_reg(2, &retvalue);
        printk("retvalue0: %d\n", retvalue);
        while(retvalue != 1)
        {   
            printk("retvalue: %d\n", retvalue);
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
        }   

        if(icn85xx_goto_progmode() != 0)
        {
            printk("icn85xx_goto_progmode() != 0 error\n");
            goto write_out;
        }
        
        icn85xx_prog_write_reg(0x040870, 1);   

    }
    else if(15 == cmd_head.wr) // write hostcomm multibyte
    {
        proc_info("cmd_head_.wr == 15  \n");        
        ret = copy_from_user(&cmd_head.data[0], &buffer[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        addr = (cmd_head.addr[1]<<8) | cmd_head.addr[0];
        printk("wr, addr: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", addr, cmd_head.data[0], cmd_head.data[1], cmd_head.data[2], cmd_head.data[3]);
        ret = icn85xx_i2c_txdata(addr, &cmd_head.data[0], cmd_head.data_len);
        if (ret < 0) {
            proc_error("write hostcomm multibyte failed!\n");
            goto write_out;
        }
    }
    else if(17 == cmd_head.wr)// write iic porgmode multibyte
    {
        proc_info("cmd_head_.wr == 17  \n");
        ret = copy_from_user(&cmd_head.data[0], &buffer[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        prog_addr = (cmd_head.flag<<16) | (cmd_head.addr[1]<<8) | cmd_head.addr[0];
        icn85xx_goto_progmode();
        ret = icn85xx_prog_i2c_txdata(prog_addr, &cmd_head.data[0], cmd_head.data_len);
        if (ret < 0) {
            proc_error("write hostcomm multibyte failed!\n");
            goto write_out;
        }

    }

write_out:
    up(&icn85xx_ts->sem); 
    proc_info("icn85xx_tool_write write_out  \n");
    return count;
    
}

static ssize_t icn85xx_tool_read(struct file *file, char __user * buffer, size_t count, loff_t * ppos)
{
    int i, j;
    int ret = 0;
    char row, column, retvalue, max_column;
    unsigned short addr;
    unsigned int prog_addr;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return -1;   
    }     
    proc_info("%s: count:%d, off:%d, cmd_head.data_len: %d\n",__func__, count,(int)(* ppos),(int)cmd_head.data_len); 
    if (cmd_head.wr % 2)
    {
        ret = 0;
        proc_info("cmd_head_.wr == 1111111  \n");
        goto read_out;
    }
    else if (0 == cmd_head.wr)   //read para
    {
        //read para
        proc_info("cmd_head_.wr == 0  \n");
        ret = icn85xx_i2c_rxdata(0x8000, &g_structPanelPara, cmd_head.data_len);
        if (ret < 0) {
            icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        }
        ret = copy_to_user(buffer, (void*)(&g_structPanelPara), cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }

        goto read_out;

    }
    else if(2 == cmd_head.wr)  //get update status
    {
        proc_info("cmd_head_.wr == 2  \n");
        retvalue = icn85xx_get_status();
        proc_info("status: %d\n", retvalue); 
        ret =copy_to_user(buffer, (void*)(&retvalue), 1);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }
    }
    else if(4 == cmd_head.wr)  //read rawdata
    {
        //icn85xx_read_reg(0x8004, &row);
        //icn85xx_read_reg(0x8005, &column);   
        proc_info("cmd_head_.wr == 4  \n");     
        row = cmd_head.addr[1];
        column = cmd_head.addr[0];
        max_column = (cmd_head.flag==0)?(24):(cmd_head.flag);
        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);
        for(i=0; i<1000; i++)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
            if(retvalue == 1)
                break;
        }
        
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x2000 + i*(max_column)*2,(char *) &log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            ret = copy_to_user(&buffer[column*2*i], (void*)(&log_rawdata[i][0]), column*2);
            if(ret)
            {
                proc_error("copy_to_user failed.\n");
                goto read_out;
            }    
        } 
    
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);    
        icn85xx_write_reg(4, 0x21); 
        goto read_out;        
    }
    else if(6 == cmd_head.wr)  //read diffdata
    {   
        proc_info("cmd_head_.wr == 6   \n");
        row = cmd_head.addr[1];
        column = cmd_head.addr[0];
        max_column = (cmd_head.flag==0)?(24):(cmd_head.flag);        
        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);

        for(i=0; i<1000; i++)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
            if(retvalue == 1)
                break;
        }      

        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x3000 + (i+1)*(max_column+2)*2 + 2,(char *) &log_diffdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            ret = copy_to_user(&buffer[column*2*i], (void*)(&log_diffdata[i][0]), column*2);
            if(ret)
            {
                proc_error("copy_to_user failed.\n");
                goto read_out;
            }

        }      
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);         
        icn85xx_write_reg(4, 0x21); 
        
        goto read_out;          
    }
    else if(8 == cmd_head.wr)  //change TxVol, read diff
    {       
        proc_info("cmd_head_.wr == 8  \n");
        row = cmd_head.addr[1];
        column = cmd_head.addr[0];
        max_column = (cmd_head.flag==0)?(24):(cmd_head.flag); 
        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);
        for(i=0; i<1000; i++)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
            if(retvalue == 1)
                break;
        }     
        
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x2000 + i*(max_column)*2,(char *) &log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
           
        } 
    
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);    
        icn85xx_write_reg(4, 0x21); 

        icn85xx_write_reg(4, 0x12);

        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);
        for(i=0; i<1000; i++)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
            if(retvalue == 1)
                break;
        }    
        
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x2000 + i*(max_column)*2,(char *) &log_diffdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 

            for(j=0; j<column; j++)
            {
                log_basedata[i][j] = log_rawdata[i][j] - log_diffdata[i][j];
            }
            ret = copy_to_user(&buffer[column*2*i], (void*)(&log_basedata[i][0]), column*2);
            if(ret)
            {
                proc_error("copy_to_user failed.\n");
                goto read_out;
            }

        } 
    
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);    
        icn85xx_write_reg(4, 0x21); 

        icn85xx_write_reg(4, 0x10);

        goto read_out;          
    }
    else if(10 == cmd_head.wr)  //read adc data
    {
        proc_info("cmd_head_.wr == 10  \n");
        if(cmd_head.flag == 0)
        {
            icn85xx_prog_write_reg(0x040874, 0);  
        }        
        icn85xx_prog_read_page(2500*cmd_head.flag,(char *) &log_diffdata[0][0],cmd_head.data_len);
        ret = copy_to_user(buffer, (void*)(&log_diffdata[0][0]), cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }

        if(cmd_head.flag == 9)
        {
            //reload code
            if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_86))
            {
                if(R_OK == icn85xx_fw_update(firmware))
                {
                    icn85xx_ts->code_loaded_flag = 1;
                    icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code ok\n"); 
                }
                else
                {
                    icn85xx_ts->code_loaded_flag = 0;
                    icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code error\n"); 
                }
            }
            else
            {
                icn85xx_bootfrom_flash(icn85xx_ts->ictype);                
                msleep(150);
/*
                memset(&cmd_head.data[0], 0, 3);
                ret = icn85xx_i2c_rxdata(0xa,&cmd_head.data[0],1);
                if(ret < 0)
                {
                    proc_error("normal read id failed.\n");                    
                }
                proc_info("nomal mode id: 0x%x\n", cmd_head.data[0]);

                icn85xx_prog_i2c_rxdata(0x40000,&cmd_head.data[0],3);
                if(ret < 0)
                {
                    proc_error("prog mode read id failed.\n");                    
                }
                proc_info("prog mode id: 0x%x 0x%x\n", cmd_head.data[1], cmd_head.data[2]);
*/                
            }
            icn85xx_ts->work_mode = 0;
        }
    }
    else if(12 == cmd_head.wr) //read hostcomm
    {
        proc_info("cmd_head_.wr == 12  \n");
        addr = (cmd_head.addr[1]<<8) | cmd_head.addr[0];
        icn85xx_read_reg(addr, &retvalue);
        ret = copy_to_user(buffer, (void*)(&retvalue), 1);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }
    }
    else if(14 == cmd_head.wr) //read adc status
    {
        proc_info("cmd_head_.wr == 14  \n");
        icn85xx_prog_read_reg(0x4085E, &retvalue);
        ret = copy_to_user(buffer, (void*)(&retvalue), 1);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }
        printk("0x4085E: 0x%x\n", retvalue);
    }  
    else if(16 == cmd_head.wr)  //read gesture data
    {
        proc_info("cmd_head_.wr == 16  \n");
        ret = copy_to_user(buffer, (void*)(&structGestureData), sizeof(structGestureData));
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }

        if(structGestureData.u8Status == 1)
            structGestureData.u8Status = 0;
    }
    else if(18 == cmd_head.wr) // read hostcomm multibyte
    {
        proc_info("cmd_head_.wr == 18  \n");       
        addr = (cmd_head.addr[1]<<8) | cmd_head.addr[0];        
        ret = icn85xx_i2c_rxdata(addr, &cmd_head.data[0], cmd_head.data_len);
        if(ret < 0)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }
        ret = copy_to_user(buffer, &cmd_head.data[0], cmd_head.data_len);
        if (ret) {
            icn85xx_error("read hostcomm multibyte failed: %d\n", ret);
        }   
        printk("rd, addr: 0x%x, data_len: %d, data: 0x%x\n", addr, cmd_head.data_len, cmd_head.data[0]);
        goto read_out;

    }
    else if(20 == cmd_head.wr)// read iic porgmode multibyte
    {
        proc_info("cmd_head_.wr == 20  \n");        
        prog_addr = (cmd_head.flag<<16) | (cmd_head.addr[1]<<8) | cmd_head.addr[0];
        icn85xx_goto_progmode();

        ret = icn85xx_prog_i2c_rxdata(prog_addr, &cmd_head.data[0], cmd_head.data_len);
        if (ret < 0) {
            icn85xx_error("read iic porgmode multibyte failed: %d\n", ret);
        }   
        ret = copy_to_user(buffer, &cmd_head.data[0], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_to_user failed.\n");
            goto read_out;
        }

        
        icn85xx_bootfrom_sram();
        goto read_out;

    }
		else if(22 == cmd_head.wr) //read ictype
		{
			proc_info("cmd_head_.wr == 22  \n");
			ret = copy_to_user(buffer, (void*)(&icn85xx_ts->ictype), 1);
			if(ret)
			{
				proc_error("copy_to_user failed.\n");
				goto read_out;
			}
		}
read_out:
    up(&icn85xx_ts->sem);   
    proc_info("%s out: %d, cmd_head.data_len: %d\n\n",__func__, count, cmd_head.data_len); 
    return cmd_head.data_len;
}

static const struct file_operations icn85xx_proc_fops = {
    .owner      = THIS_MODULE,
    .read       = icn85xx_tool_read,
    .write      = icn85xx_tool_write,
};

void init_proc_node(void)
{
    int i;
    memset(&cmd_head, 0, sizeof(cmd_head));
    cmd_head.data = NULL;

    i = 5;
    while ((!cmd_head.data) && i)
    {
        cmd_head.data = kzalloc(i * DATA_LENGTH_UINT, GFP_KERNEL);
        if (NULL != cmd_head.data)
        {
            break;
        }
        i--;
    }
    if (i)
    {
        //DATA_LENGTH = i * DATA_LENGTH_UINT + GTP_ADDR_LENGTH;
        DATA_LENGTH = i * DATA_LENGTH_UINT;
        icn85xx_trace("alloc memory size:%d.\n", DATA_LENGTH);
    }
    else
    {
        proc_error("alloc for memory failed.\n");
        return ;
    }

    icn85xx_proc_entry = proc_create(ICN85XX_ENTRY_NAME, 0666, NULL, &icn85xx_proc_fops);   
    if (icn85xx_proc_entry == NULL)
    {
        proc_error("Couldn't create proc entry!\n");
        return ;
    }
    else
    {
        icn85xx_trace("Create proc entry success!\n");
    }

    return ;
}

void uninit_proc_node(void)
{
    kfree(cmd_head.data);
    cmd_head.data = NULL;
    remove_proc_entry(ICN85XX_ENTRY_NAME, NULL);
}
    
#endif

#if TOUCH_VIRTUAL_KEYS
static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,
     __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":100:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":280:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":470:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":900:1030:50:60"
     "\n");
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.chipone-ts",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static void icn85xx_ts_virtual_keys_init(void)
{
    int ret;
    struct kobject *properties_kobj;      
    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");    
}
#endif

/* ---------------------------------------------------------------------
*
*   Chipone panel related driver
*
*
----------------------------------------------------------------------*/
/***********************************************************************************************
Name    :   icn85xx_ts_wakeup 
Input   :   void
Output  :   ret
function    : this function is used to wakeup tp
***********************************************************************************************/
void icn85xx_ts_wakeup(void)
{

}

void icn85xx_set_io_int(void)
{
        long unsigned int	config;
		
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,0xFFFF);
	    pin_config_get(SUNXI_PINCTRL,irq_pin_name,&config);
		if (4 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		      config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,4);
			  pin_config_set(SUNXI_PINCTRL,irq_pin_name,config);;
	    }
}

/***********************************************************************************************
Name    :   icn85xx_ts_reset 
Input   :   void
Output  :   ret
function    : this function is used to reset tp, you should not delete it
***********************************************************************************************/
void icn85xx_ts_reset(void)
{
  		ctp_wakeup(0, CTP_WAKEUP_LOW_PERIOD);
  			msleep(20);
  		icn85xx_set_io_int();
}


void icn85xx_set_prog_addr(void)
{  
    icn85xx_ts_reset();
}



/***********************************************************************************************
Name    :   icn85xx_irq_disable 
Input   :   void
Output  :   ret
function    : this function is used to disable irq
***********************************************************************************************/
/*void icn85xx_irq_disable(void)
{
    unsigned long irqflags;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    spin_lock_irqsave(&icn85xx_ts->irq_lock, irqflags);
    if (!icn85xx_ts->irq_is_disable)
    {
        icn85xx_ts->irq_is_disable = 1; 
        disable_irq_nosync(icn85xx_ts->irq);
        //disable_irq(icn85xx_ts->irq);
    }
    spin_unlock_irqrestore(&icn85xx_ts->irq_lock, irqflags);

}*/

/***********************************************************************************************
Name    :   icn85xx_irq_enable 
Input   :   void
Output  :   ret
function    : this function is used to enable irq
***********************************************************************************************/
/*void icn85xx_irq_enable(void)
{
    unsigned long irqflags = 0;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    spin_lock_irqsave(&icn85xx_ts->irq_lock, irqflags);
    if (icn85xx_ts->irq_is_disable) 
    {
        enable_irq(icn85xx_ts->irq);
        icn85xx_ts->irq_is_disable = 0; 
    }
    spin_unlock_irqrestore(&icn85xx_ts->irq_lock, irqflags);

}*/

/***********************************************************************************************
Name    :   icn85xx_prog_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn85xx, prog mode 
***********************************************************************************************/
int icn85xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
#if 0
    struct i2c_msg msgs[] = {   
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,            
        },
    };
        
    icn85xx_prog_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {    
        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }
    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }    
#else
    unsigned char tmp_buf[3];
    struct i2c_msg msgs[] = {
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = 3,
            .buf    = tmp_buf,           
        },
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,            
        },
    };

    tmp_buf[0] = (unsigned char)(addr>>16);
    tmp_buf[1] = (unsigned char)(addr>>8);
    tmp_buf[2] = (unsigned char)(addr);
    

    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
#endif      
    return ret;
}
int icn87xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
#if 0 
    struct i2c_msg msgs[] = {   
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,            
        },
    };
        
    icn85xx_prog_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {    
        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }
    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }    
#else
    unsigned char tmp_buf[2];
    struct i2c_msg msgs[] = {
        {
            .addr   = ICN87XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = tmp_buf,           
        },
        {
            .addr   = ICN87XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,            
        },
    };

    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);
    

    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
#endif      
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_prog_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn85xx , prog mode
***********************************************************************************************/
int icn85xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length)
{
    int ret = -1;
    char tmp_buf[128];
    int retries = 0; 
    struct i2c_msg msg[] = {
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = length + 3,
            .buf    = tmp_buf,           
        },
    };
    
    if (length > 125)
    {
        icn85xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = (unsigned char)(addr>>16);
    tmp_buf[1] = (unsigned char)(addr>>8);
    tmp_buf[2] = (unsigned char)(addr);


    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[3], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c write error: %d\n", __func__, ret); 
        //icn85xx_ts_reset();
    }
    return ret;
}


int icn87xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length)
{
    int ret = -1;
    char tmp_buf[128];
    int retries = 0; 
    struct i2c_msg msg[] = {
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = length + 2,
            .buf    = tmp_buf,           
        },
    };
    
    if (length > 125)
    {
        icn85xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);


    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[2], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c write error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_prog_write_reg
Input   :   addr -- address
            para -- parameter
Output  :   
function    :   write register of icn85xx, prog mode
***********************************************************************************************/
int icn85xx_prog_write_reg(unsigned int addr, char para)
{
    char buf[3];
    int ret = -1;

    buf[0] = para;
    ret = icn85xx_prog_i2c_txdata(addr, buf, 1);
    if (ret < 0) {
        icn85xx_error("%s write reg failed! %#x ret: %d\n", __func__, buf[0], ret);
        return -1;
    }
    
    return ret;
}


/***********************************************************************************************
Name    :   icn85xx_prog_read_reg 
Input   :   addr
            pdata
Output  :   
function    :   read register of icn85xx, prog mode
****************O 0x:85(ICN85xx)***************************************************************/
int icn85xx_prog_read_reg(unsigned int addr, char *pdata)
{
    int ret = -1;
    ret = icn85xx_prog_i2c_rxdata(addr, pdata, 1);  
    return ret;    
}
/***********************************************************************************************
Name    :   icn85xx_prog_read_page
Input   :   Addr
            Buffer
Output  :   
function    :   read large register of icn85xx, in prog mode
***********************************************************************************************/
int icn85xx_prog_read_page(unsigned int Addr,unsigned char *Buffer, unsigned int Length)
{
    int ret =0;
    unsigned int StartAddr = Addr;
    while(Length){
        if(Length > MAX_LENGTH_PER_TRANSFER){
            ret = icn85xx_prog_i2c_rxdata(StartAddr, Buffer, MAX_LENGTH_PER_TRANSFER);
            Length -= MAX_LENGTH_PER_TRANSFER;
            Buffer += MAX_LENGTH_PER_TRANSFER;
            StartAddr += MAX_LENGTH_PER_TRANSFER; 
        }
        else{
            ret = icn85xx_prog_i2c_rxdata(StartAddr, Buffer, Length);
            Length = 0;
            Buffer += Length;
            StartAddr += Length;
            break;
        }
        icn85xx_error("\n icn85xx_prog_read_page StartAddr:0x%x, length: %d\n",StartAddr,Length);
    }
    if (ret < 0) {
        icn85xx_error("\n icn85xx_prog_read_page failed! StartAddr:  0x%x, ret: %d\n", StartAddr, ret);
        return ret;
    }
    else{ 
          printk("\n icn85xx_prog_read_page, StartAddr 0x%x, Length: %d\n", StartAddr, Length);
          return ret;
      }  
}
/***********************************************************************************************
Name    :   icn85xx_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn85xx, normal mode   
***********************************************************************************************/
int icn85xx_i2c_rxdata(unsigned short addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
    unsigned char tmp_buf[2];
#if 0
    struct i2c_msg msgs[] = {
        {
            .addr   = this_client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = tmp_buf,            
        },
        {
            .addr   = this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
        },
    };
 
 
 
    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);
   

    while(retries < IIC_RETRY_NUM)
    {
       ret = i2c_transfer(this_client->adapter, msgs, 2);
   
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }    
#else 

    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);
   
    while(retries < IIC_RETRY_NUM)
    {
      //  ret = i2c_transfer(this_client->adapter, msgs, 2);
        ret = i2c_master_send(this_client, tmp_buf, 2);
        if (ret < 0)
            return ret;
        ret = i2c_master_recv(this_client, rxdata, length);
        if (ret < 0)
            return ret;
        if(ret == length)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
#endif
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn85xx , normal mode
***********************************************************************************************/
int icn85xx_i2c_txdata(unsigned short addr, char *txdata, int length)
{
    int ret = -1;
    unsigned char tmp_buf[128];
    int retries = 0;

    struct i2c_msg msg[] = {
        {
            .addr   = this_client->addr,
            .flags  = 0,
            .len    = length + 2,
            .buf    = tmp_buf,          
        },
    };
    
    if (length > 125)
    {
        icn85xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);

    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[2], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c write error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }

    return ret;
}

/***********************************************************************************************
Name    :   icn85xx_write_reg
Input   :   addr -- address
            para -- parameter
Output  :   
function    :   write register of icn85xx, normal mode
***********************************************************************************************/
int icn85xx_write_reg(unsigned short addr, char para)
{
    char buf[3];
    int ret = -1;

    buf[0] = para;
    ret = icn85xx_i2c_txdata(addr, buf, 1);
    if (ret < 0) {
        icn85xx_error("write reg failed! %#x ret: %d\n", buf[0], ret);
        return -1;
    }
    
    return ret;
}


/***********************************************************************************************
Name    :   icn85xx_read_reg 
Input   :   addr
            pdata
Output  :   
function    :   read register of icn85xx, normal mode
***********************************************************************************************/
int icn85xx_read_reg(unsigned short addr, char *pdata)
{
    int ret = -1;
    ret = icn85xx_i2c_rxdata(addr, pdata, 1);
   // printk("addr: 0x%x: 0x%x\n", addr, *pdata);  
    return ret;    
}


/***********************************************************************************************
Name    :   icn85xx_log
Input   :   0: rawdata, 1: diff data
Output  :   err type
function    :   calibrate param
***********************************************************************************************/
static void icn85xx_log(char diff)
{
    char row = 0;
    char column = 0;

    char rowMax = 0;
    char columnMax = 0;
    
    int i, j, ret;
    char retvalue = 0;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITH_FLASH_85))
    {
        rowMax = 36;
        columnMax = 24;
    }
    else
    {
        rowMax = 42;
        columnMax = 30;
    }
    icn85xx_read_reg(0x8004, &row);
    icn85xx_read_reg(0x8005, &column);    
    //scan tp rawdata
    icn85xx_write_reg(4, 0x20); 
    mdelay(1);
    for(i=0; i<1000; i++)
    {
        mdelay(1);
        icn85xx_read_reg(2, &retvalue);
        if(retvalue == 1)
            break;
    }     
    if(diff == 0)
    {
        for(i=0; i<row; i++)
        {       
            ret = icn85xx_i2c_rxdata(0x2000 + i*columnMax*2, (char *)&log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            icn85xx_rawdatadump(&log_rawdata[i][0], column, columnMax);  
    
        } 
    }
    if(diff == 1)
    {
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x3000 + (i+1)*(columnMax+2)*2 + 2, (char *)&log_diffdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            icn85xx_rawdatadump(&log_diffdata[i][0], column, columnMax);   
            
        }           
    }
    else if(diff == 2)
    {        
        for(i=0; i<row; i++)
        {       
            ret = icn85xx_i2c_rxdata(0x2000 + i*columnMax*2, (char *)&log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            if((log_basedata[0][0] != 0) || (log_basedata[0][1] != 0))
            {
                for(j=0; j<column; j++)
                {
                    log_rawdata[i][j] = log_basedata[i][j] - log_rawdata[i][j];
                }
            }
            icn85xx_rawdatadump(&log_rawdata[i][0], column, columnMax);  
    
        }    
        if((log_basedata[0][0] == 0) && (log_basedata[0][1] == 0))
        {
            memcpy(&log_basedata[0][0], &log_rawdata[0][0], COL_NUM*ROW_NUM*2);           
        }
     
        
    }
    
    //finish scan tp rawdata
    icn85xx_write_reg(2, 0x0);      
    icn85xx_write_reg(4, 0x21);    
 
}

/***********************************************************************************************
Name    :   icn85xx_testTP
Input   :   0: ca, 1: diff test
Output  :   err type
function    :   test TP
***********************************************************************************************/
static int icn85xx_testTP(char type, int para1, int para2, int para3)
{
    char retvalue = 0;  //ok
    char ret;
    char row = 0;
    char column = 0;
    char rowMax = 0;
    char columnMax = 0;
    char ScanMode = 0;
    char ictype = 85;
    int diffMax = 0;
    int diffMin = 100000;
    char vol, regValue;
    int i, j;
    int regAddr;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITH_FLASH_85))
    {
        rowMax = 36;
        columnMax = 24;
        ictype = 85;
    }
    else
    {
        rowMax = 42;
        columnMax = 30;
        ictype = 86;
    }
    
    icn85xx_read_reg(0x8004, &row);
    icn85xx_read_reg(0x8005, &column);  

    icn85xx_ts->work_mode = 4; //idle
    if(type == 0)
    {
        memset(&log_basedata[0][0], 0, COL_NUM*ROW_NUM*2);
        icn85xx_log(0);
        for(i=0; i < row; i++)
        {
            for (j = 0; j < (column-1); j++) 
            {                
                log_basedata[i][j] = log_rawdata[i][j+1] - log_rawdata[i][j];
            }            
        }
                        
        for(i=0; i < (row-1); i++)
        {
            for (j = 0; j < (column-1); j++) 
            {                
                log_rawdata[i][j]  = log_basedata[i+1][j] -  log_basedata[i][j];
                if(abs(log_rawdata[i][j]) >  abs(para1))
                {
                    retvalue = 1;  // error
                }
            }
            icn85xx_rawdatadump(&log_rawdata[i][0], column-1, columnMax);
        }
        
    }
    else if(type == 1)
    {
        memset(&log_rawdata[0][0], 0, COL_NUM*ROW_NUM*2);

        if(ictype == 86)
        {
            icn85xx_read_reg(0x8000+87, &ScanMode);
            icn85xx_write_reg(0x8000+87, 0);  //change scanmode

            
            printk("reinit tp1, ScanMode: %d\n", ScanMode);
            icn85xx_write_reg(0, 1); 
            mdelay(100);
            icn85xx_write_reg(0, 0);   
            mdelay(100);
            regAddr =0x040b38;
            ret = icn85xx_i2c_txdata(0xf000, &regAddr, 4);
            if (ret < 0) {
                icn85xx_error("1 write reg failed! ret: %d\n", ret);
                return -1;
            }
            ret = icn85xx_i2c_rxdata(0xf004, &regValue, 1);
            printk("regValue: 0x%x\n", regValue);
            ret = icn85xx_i2c_txdata(0xf000, &regAddr, 4);
            if (ret < 0) {
                icn85xx_error("2 write reg failed! ret: %d\n", ret);
                return -1;
            }
            regAddr = regValue&0xfe;
            ret = icn85xx_i2c_txdata(0xf004, &regAddr, 1);
            if (ret < 0) {
                icn85xx_error("3 write reg failed! ret: %d\n", ret);
                return -1;
            }
        }
        mdelay(100);
        for(i=0; i<5; i++)
            icn85xx_log(0);

        vol = (para1&0xff) | 0x10;
        printk("-------vol: 0x%x\n", vol);
        icn85xx_write_reg(4, vol);
        memcpy(&log_basedata[0][0], &log_rawdata[0][0], COL_NUM*ROW_NUM*2); 
        mdelay(100);
        for(i=0; i<5; i++)
            icn85xx_log(0);

        icn85xx_write_reg(4, 0x10);
        
        if(ictype == 86)
        {
            icn85xx_write_reg(0x8000+87, ScanMode);

            printk("reinit tp2\n");
            icn85xx_write_reg(0, 1); 
            mdelay(100);
            icn85xx_write_reg(0, 0); 
            mdelay(100);

            regAddr =0x040b38;
            ret = icn85xx_i2c_txdata(0xf000, &regAddr, 4);
            if (ret < 0) {
                icn85xx_error("4 write reg failed! ret: %d\n", ret);
                return -1;
            }
            ret = icn85xx_i2c_txdata(0xf004, &regValue, 1);
            if (ret < 0) {
                icn85xx_error("5 write reg failed! ret: %d\n", ret);
                return -1;
            }

            

        }
        update_data2 = para2*120/100;
        update_data3 = para3*80/100;
        printk("-------diff data: %d, %d\n", update_data2, update_data3);
        for(i=0; i < row; i++)
        {
            for (j = 0; j < column; j++) 
            {                
                log_diffdata[i][j] = log_basedata[i][j] - log_rawdata[i][j];               
                if(log_diffdata[i][j] > diffMax)
                {
                    diffMax = log_diffdata[i][j];
                }
                else if(log_diffdata[i][j] < diffMin)
                {
                    diffMin = log_diffdata[i][j];
                } 
                
                if((update_data2 > 0) && (update_data3 > 0))  // make sure Max/Min > 0
                {
                    if((log_diffdata[i][j] > update_data2) || (log_diffdata[i][j] < update_data3))
                    {
                        retvalue = 1;  // error
                    }
                }    
            }         
           
            icn85xx_rawdatadump(&log_diffdata[i][0], column, columnMax);
        }

        update_data2 = diffMax;
        update_data3 = diffMin;
        
    }
    icn85xx_ts->work_mode = 0;

    return retvalue;
}

/***********************************************************************************************
Name    :   icn85xx_iic_test 
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_iic_test(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    int  ret = -1;
    char value = 0;
    char buf[3];
    int  retry = 0;
    int  flashid;
    icn85xx_ts->ictype = 0;
    icn85xx_trace("====%s begin=====.  \n", __func__);
    icn85xx_ts->ictype = ICTYPE_UNKNOWN;
    
    while(retry++ < 3)
    {       
        ret = icn85xx_read_reg(0xa, &value);
        if(ret > 0)
        {
            if(value == 0x85)
            {
                icn85xx_ts->ictype = ICN85XX_WITH_FLASH_85;
                setbootmode(ICN85XX_WITH_FLASH_85);
                return ret;
            }
            else if((value == 0x86)||(value == 0x88))
            {
                icn85xx_ts->ictype = ICN85XX_WITH_FLASH_86;
                setbootmode(ICN85XX_WITH_FLASH_86);
                return ret;  
            }
            else if(value == 0x87)
            {
                icn85xx_ts->ictype = ICN85XX_WITH_FLASH_87;
                setbootmode(ICN85XX_WITH_FLASH_87);
                return ret;  
            }
        }
        
        icn85xx_error("iic test error! retry = %d\n", retry);
        msleep(3);
    }

    // add junfuzhang 20131211
    // force ic enter progmode
    icn85xx_goto_progmode();
    msleep(10);
    
    retry = 0;
    while(retry++ < 3)
    {       
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	ret = icn85xx_prog_i2c_txdata(0x040000,buf,3);
	if (ret < 0) {
		icn85xx_error("write reg failed! ret: %d\n", ret);
		return ret;
	}    	
    	
        ret = icn85xx_prog_i2c_rxdata(0x040000, buf, 3);
        icn85xx_trace("icn85xx_check_progmod: %d, 0x%2x, 0x%2x, 0x%2x\n", ret, buf[0], buf[1], buf[2]);
        if(ret > 0)
        {
            if((buf[2] == 0x85) && (buf[1] == 0x05))
            {
                flashid = icn85xx_read_flashid();
                if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
                    ||(MD25D20_ID1 == flashid) || (MD25D20_ID1 == flashid)
                    ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid)
                    || (MD25D05_ID == flashid)|| (MD25D10_ID == flashid))
                {
                    icn85xx_ts->ictype = ICN85XX_WITH_FLASH_85;
                    setbootmode(ICN85XX_WITH_FLASH_85);
                }
                else
                {
                    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_85;
                    setbootmode(ICN85XX_WITHOUT_FLASH_85);
                }
                return ret;
            }
            else if(((buf[2] == 0x85) && (buf[1] == 0x0e))||(buf[2] == 0x88))
            {
                flashid = icn85xx_read_flashid();
                if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
                    ||(MD25D20_ID1 == flashid) || (MD25D20_ID1 == flashid)
                    ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid)
                    || (MD25D05_ID == flashid)|| (MD25D10_ID == flashid))
                {
                    icn85xx_ts->ictype = ICN85XX_WITH_FLASH_86;
                    setbootmode(ICN85XX_WITH_FLASH_86);
                }
                else
                {
                    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_86;
                    setbootmode(ICN85XX_WITHOUT_FLASH_86);
                }
                return ret;
            }
            else  //for ICNT87
            {                
                ret = icn87xx_prog_i2c_rxdata(0xf001, buf, 2);
                if(ret > 0)                    
                {
                    if(buf[1] == 0x87)
                    {                        
                        flashid = icn87xx_read_flashid();                        
                        printk("icnt87 flashid: 0x%x\n",flashid);
                        if(0x114051 == flashid)
                        {
                            icn85xx_ts->ictype = ICN85XX_WITH_FLASH_87;  
                            setbootmode(ICN85XX_WITH_FLASH_87);
                        }                        
                        else
                        {
                            icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_87;  
                            setbootmode(ICN85XX_WITHOUT_FLASH_87);
                        }
                        return ret;
                    }

                }


            }
        }          
        icn85xx_error("iic2 test error! %d\n", retry);
        msleep(3);
    }    
    
    return ret;    
}
/***********************************************************************************************
Name    :   icn85xx_ts_release 
Input   :   void
Output  :   
function    : touch release
***********************************************************************************************/
static void icn85xx_ts_release(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    icn85xx_info("==icn85xx_ts_release ==\n");
    input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_sync(icn85xx_ts->input_dev);
}

/***********************************************************************************************
Name    :   icn85xx_report_value_A
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
static void icn85xx_report_value_A(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    char buf[POINT_NUM*POINT_SIZE+3]={0};
    int ret = -1;
    int i;
    icn85xx_info("==icn85xx_report_value_A ==\n");
#if TOUCH_VIRTUAL_KEYS
    unsigned char button;
    static unsigned char button_last;
#endif

    ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
        icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        return ;
    }
#if TOUCH_VIRTUAL_KEYS    
    button = buf[0];    
    icn85xx_info("%s: button=%d\n",__func__, button);

    if((button_last != 0) && (button == 0))
    {
        icn85xx_ts_release();
        button_last = button;
        return ;       
    }
    if(button != 0)
    {
        switch(button)
        {
            case ICN_VIRTUAL_BUTTON_HOME:
                icn85xx_info("ICN_VIRTUAL_BUTTON_HOME down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 280);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);            
                break;
            case ICN_VIRTUAL_BUTTON_BACK:
                icn85xx_info("ICN_VIRTUAL_BUTTON_BACK down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 470);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);                
                break;
            case ICN_VIRTUAL_BUTTON_MENU:
                icn85xx_info("ICN_VIRTUAL_BUTTON_MENU down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 100);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);            
                break;                      
            default:
                icn85xx_info("other gesture\n");
                break;          
        }
        button_last = button;
        return ;
    }        
#endif
 
    icn85xx_ts->point_num = buf[1];    
    if (icn85xx_ts->point_num == 0) {
        icn85xx_ts_release();
        return ; 
    }   
    for(i=0;i<icn85xx_ts->point_num;i++){
        if(buf[8 + POINT_SIZE*i]  != 4)
        {
          break ;
        }
        else
        {
        
        }
    }
    
    if(i == icn85xx_ts->point_num) {
        icn85xx_ts_release();
        return ; 
    }   

    for(i=0; i<icn85xx_ts->point_num; i++)
    {
        icn85xx_ts->point_info[i].u8ID = buf[2 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u16PosX = (buf[4 + POINT_SIZE*i]<<8) + buf[3 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u16PosY = (buf[6 + POINT_SIZE*i]<<8) + buf[5 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u8Pressure = buf[7 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u8EventId = buf[8 + POINT_SIZE*i];    

        if(1 == icn85xx_ts->revert_x_flag)
        {
            icn85xx_ts->point_info[i].u16PosX = icn85xx_ts->screen_max_x- icn85xx_ts->point_info[i].u16PosX;
        }
        if(1 == icn85xx_ts->revert_y_flag)
        {
            icn85xx_ts->point_info[i].u16PosY = icn85xx_ts->screen_max_y- icn85xx_ts->point_info[i].u16PosY;
        }
        
        icn85xx_info("u8ID %d\n", icn85xx_ts->point_info[i].u8ID);
        //icn85xx_info("u16PosX %d\n", icn85xx_ts->point_info[i].u16PosX);
        icn85xx_info("u16PosY %d\n", icn85xx_ts->point_info[i].u16PosY);
        //icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[i].u8Pressure);
        icn85xx_info("u8EventId %d\n", icn85xx_ts->point_info[i].u8EventId);  


        input_report_abs(icn85xx_ts->input_dev, ABS_MT_TRACKING_ID, icn85xx_ts->point_info[i].u8ID);    
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, icn85xx_ts->point_info[i].u8Pressure);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[i].u16PosX);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[i].u16PosY);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
        input_mt_sync(icn85xx_ts->input_dev);
        icn85xx_point_info("point: %d ===x = %d,y = %d, press = %d ====\n",i, icn85xx_ts->point_info[i].u16PosX,icn85xx_ts->point_info[i].u16PosY, icn85xx_ts->point_info[i].u8Pressure);
    }

    input_sync(icn85xx_ts->input_dev);
  
}
/***********************************************************************************************
Name    :   icn85xx_report_value_B
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
#if CTP_REPORT_PROTOCOL
static void icn85xx_report_value_B(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    char buf[POINT_NUM*POINT_SIZE+4]={0};
//  char buf_check = 0,i,j;
    static unsigned char finger_last[POINT_NUM + 1]={0};
    unsigned char  finger_current[POINT_NUM + 1] = {0};
    unsigned int position = 0;
    int temp = 0;
    int ret = -1;
    icn85xx_trace("==icn85xx_report_value_B ==\n");
     
    ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
         icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
          return ;
      }      
    icn85xx_ts->point_num = buf[1];
    if (icn85xx_ts->point_num > POINT_NUM)
     {
       return ;
     }

    if(icn85xx_ts->point_num > 0)
    {
        for(position = 0; position<icn85xx_ts->point_num; position++)
        {       
            temp = buf[2 + POINT_SIZE*position] + 1;
            finger_current[temp] = 1;
            icn85xx_ts->point_info[temp].u8ID = buf[2 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u16PosX = (buf[4 + POINT_SIZE*position]<<8) + buf[3 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u16PosY = (buf[6 + POINT_SIZE*position]<<8) + buf[5 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u8Pressure = buf[7 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u8EventId = buf[8 + POINT_SIZE*position];
            
            if(icn85xx_ts->point_info[temp].u8EventId == 4)
                finger_current[temp] = 0;
                            
            if(1 == icn85xx_ts->revert_x_flag)
            {
                icn85xx_ts->point_info[temp].u16PosX = icn85xx_ts->screen_max_x- icn85xx_ts->point_info[temp].u16PosX;
            }
            if(1 == icn85xx_ts->revert_y_flag)
            {
                icn85xx_ts->point_info[temp].u16PosY = icn85xx_ts->screen_max_y- icn85xx_ts->point_info[temp].u16PosY;
            }
            //icn85xx_info("temp %d\n", temp);
     //       icn85xx_info("u8ID %d\n", icn85xx_ts->point_info[temp].u8ID);
       //     icn85xx_info("u16PosX %d\n", icn85xx_ts->point_info[temp].u16PosX);
            //icn85xx_info("u16PosY %d\n", icn85xx_ts->point_info[temp].u16PosY);
           // icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[temp].u8Pressure);
         //   icn85xx_info("u8EventId %d\n", icn85xx_ts->point_info[temp].u8EventId);             
            //icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[temp].u8Pressure*16);
        }
    }   
    else
    {
        for(position = 1; position < POINT_NUM+1; position++)
        {
            finger_current[position] = 0;
        }
        icn85xx_info("no touch\n");
    }

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        if((finger_current[position] == 0) && (finger_last[position] != 0))
        {
            input_mt_slot(icn85xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, false);
            icn85xx_point_info("one touch up: %d\n", position);
            input_report_abs(icn85xx_ts->input_dev, ABS_PRESSURE, 0);
        }
        else if(finger_current[position])
        {
            input_mt_slot(icn85xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, true);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
            //input_report_abs(icn85xx_ts->input_dev, ABS_MT_PRESSURE, icn85xx_ts->point_info[position].u8Pressure);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_PRESSURE, 200);
            input_report_abs(icn85xx_ts->input_dev, ABS_PRESSURE, 200);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[position].u16PosX);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[position].u16PosY);
            icn85xx_trace("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn85xx_ts->point_info[position].u16PosX,icn85xx_ts->point_info[position].u16PosY, icn85xx_ts->point_info[position].u8Pressure);
        }

    }
    input_sync(icn85xx_ts->input_dev);

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        finger_last[position] = finger_current[position];
    }
    
}
#endif

/***********************************************************************************************
Name    :   icn85xx_ts_pen_irq_work
Input   :   void
Output  :   
function    : work_struct
***********************************************************************************************/
static void icn85xx_ts_pen_irq_work(struct work_struct *work)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);  
    int ret = -1;

#if SUPPORT_PROC_FS
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return ;   
    }  
#endif
      
	if(icn85xx_ts->work_mode == 0)
    {
#if CTP_REPORT_PROTOCOL
        icn85xx_report_value_B();
#else
        icn85xx_report_value_A();
#endif 


        if(icn85xx_ts->use_irq)
        {
            //zyt icn85xx_irq_enable();
               ret = input_set_int_enable(&(config_info.input_type), 1);
               if (ret < 0)
                       printk("%s irq ensable failed\n", ICN85XX_NAME);           
        }
        if(log_on_off == 4)
        {
            printk("normal raw data\n");
            icn85xx_log(0);   //raw data
        }
        else if(log_on_off == 5)
        {
            printk("normal diff data\n");
            icn85xx_log(1);   //diff data         
        }
        else if(log_on_off == 6)
        {
            printk("normal raw2diff\n");
            icn85xx_log(2);   //diff data         
        }
    }
    else if(icn85xx_ts->work_mode == 1)
    {
        printk("raw data\n");
        icn85xx_log(0);   //raw data
    }
    else if(icn85xx_ts->work_mode == 2)
    {
        printk("diff data\n");
        icn85xx_log(1);   //diff data
    }
    else if(icn85xx_ts->work_mode == 3)
    {
        printk("raw2diff data\n");
        icn85xx_log(2);   //diff data
    }
    else if(icn85xx_ts->work_mode == 4)  //idle
    {
        ;
    }
    else if(icn85xx_ts->work_mode == 5)//write para, reinit
    {
        printk("reinit tp\n");
        icn85xx_write_reg(0, 1); 
        mdelay(100);
        icn85xx_write_reg(0, 0);            
        icn85xx_ts->work_mode = 0;
    }
    else if((icn85xx_ts->work_mode == 6) ||(icn85xx_ts->work_mode == 7))  //gesture test mode
    {    
        #if SUPPORT_PROC_FS
        char buf[sizeof(structGestureData)]={0};
        int ret = -1;
        int i;
        ret = icn85xx_i2c_rxdata(0x7000, buf, 2);
        if (ret < 0) {
            icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            return ;
        }            
        structGestureData.u8Status = 1;
        structGestureData.u8Gesture = buf[0];
        structGestureData.u8GestureNum = buf[1];
        proc_info("structGestureData.u8Gesture: 0x%x\n", structGestureData.u8Gesture);
        proc_info("structGestureData.u8GestureNum: %d\n", structGestureData.u8GestureNum);

        ret = icn85xx_i2c_rxdata(0x7002, buf, structGestureData.u8GestureNum*6);
        if (ret < 0) {
            icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            return ;
        }  
       
        for(i=0; i<structGestureData.u8GestureNum; i++)
        {
            structGestureData.point_info[i].u16PosX = (buf[1 + 6*i]<<8) + buf[0+ 6*i];
            structGestureData.point_info[i].u16PosY = (buf[3 + 6*i]<<8) + buf[2 + 6*i];
            structGestureData.point_info[i].u8EventId = buf[5 + 6*i];
            proc_info("(%d, %d, %d)", structGestureData.point_info[i].u16PosX, structGestureData.point_info[i].u16PosY, structGestureData.point_info[i].u8EventId);
        } 
        proc_info("\n");
        /*if(icn85xx_ts->use_irq)
        {
            icn85xx_irq_enable();
        }*/

        if(((icn85xx_ts->work_mode == 7) && (structGestureData.u8Gesture == 0xFB))
            || (icn85xx_ts->work_mode == 6))
        {
            proc_info("return normal mode\n");
            icn85xx_ts->work_mode = 0;  //return normal mode
        }
        
        #endif	
    }


#if SUPPORT_PROC_FS
    up(&icn85xx_ts->sem);
#endif


}
/***********************************************************************************************
Name    :   chipone_timer_func
Input   :   void
Output  :   
function    : Timer interrupt service routine.
***********************************************************************************************/
static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer)
{
    struct icn85xx_ts_data *icn85xx_ts = container_of(timer, struct icn85xx_ts_data, timer);
    queue_work(icn85xx_ts->ts_workqueue, &icn85xx_ts->pen_event_work);
    //icn85xx_info("chipone_timer_func\n"); 
    if(icn85xx_ts->use_irq == 1)
    {
        if((icn85xx_ts->work_mode == 1) || (icn85xx_ts->work_mode == 2) || (icn85xx_ts->work_mode == 3))
        {
            hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_POLL_TIMER/1000, (CTP_POLL_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }
    }
    else
    {
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_POLL_TIMER/1000, (CTP_POLL_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
    return HRTIMER_NORESTART;
}
/***********************************************************************************************
Name    :   icn85xx_ts_interrupt
Input   :   void
Output  :   
function    : interrupt service routine
***********************************************************************************************/
static irqreturn_t icn85xx_ts_interrupt(int irq, void *dev_id)
{
    struct icn85xx_ts_data *icn85xx_ts = dev_id;
    int ret = -1;
    icn85xx_trace("==========------icn85xx_ts TS Interrupt-----============\n"); 

    if(icn85xx_ts->use_irq){
        //zyt icn85xx_irq_disable();
        ret = input_set_int_enable(&(config_info.input_type), 0);
        if (ret < 0)
					printk("%s irq dissable failed\n", ICN85XX_NAME);
    }
    if (!work_pending(&icn85xx_ts->pen_event_work)) 
    {
       // icn85xx_info("Enter work\n");
        queue_work(icn85xx_ts->ts_workqueue, &icn85xx_ts->pen_event_work);

    }

    return IRQ_HANDLED;
}

/***********************************************************************************************
Name    :   icn85xx_ts_suspend
Input   :   void
Output  :
function    : tp enter sleep mode
***********************************************************************************************/
static void icn85xx_ts_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(client);
//  unsigned char i = 0;
		int ret = -1;

    icn85xx_trace("icn85xx_ts_suspend\n");
    if (icn85xx_ts->use_irq)
    {
        //zyt icn85xx_irq_disable();
        ret = input_set_int_enable(&(config_info.input_type), 0);
        if (ret < 0)
		printk("%s irq dissable failed\n", ICN85XX_NAME);
    }
    else
    {
        hrtimer_cancel(&icn85xx_ts->timer);
    }
    //}         // flush_workqueue(icn85xx_ts->workqueue);
    //reset flag if ic is flashless when power off
    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_86)
        || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87))
    {
        icn85xx_ts->code_loaded_flag = 0;
    }

#if SUSPEND_POWER_OFF
    //power off
    //todo
   //zyt regulator_disable(tp_regulator);
 //  input_set_power_enable(&(config_info.input_type), 0); //add by zyt
#else
      icn85xx_write_reg(ICN85XX_REG_PMODE, PMODE_HIBERNATE);
    //icn85xx_write_reg(ICN85XX_REG_PMODE, 0x40);    //gesture mode
#endif
    icn85xx_ts->work_mode = 6;

}

/***********************************************************************************************
Name    :   icn85xx_ts_resume
Input   :   void
Output  :
function    : wakeup tp or reset tp
***********************************************************************************************/
static void icn85xx_ts_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(client);
    int i;
    int ret = -1;
    int retry = 5;
    int need_update_fw = false;
    unsigned char value;
    icn85xx_trace("==icn85xx_ts_resume== \n");
//report touch release
//	input_set_power_enable(&(config_info.input_type), 1); //add by zyt

#if CTP_REPORT_PROTOCOL
    for(i = 0; i < POINT_NUM; i++)
    {
        input_mt_slot(icn85xx_ts->input_dev, i);
        input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, false);
    }
#else
    icn85xx_ts_release();
#endif
 /*
		ctp_wakeup(0, CTP_WAKEUP_LOW_PERIOD);
                msleep(CTP_WAKEUP_HIGH_PERIOD);
        if (icn85xx_ts->use_irq) {
		       ret = input_set_int_enable(&(config_info.input_type), 1);
		       if (ret < 0)
		           printk("%s irq ensable failed\n", ICN85XX_NAME);
         } else {
           hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }
*/
///*
#if SUSPEND_POWER_OFF
    //power on tp
    //todo
     regulator_enable(tp_regulator);

#else

    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_86))
    {
        while (retry-- && !need_update_fw)
        {
            icn85xx_ts_reset();
            icn85xx_bootfrom_sram();
            msleep(50);
            ret = icn85xx_read_reg(0xa, &value);
            if (ret > 0)
            {
                need_update_fw = false;
                break;
            }
        }
        if (retry <= 0) need_update_fw = true;

        if (need_update_fw)
        {
            if(R_OK == icn85xx_fw_update(firmware))
            {
                icn85xx_ts->code_loaded_flag = 1;
                icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code ok\n");
            }
            else
            {
                icn85xx_ts->code_loaded_flag = 0;
                icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code error\n");
            }
        }
    }
    else if(icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87)
    {
        while (retry-- && !need_update_fw)
        {
            //icn85xx_ts_reset();
            icn85xx_set_prog_addr();
            icn87xx_boot_sram();
            msleep(50);
            ret = icn85xx_read_reg(0xa, &value);
            if (ret > 0)
            {
                need_update_fw = false;
                break;
            }
        }
        if (retry <= 0) need_update_fw = true;

        if (need_update_fw)
        {
            if(R_OK == icn87xx_fw_update(firmware))
            {
                icn85xx_ts->code_loaded_flag = 1;
                icn85xx_trace("ICN87XX_WITHOUT_FLASH, reload code ok\n"); 
            }
            else
            {
                icn85xx_ts->code_loaded_flag = 0;
                icn85xx_trace("ICN87XX_WITHOUT_FLASH, reload code error\n"); 
            }
        }
    }
    else
    {
        icn85xx_ts_reset();
    }
#endif
   if (icn85xx_ts->use_irq)
    {
        //icn85xx_irq_enable();
		    ret = input_set_int_enable(&(config_info.input_type), 1);
		    if(ret < 0)
		       printk("%s irq ensable failed\n", ICN85XX_NAME);
    }
    else
    {
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
    icn85xx_ts->work_mode = 0;
///    */
}

/***********************************************************************************************
Name    :   icn85xx_request_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
/*
static void icn85xx_request_io_port(struct icn85xx_ts_data *icn85xx_ts)
{
    icn85xx_ts->screen_max_x = SCREEN_MAX_X;
    icn85xx_ts->screen_max_y = SCREEN_MAX_Y;
    icn85xx_ts->irq = CTP_IRQ_PORT;
    return ;
}
*
/***********************************************************************************************
Name    :   icn85xx_free_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static void icn85xx_free_io_port(struct icn85xx_ts_data *icn85xx_ts)
{    
    return ;
}

/***********************************************************************************************
Name    :   icn85xx_request_irq
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
/*static int icn85xx_request_irq(struct icn85xx_ts_data *icn85xx_ts)
{
    int err = -1;
    icn85xx_ts->irq = IRQ_SIRQ0;


    err = request_irq(icn85xx_ts->irq, icn85xx_ts_interrupt, IRQ_TYPE_EDGE_FALLING, "icn85xx_ts", icn85xx_ts);
    if (err < 0) 
    {
        icn85xx_ts->use_irq = 0;
        icn85xx_error("icn85xx_ts_probe: request irq failed\n");
        return err;
    } 
    icn85xx_irq_disable();
    icn85xx_ts->use_irq = 1;
    return 0;
}
*/

/***********************************************************************************************
Name    :   icn85xx_free_irq
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static void icn85xx_free_irq(struct icn85xx_ts_data *icn85xx_ts)
{
    if (icn85xx_ts) 
    {
        if (icn85xx_ts->use_irq)
        {
            free_irq(icn85xx_ts->irq, icn85xx_ts);
        }
        else
        {
            hrtimer_cancel(&icn85xx_ts->timer);
        }
    }
     
}

/***********************************************************************************************
Name    :   icn85xx_request_input_dev
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_request_input_dev(struct icn85xx_ts_data *icn85xx_ts)
{
    int ret = -1;    
    struct input_dev *input_dev;

    input_dev = input_allocate_device();
    if (!input_dev) {
        icn85xx_error("failed to allocate input device\n");
        return -ENOMEM;
    }
    icn85xx_ts->input_dev = input_dev;

    icn85xx_ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if CTP_REPORT_PROTOCOL
    __set_bit(INPUT_PROP_DIRECT, icn85xx_ts->input_dev->propbit);
    input_mt_init_slots(icn85xx_ts->input_dev, POINT_NUM*2, INPUT_MT_DIRECT);
#else
    set_bit(ABS_MT_TOUCH_MAJOR, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_X, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_Y, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_WIDTH_MAJOR, icn85xx_ts->input_dev->absbit); 
#endif
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 0, screen_max_x, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 0, screen_max_y, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);  
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_TRACKING_ID, 0, POINT_NUM*2, 0, 0);

    input_set_abs_params(icn85xx_ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);

    __set_bit(KEY_MENU,  input_dev->keybit);
    __set_bit(KEY_BACK,  input_dev->keybit);
    __set_bit(KEY_HOME,  input_dev->keybit);
    __set_bit(KEY_SEARCH,  input_dev->keybit);

    input_dev->name = CTP_NAME;
    ret = input_register_device(input_dev);
    if (ret) {
        icn85xx_error("Register %s input device failed\n", input_dev->name);
        input_free_device(input_dev);
        return -ENODEV;        
    }

       return 0;
}
#if SUPPORT_SENSOR_ID
static void read_sensor_id(void)
{
        int i,ret;
        //icn85xx_trace("scan sensor id value begin sensor_id_num = %d\n",(sizeof(sensor_id_table)/sizeof(sensor_id_table[0])));
        ret = icn85xx_read_reg(0x10, &cursensor_id);
        if(ret > 0)
            {   
              icn85xx_trace("cursensor_id= 0x%x\n", cursensor_id); 
            }
        else
            {
              icn85xx_error("icn85xx read cursensor_id failed.\n");
              cursensor_id = -1;
            }

         ret = icn85xx_read_reg(0x1e, &tarsensor_id);
        if(ret > 0)
            {   
              icn85xx_trace("tarsensor_id= 0x%x\n", tarsensor_id);
              tarsensor_id = -1;
            }
        else
            {
              icn85xx_error("icn85xx read tarsensor_id failed.\n");
            }
         ret = icn85xx_read_reg(0x1f, &id_match);
        if(ret > 0)
            {   
              icn85xx_trace("match_flag= 0x%x\n", id_match); // 1: match; 0:not match
            }
        else
            {
              icn85xx_error("icn85xx read id_match failed.\n");
              id_match = -1;
            }
        // scan sensor id value
    icn85xx_trace("begin to scan id table,find correct fw or bin. sensor_id_num = %d\n",(sizeof(sensor_id_table)/sizeof(sensor_id_table[0])));
   
   for(i = 0;i < (sizeof(sensor_id_table)/sizeof(sensor_id_table[0])); i++)  // not change tp
    {
        if (cursensor_id == sensor_id_table[i].value)
            {
                #if COMPILE_FW_WITH_DRIVER
                    icn85xx_set_fw(sensor_id_table[i].size, sensor_id_table[i].fw_name);    
                #else
                    strcpy(firmware,sensor_id_table[i].bin_name);
                    icn85xx_trace("icn85xx matched firmware = %s\n", firmware);
                #endif
                    icn85xx_trace("icn85xx matched id = 0x%x\n", sensor_id_table[i].value);
                    invalid_id = 1;
                    break;
            }
        else
            { 
              invalid_id = 0;
              icn85xx_trace("icn85xx not matched id%d= 0x%x\n", i,sensor_id_table[i].value);
              //icn85xx_trace("not match sensor_id_table[%d].value= 0x%x,bin_name = %s\n",i,sensor_id_table[i].value,sensor_id_table[i].bin_name);
            }
     }
        
}
static void compare_sensor_id(void)
{
   int retry = 5;
   
   read_sensor_id(); // select sensor id 
   
   if(0 == invalid_id)   //not compare sensor id,update default fw or bin
    {
      icn85xx_trace("not compare sensor id table,update default: invalid_id= %d, cursensor_id= %d\n", invalid_id,cursensor_id);
      #if COMPILE_FW_WITH_DRIVER
               icn85xx_set_fw(sensor_id_table[0].size, sensor_id_table[0].fw_name);
      #else
                strcpy(firmware,sensor_id_table[0].bin_name);
                icn85xx_trace("match default firmware = %s\n", firmware);
      #endif
    
      while(retry > 0)
            {
                if(R_OK == icn85xx_fw_update(firmware))
                {
                    icn85xx_trace("icn85xx upgrade default firmware ok\n");
                    break;
                }
                retry--;
                icn85xx_error("icn85xx_fw_update default firmware failed.\n");   
            }
    }
   
   if ((1 == invalid_id)&&(0 == id_match))  // tp is changed,update current fw or bin
    {
        icn85xx_trace("icn85xx detect tp is changed!!! invalid_id= %d,id_match= %d,\n", invalid_id,id_match);
         while(retry > 0)
            {
                if(R_OK == icn85xx_fw_update(firmware))
                {
                    icn85xx_trace("icn85xx upgrade cursensor id firmware ok\n");
                    break;
                }
                retry--;
                icn85xx_error("icn85xx_fw_update current id firmware failed.\n");   
            }
    }
}
#endif

static void icn85xx_update(struct icn85xx_ts_data *icn85xx_ts)
{
    short fwVersion = 0;
    short curVersion = 0;
    int retry = 0;
    
    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_86))
    {
        #if (COMPILE_FW_WITH_DRIVER && !SUPPORT_SENSOR_ID)
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif
        
        #if SUPPORT_SENSOR_ID
        while(0 == invalid_id ) //reselect sensor id  
        {
          compare_sensor_id();   // select sensor id
          icn85xx_trace("invalid_id= %d\n", invalid_id);
        }
        #else
        if(R_OK == icn85xx_fw_update(firmware))
        {
            icn85xx_ts->code_loaded_flag = 1;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update default fw ok\n");
        }
        else
        {
            icn85xx_ts->code_loaded_flag = 0;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update error\n"); 
        }
        #endif

    }
    else if((icn85xx_ts->ictype == ICN85XX_WITH_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITH_FLASH_86))
    {
        #if (COMPILE_FW_WITH_DRIVER && !SUPPORT_SENSOR_ID)
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif 
        
        #if SUPPORT_SENSOR_ID
        while(0 == invalid_id ) //reselect sensor id  
        {
            compare_sensor_id();   // select sensor id
            if( 1 == invalid_id)
            {
              icn85xx_trace("select sensor id ok. begin compare fwVersion with curversion\n");
            }
        }
        #endif
        
        fwVersion = icn85xx_read_fw_Ver(firmware);
        curVersion = icn85xx_readVersion();
        icn85xx_trace("fwVersion : 0x%x\n", fwVersion); 
        icn85xx_trace("current version: 0x%x\n", curVersion);  

        #if FORCE_UPDATA_FW
            retry = 5;
            while(retry > 0)
            {
                if(icn85xx_goto_progmode() != 0)
                {
                    printk("icn85xx_goto_progmode() != 0 error\n");
                    return -1; 
                } 
                icn85xx_read_flashid();
                if(R_OK == icn85xx_fw_update(firmware))
                {
                    break;
                }
                retry--;
                icn85xx_error("icn85xx_fw_update failed.\n");        
            }

        #else
            if(fwVersion > curVersion)
            {
                retry = 5;
                while(retry > 0)
                {
                    if(R_OK == icn85xx_fw_update(firmware))
                    {
                        break;
                    }
                    retry--;
                    icn85xx_error("icn85xx_fw_update failed.\n");   
                }
            }
        #endif
    }
    else if(icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87)
    {
        printk("icn85xx_update 87 without flash\n");
        #if (COMPILE_FW_WITH_DRIVER && !SUPPORT_SENSOR_ID)
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif
  
        if(R_OK == icn87xx_fw_update(firmware))
        {
            icn85xx_ts->code_loaded_flag = 1;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update default fw ok\n");
        }
        else
        {
            icn85xx_ts->code_loaded_flag = 0;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update error\n"); 
        }
     
    }
    else if(icn85xx_ts->ictype == ICN85XX_WITH_FLASH_87)
    {
        printk("icn85xx_update 87 with flash\n");

        #if (COMPILE_FW_WITH_DRIVER && !SUPPORT_SENSOR_ID)
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif           
        fwVersion = icn85xx_read_fw_Ver(firmware);
        curVersion = icn85xx_readVersion();
        icn85xx_trace("fwVersion : 0x%x\n", fwVersion); 
        icn85xx_trace("current version: 0x%x\n", curVersion); 
             
        
        #if FORCE_UPDATA_FW        
            if(R_OK == icn87xx_fw_update(firmware))
            {
                icn85xx_ts->code_loaded_flag = 1;
                icn85xx_trace("ICN87XX_WITH_FLASH, update default fw ok\n");
            }
            else
            {
                icn85xx_ts->code_loaded_flag = 0;
                icn85xx_trace("ICN87XX_WITH_FLASH, update error\n"); 
            }    
         
        #else
            if(fwVersion > curVersion)
            {
                retry = 5;
                while(retry > 0)
                {
                    if(R_OK == icn87xx_fw_update(firmware))
                    {
                        break;
                    }
                    retry--;
                    icn85xx_error("icn87xx_fw_update failed.\n");   
                }
            }
        #endif
    }
    else
    {
       //for next ic type
    }
}

static int icn85xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct icn85xx_ts_data *icn85xx_ts;
    int err = 0;
    
 
    printk("20141014  ====%s begin=====.\n", __func__);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        icn85xx_error("I2C check functionality failed.\n");
        return -ENODEV;
    }

    icn85xx_ts = kzalloc(sizeof(*icn85xx_ts), GFP_KERNEL);
    if (!icn85xx_ts)
    {
        icn85xx_error("Alloc icn85xx_ts memory failed.\n");
        return -ENOMEM;
    }
    memset(icn85xx_ts, 0, sizeof(*icn85xx_ts));

    this_client = client;
    this_client->addr = client->addr;
    i2c_set_clientdata(client, icn85xx_ts);

    icn85xx_ts->irq = gpio_to_irq(CTP_IRQ_NUMBER);

    icn85xx_ts_reset();
    
    icn85xx_ts->work_mode = 0;
    spin_lock_init(&icn85xx_ts->irq_lock);
//    icn85xx_ts->irq_lock = SPIN_LOCK_UNLOCKED;

    err = icn85xx_iic_test();
    if (err <= 0)
    {
        icn85xx_error("icn85xx_iic_test  failed.\n");
        return -1;
     
    }
    else
    {
        icn85xx_trace("iic communication ok_: 0x%x\n", icn85xx_ts->ictype); 
    }
    
    icn85xx_update(icn85xx_ts);

    err= icn85xx_request_input_dev(icn85xx_ts);
    if (err < 0)
    {
        icn85xx_error("request input dev failed\n");
        kfree(icn85xx_ts);
        return err;        
    }

  //      icn85xx_request_io_port(icn85xx_ts);

#if TOUCH_VIRTUAL_KEYS
    icn85xx_ts_virtual_keys_init();
#endif
/*
    err = icn85xx_request_irq(icn85xx_ts);
    if (err != 0)
    {
        icn85xx_error("request irq error, use timer\n");
        icn85xx_ts->use_irq = 0;
        hrtimer_init(&icn85xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        icn85xx_ts->timer.function = chipone_timer_func;
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
*/

#if SUPPORT_PROC_FS
    sema_init(&icn85xx_ts->sem, 1);
   init_proc_node();
#endif

    INIT_WORK(&icn85xx_ts->pen_event_work, icn85xx_ts_pen_irq_work);
    icn85xx_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
    if (!icn85xx_ts->ts_workqueue) {
        icn85xx_error("create_singlethread_workqueue failed.\n");
        kfree(icn85xx_ts);
        return -ESRCH;
    }

        config_info.dev = &(icn85xx_ts->input_dev->dev);
        int_handle = input_request_int(&(config_info.input_type), icn85xx_ts_interrupt,CTP_IRQ_MODE, icn85xx_ts);
        if (int_handle) {
                printk("icn85xx_ts_probe: request irq failed\n");
                input_free_int(&(config_info.input_type), icn85xx_ts);
                icn85xx_ts->use_irq = 0;
                hrtimer_init(&icn85xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
                icn85xx_ts->timer.function = chipone_timer_func;
                hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }else {
	        icn85xx_ts->use_irq = 1;
	        printk("icn85xx_ts_probe: request irq success\n");
        }
 
#if SUPPORT_SYSFS
   // icn85xx_create_sysfs(client);
#endif


/*    if(icn85xx_ts->use_irq)
        icn85xx_irq_enable();*/
    printk("==%s over =\n", __func__);    
    return 0;
}

static int icn85xx_ts_remove(struct i2c_client *client)
{
		int ret = -1;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(client);  
    icn85xx_trace("==icn85xx_ts_remove=\n");
    if(icn85xx_ts->use_irq){
         //zyt icn85xx_irq_enable();
         ret = input_set_int_enable(&(config_info.input_type), 0);
         if (ret < 0)
                printk("%s irq disable failed\n", ICN85XX_NAME);
    }
    gpio_free(CTP_RST_PORT);
#if SUPPORT_PROC_FS
    uninit_proc_node();
#endif

#if SUPPORT_SYSFS
    icn85xx_remove_sysfs(client);
#endif
    input_unregister_device(icn85xx_ts->input_dev);
    input_free_device(icn85xx_ts->input_dev);
    cancel_work_sync(&icn85xx_ts->pen_event_work);
    destroy_workqueue(icn85xx_ts->ts_workqueue);
    kfree(icn85xx_ts);    
    i2c_set_clientdata(client, NULL);
    return 0;
}

static SIMPLE_DEV_PM_OPS(icn85xx_pm_ops, icn85xx_ts_suspend, icn85xx_ts_resume);

static const struct i2c_device_id icn85xx_ts_id[] = {
    { CTP_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, icn85xx_ts_id);

static struct i2c_driver icn85xx_ts_driver = {
    .class      = I2C_CLASS_HWMON,
    .probe      = icn85xx_ts_probe,
    .remove     = icn85xx_ts_remove,
    .id_table   = icn85xx_ts_id,
    .driver = {
        .name   = CTP_NAME,
        .owner  = THIS_MODULE,
        #if SUPPORT_SYSFS
          .groups   = icn_drv_grp,
        #endif
	.pm	= &icn85xx_pm_ops,
    },
    .address_list   = normal_i2c,  
};

static struct i2c_board_info tp_info = {
    .type   = CTP_NAME,
    .addr   =  ICN85XX_NORMAL_IIC_ADDR,
};
//Power init
static struct regulator *regulator_init(const char *name, int minvol, int maxvol)
{

    struct regulator *power;

    power = regulator_get(NULL,"sensor28");// name);
    if (IS_ERR(power)) {
        printk("Nova err,regulator_get fail\n!!!");
        return NULL;
    }

    if (regulator_set_voltage(power, minvol, maxvol)) {
        printk("Nova err,cannot set voltage\n!!!");
        regulator_put(power);
        
        return NULL;
    }
    regulator_enable(power);
    return (power);
}

static int icn85xx_hw_init(void)
{
    int err = 0;
    err = gpio_request(CTP_RST_PORT, CTP_NAME);
    if ( err ) {
        err = -EINVAL;
        return err;
    }

    err = gpio_direction_output(CTP_RST_PORT, 1);
    if ( err ) {
        err = -EINVAL;
        return err;
    }

    tp_regulator = regulator_init(CTP_POWER_ID, 
        CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
    if ( !tp_regulator ) {
        printk(KERN_ERR "icn85xx tp init power failed");
        err = -EINVAL;
        return err;
    }
    
    msleep(100);

    gpio_set_value_cansleep(CTP_RST_PORT, 0);
    msleep(30);
    gpio_set_value_cansleep(CTP_RST_PORT, 1);
    msleep(50);
        
    return err;
}



static int __init icn85xx_ts_init(void)
{ 
    int ret = -1;

    printk("==icn85xx_init %d    20150319!!!!!!!!\n",__LINE__);
    
   
//    dprintk(DEBUG_INIT,"***************************init begin*************************************\n");
    if (input_fetch_sysconfig_para(&(config_info.input_type))) {
      printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
	    return 0;
		} else {
		  ret = input_init_platform_resource(&(config_info.input_type));
			if (0 != ret) {
			        printk("%s:init_platform_resource err. \n", __func__);
			}
		}
		if(config_info.ctp_used == 0){
		         printk("*** ctp_used set to 0 !\n");
		         printk("*** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
		         return 0;
		}

    if(!ctp_get_system_config()){
             printk("%s:read config fail!\n",__func__);
             return ret;
    }
     
//    icn85xx_hw_init();  
//    i2c_new_device(adap, &tp_info); 
		input_set_power_enable(&(config_info.input_type), 1);
		msleep(10);
		sunxi_gpio_to_name(CTP_IRQ_NUMBER,irq_pin_name);

    ret = i2c_add_driver(&icn85xx_ts_driver);
    printk("icn85xx init ok,by zyt\n");
    return ret;
}

static void __exit icn85xx_ts_exit(void)
{
    icn85xx_trace("==icn85xx_ts_exit==\n");
    i2c_del_driver(&icn85xx_ts_driver);
}

module_init(icn85xx_ts_init);
module_exit(icn85xx_ts_exit);

MODULE_AUTHOR("<zmtian@chiponeic.com>");
MODULE_DESCRIPTION("Chipone icn85xx TouchScreen driver");
MODULE_LICENSE("GPL");
