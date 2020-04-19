/*
**********************************************************************************************************************
*
*                                  the Embedded Secure Bootloader System
*
*
*                              Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date    :
*
* Descript:
**********************************************************************************************************************
*/
#include <common.h>
#include <asm/io.h>
#include <spare_head.h>
#include <asm/arch/platform.h>
#include <asm/arch/gpio.h>
#include <sys_config_old.h>

__s32 boot_set_gpio(void  *user_gpio_list, __u32 group_count_max, __s32 set_gpio)
{
    normal_gpio_set_t   *tmp_user_gpio_data, *gpio_list;
    __u32               first_port;         //save GPIO number
    __u32               tmp_group_func_data;
    __u32               tmp_group_pull_data;
    __u32               tmp_group_dlevel_data;
    __u32               tmp_group_data_data;
    __u32               data_change = 0;
    //__u32              *tmp_group_port_addr;
    volatile __u32      *tmp_group_func_addr,   *tmp_group_pull_addr;
    volatile __u32      *tmp_group_dlevel_addr, *tmp_group_data_addr;
    __u32               port, port_num, port_num_func, port_num_pull;
    __u32               pre_port, pre_port_num_func;
    __u32               pre_port_num_pull;
    __s32               i, tmp_val;


    gpio_list = (normal_gpio_set_t *)user_gpio_list;

    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;
        port_num = tmp_user_gpio_data->port_num;
        if(!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //update register addr
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //update pull register
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//update level register
        tmp_group_data_addr    = PIO_REG_DATA(port);                 //update data register

        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);

        pre_port          = port;
        pre_port_num_func = port_num_func;
        pre_port_num_pull = port_num_pull;
        //update functional register
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        tmp_group_func_data &= ~(0x07 << tmp_val);
        if(set_gpio)
        {
            tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
        }
        //According to pull value to decide whether update pull register
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
            tmp_group_pull_data &= ~(                           0x03  << tmp_val);
            tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //according to driver level value to decide whether update driver level register
        if(tmp_user_gpio_data->drv_level >= 0)
        {
            tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
            tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        //according to user input value, decide whether update data register
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
                tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }

        break;
    }
    //check the data valid or not
    if(first_port >= group_count_max)
    {
        return -1;
    }
    //save user data
    for(i = first_port + 1; i < group_count_max; i++)
    {
        tmp_user_gpio_data = gpio_list + i;
        port     = tmp_user_gpio_data->port;
        port_num = tmp_user_gpio_data->port_num;
        if(!port)
        {
            break;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);
            if(data_change)
            {
                data_change = 0;
                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //write back data register
            }

            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);
            tmp_group_data_addr    = PIO_REG_DATA(port);

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
            tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
        }
        else if(pre_port_num_func != port_num_func)
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }
        //save hardware register
        pre_port_num_pull = port_num_pull;
        pre_port_num_func = port_num_func;
        pre_port          = port;

        //update function register
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        if(tmp_user_gpio_data->mul_sel >= 0)
        {
            tmp_group_func_data &= ~(0x07  << tmp_val);
            if(set_gpio)
            {
                tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
            }
        }
        //according to pull value decide whether update pull register
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
            tmp_group_pull_data &= ~(                           0x03  << tmp_val);
            tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //according to driver level value decide whether update driver level register
        if(tmp_user_gpio_data->drv_level >= 0)
        {
            tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
            tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
                tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }

    if(tmp_group_func_addr)
    {
        GPIO_REG_WRITE(tmp_group_func_addr,   tmp_group_func_data);
        GPIO_REG_WRITE(tmp_group_pull_addr,   tmp_group_pull_data);
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);
        if(data_change)
        {
            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data);
        }
    }

    return 0;
}
