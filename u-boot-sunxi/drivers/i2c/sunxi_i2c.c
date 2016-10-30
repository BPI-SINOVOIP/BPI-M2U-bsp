/*
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* This code should work for both the S3C2400 and the S3C2410
 * as they seem to have the same I2C controller inside.
 * The different address mapping is handled by the s3c24xx.h files below.
 */

#include <common.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/twi.h>
#include <sys_config.h>
#include <asm/arch/timer.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <sys_config.h>
//#include <sys_config_old.h>
#include <fdt_support.h>

#define	I2C_WRITE		0
#define I2C_READ		1

#define I2C_OK			0
#define I2C_NOK			1
#define I2C_NACK		2
#define I2C_NOK_LA		3	/* Lost arbitration */
#define I2C_NOK_TOUT	4	/* time out */

#define I2C_START_TRANSMIT     0x08
#define I2C_RESTART_TRANSMIT   0x10
#define I2C_ADDRWRITE_ACK	   0x18
#define I2C_ADDRREAD_ACK	   0x40
#define I2C_DATAWRITE_ACK      0x28
#define I2C_READY			   0xf8
#define I2C_DATAREAD_NACK	   0x58
#define I2C_DATAREAD_ACK	   0x50
/* status or interrupt source */
/*------------------------------------------------------------------------------
* Code   Status
* 00h    Bus error
* 08h    START condition transmitted
* 10h    Repeated START condition transmitted
* 18h    Address + Write bit transmitted, ACK received
* 20h    Address + Write bit transmitted, ACK not received
* 28h    Data byte transmitted in master mode, ACK received
* 30h    Data byte transmitted in master mode, ACK not received
* 38h    Arbitration lost in address or data byte
* 40h    Address + Read bit transmitted, ACK received
* 48h    Address + Read bit transmitted, ACK not received
* 50h    Data byte received in master mode, ACK transmitted
* 58h    Data byte received in master mode, not ACK transmitted
* 60h    Slave address + Write bit received, ACK transmitted
* 68h    Arbitration lost in address as master, slave address + Write bit received, ACK transmitted
* 70h    General Call address received, ACK transmitted
* 78h    Arbitration lost in address as master, General Call address received, ACK transmitted
* 80h    Data byte received after slave address received, ACK transmitted
* 88h    Data byte received after slave address received, not ACK transmitted
* 90h    Data byte received after General Call received, ACK transmitted
* 98h    Data byte received after General Call received, not ACK transmitted
* A0h    STOP or repeated START condition received in slave mode
* A8h    Slave address + Read bit received, ACK transmitted
* B0h    Arbitration lost in address as master, slave address + Read bit received, ACK transmitted
* B8h    Data byte transmitted in slave mode, ACK received
* C0h    Data byte transmitted in slave mode, ACK not received
* C8h    Last byte transmitted in slave mode, ACK received
* D0h    Second Address byte + Write bit transmitted, ACK received
* D8h    Second Address byte + Write bit transmitted, ACK not received
* F8h    No relevant status information or no interrupt
*-----------------------------------------------------------------------------*/


static  struct sunxi_twi_reg *i2c  __attribute__((section(".data"))) = NULL;

/*
**********************************************************************************************************************
*                                               sw_iic_exit
*
* Description:
*
* Arguments  :
*
* Returns    :
*
* Notes      :    none
*
**********************************************************************************************************************
*/
static __s32 i2c_sendstart(void)
{
    __s32  time = 0xfffff;
    __u32  tmp_val;

    i2c->eft  = 0;
    i2c->srst = 1;
    i2c->ctl |= 0x20;

    while((time--)&&(!(i2c->ctl & 0x08)));
	if(time <= 0)
	{
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
    if(tmp_val != I2C_START_TRANSMIT)
    {
		return -I2C_START_TRANSMIT;
    }

    return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               TWIC_SendReStart
*
* Description:
*
* Arguments  :
*
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
static __s32 i2c_sendRestart(void)
{
    __s32  time = 0xffff;
    __u32  tmp_val;

#if defined(CONFIG_ARCH_SUN7I) | defined(CONFIG_ARCH_SUN5I)
	tmp_val = i2c->ctl & 0xC0;
#else
	tmp_val = i2c->ctl;
#endif
        tmp_val |= 0x20;
	i2c->ctl = tmp_val;

    while( (time--) && (!(i2c->ctl & 0x08)) );
	if(time <= 0)
	{
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
    if(tmp_val != I2C_RESTART_TRANSMIT)
    {
		return -I2C_RESTART_TRANSMIT;
    }

    return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               TWIC_SendSlaveAddr
*
* Description:
*
* Arguments  :
*
* Returns    :    EPDK_OK = successed;   EPDK_FAIL = failed
*
* Notes      :     none
*
**********************************************************************************************************************
*/
static __s32 i2c_sendslaveaddr(__u32 saddr,  __u32 rw)
{
    __s32  time = 0xffff;
    __u32  tmp_val;

	rw &= 1;
	i2c->data = ((saddr & 0xff) << 1)| rw;
#if defined(CONFIG_ARCH_SUN5I) || defined(CONFIG_ARCH_SUN7I)
        i2c->ctl &=  0xF7; //write 0 to clean int flag
#else
        i2c->ctl  |= (0x01<<3);//write 1 to clean int flag
#endif
	while(( time-- ) && (!( i2c->ctl & 0x08 )));
	if(time <= 0)
	{
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
	if(rw == I2C_WRITE)//+write
	{
		if(tmp_val != I2C_ADDRWRITE_ACK)
		{
			return -I2C_ADDRWRITE_ACK;
		}
	}

	else//+read
	{
		if(tmp_val != I2C_ADDRREAD_ACK)
		{
			return -I2C_ADDRREAD_ACK;
		}
	}

	return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               i2c_SendByteAddr
*
* Description:
*
* Arguments  :
*
* Returns    :    EPDK_OK = successed;   EPDK_FAIL = failed
*
* Notes      :     none
*
**********************************************************************************************************************
*/
static __s32 i2c_sendbyteaddr(__u32 byteaddr)
{
    __s32  time = 0xffff;
    __u32  tmp_val;

	i2c->data = byteaddr & 0xff;
#if defined(CONFIG_ARCH_SUN5I)||defined(CONFIG_ARCH_SUN7I)
	i2c->ctl &= 0xF7;   //write 0 to clean int flag
#else
        i2c->ctl |= (0x01<<3);//write 1 to clean int flag
#endif
        while( (time--) && (!(i2c->ctl & 0x08)) );
	if(time <= 0)
	{
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
	if(tmp_val != I2C_DATAWRITE_ACK)
	{
		return -I2C_DATAWRITE_ACK;
	}

	return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               TWIC_GetData
*
* Description:
*
* Arguments  :
*
* Returns    :    EPDK_OK = successed;   EPDK_FAIL = failed
*
* Notes      :     none
*
**********************************************************************************************************************
*/
static __s32 i2c_getdata(__u8 *data_addr, __u32 data_count)
{
    __s32  time = 0xffff;
    __u32  tmp_val;
    __u32  i;
    if(data_count == 1)
    {
#if defined(CONFIG_ARCH_SUN5I)||defined(CONFIG_ARCH_SUN7I)
	i2c->ctl &= 0xF7;
#else
        i2c->ctl |= (0x01<<3);
#endif
        while( (time--) && (!(i2c->ctl & 0x08)) );
	if(time <= 0)
	{
            return -I2C_NOK_TOUT;
	}
	for(time=0;time<100;time++);
	*data_addr = i2c->data;

	tmp_val = i2c->status;
	if(tmp_val != I2C_DATAREAD_NACK)
	{
            return -I2C_DATAREAD_NACK;
	}
	}
	else
	{
		for(i=0; i< data_count - 1; i++)
		{
			time = 0xffff;
			tmp_val = i2c->ctl | (0x01<<2);
#if defined(CONFIG_ARCH_SUN5I)||defined(CONFIG_ARCH_SUN7I)
                        tmp_val = i2c->ctl & (0xf7);
#else
                        tmp_val = i2c->ctl | (0x01<<3);
#endif
		        tmp_val |= 0x04;
                        i2c->ctl = tmp_val;
		 	//i2c->ctl |=(0x01<<3);

			while( (time--) && (!(i2c->ctl & 0x08)) );
			if(time <= 0)
			{
				return -I2C_NOK_TOUT;
			}
			for(time=0;time<100;time++);
			time = 0xffff;
			data_addr[i] = i2c->data;
		    while( (time--) && (i2c->status != I2C_DATAREAD_ACK) );
			if(time <= 0)
			{
                            return -I2C_NOK_TOUT;
			}
		}

        time = 0xffff;
		i2c->ctl &= 0xFb;
#if defined(CONFIG_ARCH_SUN5I)||defined(CONFIG_ARCH_SUN7I)
		i2c->ctl &= 0xf7;
#else
		i2c->ctl |= (0x01<<3);
#endif
		while( (time--) && (!(i2c->ctl & 0x08)) );
		if(time <= 0)
		{
                    return -I2C_NOK_TOUT;
		}
		for(time=0;time<100;time++);
		data_addr[data_count - 1] = i2c->data;
	    while( (time--) && (i2c->status != I2C_DATAREAD_NACK) );
		if(time <= 0)
		{
                    return -I2C_NOK_TOUT;
		}
	}

	return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               i2c_SendData
*
* Description:
*
* Arguments  :
*
* Returns    :    EPDK_OK = successed;   EPDK_FAIL = failed
*
* Notes      :     none
*
**********************************************************************************************************************
*/
static __s32 i2c_senddata(__u8  *data_addr, __u32 data_count)
{
    __s32  time = 0xffff;
    __u32  i;

	for(i=0; i< data_count; i++)
	{
		time = 0xffff;
	        i2c->data = data_addr[i];
#if defined(CONFIG_ARCH_SUN5I)|defined(CONFIG_ARCH_SUN7I)
		i2c->ctl &= 0xF7;
#else
                i2c->ctl |= (0x01<<3);
#endif
                while( (time--) && (!(i2c->ctl & 0x08)) );
		if(time <= 0)
		{
                    return -I2C_NOK_TOUT;
		}
		time = 0xffff;
		while( (time--) && (i2c->status != I2C_DATAWRITE_ACK) );
                if(time <= 0)
		{
                    return -I2C_NOK_TOUT;
		}
	}

	return I2C_OK;
}
/*
**********************************************************************************************************************
*                                               i2c_Stop
*
* Description:
*
* Arguments  :
*
* Returns    :    EPDK_OK = successed;   EPDK_FAIL = failed
*
* Notes      :     none
*
**********************************************************************************************************************
*/
static __s32 i2c_stop(void)
{
    __s32  time = 0xffff;
    __u32  tmp_val;

//	tmp_val = (i2c->ctl & 0xC0) | 0x10;
//	i2c->ctl = tmp_val;
	i2c->ctl |= (0x01 << 4);
#if defined(CONFIG_ARCH_SUN5I)|defined(CONFIG_ARCH_SUN7I)
        i2c->ctl &= 0xf7;
#else
        i2c->ctl |= (0x01 << 3);
#endif
        while( (time--) && (i2c->ctl & 0x10) );
	if(time <= 0)
	{
		return -I2C_NOK_TOUT;
	}
	time = 0xffff;
	while( (time--) && (i2c->status != I2C_READY) );
	tmp_val = i2c->status;
	if(tmp_val != I2C_READY)
	{
		return -I2C_NOK_TOUT;
	}

	return I2C_OK;
}

void set_cpux_i2c_clock(int bus_num)
{
    int reg_value = 0;

#if !defined(CONFIG_ARCH_SUN5I)||!defined(CONFIG_ARCH_SUN7I)
	/* reset i2c clock    */
	/* reset apb2 twi0*/
	reg_value = readl(CCMU_BUS_SOFT_RST_REG4);
	reg_value |= 0x01 << bus_num;
	writel(reg_value, CCMU_BUS_SOFT_RST_REG4);
	__msdelay(1);
#endif
	reg_value = readl(CCMU_BUS_CLK_GATING_REG3);
	reg_value &= ~(1<<bus_num);
	writel(reg_value,CCMU_BUS_CLK_GATING_REG3);
	__msdelay(1);
	reg_value |= (1 << bus_num);
	writel(reg_value,CCMU_BUS_CLK_GATING_REG3);
}


/*
**********************************************************************************************************************
*                                               i2c_init
*
* Description:
*
* Arguments  :
*
* Returns    :    none
*
* Notes      :    none
*
**********************************************************************************************************************
*/

void i2c_set_clock(int speed, int slaveaddr)
{
	int i, clk_n, clk_m;
	    /* reset i2c control  */
	i = 0xffff;
	i2c->srst = 1;
	while((i2c->srst) && (i))
	{
	    i --;
	}
	if((i2c->lcr & 0x30) != 0x30 )
	{
	    /* toggle I2C SCL and SDA until bus idle */
	    i2c->lcr = 0x05;
	    __usdelay(500);
	    i = 10;
	    while ((i > 0) && ((i2c->lcr & 0x02) != 2))
	    {
		    /*control scl and sda output high level*/
		    i2c->lcr |= 0x08;
		    i2c->lcr |= 0x02;
		    __usdelay(1000);
		    /*control scl and sda output low level*/
		    i2c->lcr &= ~0x08;
		    i2c->lcr &= ~0x02;
		    __usdelay(1000);
		    i--;
	    }
	    i2c->lcr = 0x0;
	    __usdelay(500);
	}

	if(speed < 100)
	{
	    speed = 100;
	}
	else if(speed > 400)
	{
	    speed = 400;
	}
	clk_n = 1;
	clk_m = (24000/10)/((2^clk_n) * speed) - 1;

	i2c->clk = (clk_m<<3) | clk_n;
	i2c->ctl = 0x40;
	i2c->eft = 0;

}

#ifdef CONFIG_CPUX_I2C
#if 0

void i2c_init(int speed, int slaveaddr)
{
	int bus_num;
	int ret;

	if(i2c != NULL)
	{
		printf("error:i2c has been initialized\n");
		return ;
	}
	ret = script_parser_fetch("twi_para", "twi_port", &bus_num, 1);
	if(ret)
	{
		printf("error:fail to get the i2c port\n");
		return ;
	}
	ret = gpio_request_simple("twi_para", NULL);
	if(ret)
	{
		printf("error:fail to set the i2c gpio\n");
		return ;
	}

	set_cpux_i2c_clock(bus_num);

	i2c = (struct sunxi_twi_reg *)(SUNXI_TWI0_BASE + (bus_num * TWI_CONTROL_OFFSET));

	i2c_set_clock(speed,slaveaddr);
	printf("i2c_init ok\n");
	return ;
}
#else
 void i2c_init(int speed, int slaveaddr)
{
	int bus_num;
	int i2c_nodeoffset;

	if(i2c != NULL)
	{
		printf("error:i2c has been initialized\n");
		return ;
	}
	i2c = (struct sunxi_twi_reg *)SUNXI_TWI0_BASE;

	i2c_nodeoffset = fdt_path_offset(working_fdt,"/soc/twi_para");
	if(i2c_nodeoffset < 0)
	{
		printf("%s:%d: get nodeerror\n",__func__, __LINE__);
		return ;
	}

	if(fdt_getprop_u32(working_fdt,i2c_nodeoffset,"twi_port", (uint32_t*)&bus_num)<0)
	{
		printf("can not get i2c bus num \n");
	}
	if(bus_num > SUNXI_I2C_CONTROLLER)
	{
		printf("can not support i2c bus num %d \n",bus_num);
		return ;
	}
	set_cpux_i2c_clock(bus_num);
	if(0 != fdt_set_all_pin_by_offset(i2c_nodeoffset,"pinctrl-0"))
	{
		printf("set pin for i2c bus num %d error\n",bus_num);
		return ;
	}
	i2c = (struct sunxi_twi_reg *)(SUNXI_TWI0_BASE + (bus_num * TWI_CONTROL_OFFSET));

	i2c_set_clock(speed,slaveaddr);
	printf("i2c_init ok\n");
	return ;
}

#endif
void i2c_exit(int bus_num)
{
	int reg_value = 0;
	reg_value = readl(CCMU_BUS_CLK_GATING_REG3);
	reg_value &= ~(1<<bus_num);
	writel(reg_value,CCMU_BUS_CLK_GATING_REG3);
	i2c = NULL;
	return ;
}
#endif

#ifdef CONFIG_CPUS_I2C
int set_cpus_i2c_clock(void)
{
	int reg_value = 0;

	//opne CPUS PIO
	volatile unsigned int reg_val;
	// R_GPIO reset deassert
	reg_val = readl(SUNXI_RPRCM_BASE+0xb0);
	reg_val |= 1;
	writel(reg_val, SUNXI_RPRCM_BASE+0xb0);
	// R_GPIO GATING open
	reg_val = readl(SUNXI_RPRCM_BASE+0x28);
	reg_val |= 1;
	writel(reg_val, SUNXI_RPRCM_BASE+0x28);

	//set stwi pin
	if(fdt_set_all_pin("/soc/s_twi","pinctrl-0"))
	{
		return -1;
	}
	//deassert twi reset
	reg_value = readl(SUNXI_RPRCM_BASE + 0xb0);
	reg_value &= ~(0x01 << 6);
	writel(reg_value,SUNXI_RPRCM_BASE + 0xb0);
	reg_value = readl(SUNXI_RPRCM_BASE + 0xb0);
	reg_value |= 0x01 << 6;
	writel(reg_value,SUNXI_RPRCM_BASE + 0xb0);
	__msdelay(1);
	//open twi gating
	reg_value = readl(SUNXI_RPRCM_BASE + 0x28);
	reg_value |= 0x01 << 6;
	writel(reg_value,SUNXI_RPRCM_BASE + 0x28);
	__msdelay(1);
	return 0;

}


void i2c_init_cpus(int speed, int slaveaddr)
{
	if(i2c != NULL)
	{
		printf("error:i2c has been initialized\n");
	}
	i2c = (struct sunxi_twi_reg *)SUNXI_RTWI_BASE;
	if(set_cpus_i2c_clock())
	{
		printf("error:%s\n", __func__);
	}
	i2c_set_clock(speed,slaveaddr);
	return ;
}


/*
**********************************************************************************************************************
*                                               i2c_init
*
* Description:
*
* Arguments  :
*
* Returns    :    none
*
* Notes      :    none
*
**********************************************************************************************************************
*/
void i2c_exit_cpus(void)
{
	int reg_value = 0;
	reg_value = *((unsigned int *)(SUNXI_RPRCM_BASE + 0xb0));
	reg_value &= ~(0x01 << 6);
	*((unsigned int *)(SUNXI_RPRCM_BASE + 0xb0)) = reg_value;
	__msdelay(1);
	reg_value = *((unsigned int *)(SUNXI_RPRCM_BASE + 0x28));
	reg_value &= ~(0x01 << 6);
	*((unsigned int *)(SUNXI_RPRCM_BASE + 0x28)) = reg_value;
	__msdelay(1);
	i2c = NULL;
}

#endif


/*
****************************************************************************************************
*
*                                       i2c_read
*
*  Description:
*
*
*  Parameters:
*
*  Return value:
*
*  Read/Write interface:
*    chip:    I2C slave chip address, range 0..127
*    addr:    Memory (register) address within the chip
*    alen:    Number of bytes to use for addr (
*             0, 1: addr len = 8bit
*			  2: addr len = 16bit
*			  3, 4: addr len = 32bit
*
*    buffer:  Where to read/write the data
*    len:     How many bytes to read/write
*
*    Returns: 0 on success, not 0 on failure
*
****************************************************************************************************
*/
int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
    int   i, ret, ret0, addrlen;
    char  *slave_reg;

	ret0 = -1;
    ret = i2c_sendstart();
    if(ret)
	{
		goto i2c_read_err_occur;
	}

    ret = i2c_sendslaveaddr(chip, I2C_WRITE);
    if(ret)
    {
	    goto i2c_read_err_occur;
	}
    //send byte address
    if(alen >= 3)
    {
	addrlen = 2;
    }
    else if(alen <= 1)
    {
	addrlen = 0;
    }
    else
    {
	addrlen = 1;
    }
    slave_reg = (char *)&addr;
    for (i = addrlen; i>=0; i--)
    {
    	ret = i2c_sendbyteaddr(slave_reg[i] & 0xff);
    	if(ret)
    	{
    		goto i2c_read_err_occur;
    	}
    }
    ret = i2c_sendRestart();
    if(ret)
	{
		goto i2c_read_err_occur;
	}
    ret = i2c_sendslaveaddr(chip, I2C_READ);
    if(ret)
    {
        goto i2c_read_err_occur;
	}
    //get data
	ret = i2c_getdata(buffer, len);
	if(ret)
	{
		goto i2c_read_err_occur;
	}
    ret0 = 0;

i2c_read_err_occur:
	i2c_stop();

	return ret0;
}
/*
****************************************************************************************************
*
*             TWIC_Write
*
*  Description:
*       DRV_MOpen
*
*  Parameters:
*
*  Return value:
*       EPDK_OK
*       EPDK_FAIL
****************************************************************************************************
*/
int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
    int   i, ret, ret0, addrlen;
    char  *slave_reg;

    ret0 = -1;
    ret = i2c_sendstart();
    if(ret)
	{
		goto i2c_write_err_occur;
	}

    ret = i2c_sendslaveaddr(chip, I2C_WRITE);
    if(ret)
    {
	    goto i2c_write_err_occur;
	}
    //send byte address
    if(alen >= 3)
    {
        addrlen = 2;
    }
    else if(alen <= 1)
    {
        addrlen = 0;
    }
    else
    {
	addrlen = 1;
    }
    slave_reg = (char *)&addr;
    for (i = addrlen; i>=0; i--)
    {
    	ret = i2c_sendbyteaddr(slave_reg[i] & 0xff);
    	if(ret)
    	{
    		goto i2c_write_err_occur;
    	}
    }

	ret = i2c_senddata(buffer, len);
	if(ret)
	{
		goto i2c_write_err_occur;
	}
    ret0 = 0;

i2c_write_err_occur:
	i2c_stop();

	return ret0;
}



