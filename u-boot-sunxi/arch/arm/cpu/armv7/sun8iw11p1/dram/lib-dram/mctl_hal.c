//*****************************************************************************
//	Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
//
//	File: 				mctl_hal.c
//
//	Description:  This file implements basic functions for AW1699 DRAM controller
//
//	History:
//				2015/12/04		ZHUWEI			0.10	Initial version base on B100
//*****************************************************************************
#include "mctl_reg.h"
#include "mctl_hal.h"

#ifdef USE_PMU
extern int set_ddr_voltage(int set_vol);
#endif
#ifndef FPGA_VERIFY
/********************************************************************************
 *IC boot code
 ********************************************************************************/
void dram_udelay (unsigned int n)
{
#ifdef SYSTEM_VERIFY
	__usdelay(n);
#else
	/*FPGA验证、IC验证及仿真用此delay函数*/
	while(--n);
#endif
}
void paraconfig(unsigned int *para, unsigned int mask, unsigned int value)
{
	*para &= ~(mask);
	*para |= value;
}

/*****************************************************************************
作用：DRAM bit delay补偿函数
参数：__dram_para_t *para
返回值：void
*****************************************************************************/
void bit_delay_compensation(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	unsigned int i = 0;
	//dq0~dq7 read and write delay
	for(i=0;i<9;i++)
	{
		reg_val = mctl_read_w(DATX0IOCR(i));
		reg_val |= (((((para->dram_tpr11)&(0xf<<0))>>0 )<<9)|((((para->dram_tpr12)&(0xf<<0))>>0 )<<1));
		mctl_write_w(reg_val,DATX0IOCR(i));
	}
	//dq8~dq15 read and write delay
	for(i=0;i<9;i++)
	{
		reg_val = mctl_read_w(DATX1IOCR(i));
		reg_val |= (((((para->dram_tpr11)&(0xf<<4))>>4 )<<9)|((((para->dram_tpr12)&(0xf<<4))>>4 )<<1));
		mctl_write_w(reg_val,DATX1IOCR(i));
	}
	//dq16~dq23 read and write delay
	for(i=0;i<9;i++)
	{
		reg_val = mctl_read_w(DATX2IOCR(i));
		reg_val |= (((((para->dram_tpr11)&(0xf<<8))>>8 )<<9)|((((para->dram_tpr12)&(0xf<<8))>>8 )<<1));
		mctl_write_w(reg_val,DATX2IOCR(i));
	}
	//dq24~dq31 read and write delay
	for(i=0;i<9;i++)
	{
		reg_val = mctl_read_w(DATX3IOCR(i));
		reg_val |= (((((para->dram_tpr11)&(0xf<<12))>>12 )<<9)|((((para->dram_tpr12)&(0xf<<12))>>12 )<<1));
		mctl_write_w(reg_val,DATX3IOCR(i));
	}
	reg_val=mctl_read_w(PGCR0);
	reg_val&=(~(0x1<<26));
	mctl_write_w(reg_val,PGCR0);
	//dqs0 read and write delay
	reg_val = mctl_read_w(DATX0IOCR(9));//dqs0_p
	reg_val |= (((((para->dram_tpr11)&(0xf<<16))>>16 )<<9)|((((para->dram_tpr12)&(0xf<<16))>>16 )<<1));
	mctl_write_w(reg_val,DATX0IOCR(9));
	reg_val = mctl_read_w(DATX0IOCR(10));//dqs0_n
	reg_val |= (((((para->dram_tpr11)&(0xf<<16))>>16 )<<9)|((((para->dram_tpr12)&(0xf<<16))>>16 )<<1));
	mctl_write_w(reg_val,DATX0IOCR(10));
	//dqs1 read and write delay
	reg_val = mctl_read_w(DATX1IOCR(9));//dqs2_p
	reg_val |= (((((para->dram_tpr11)&(0xf<<20))>>20 )<<9)|((((para->dram_tpr12)&(0xf<<20))>>20 )<<1));
	mctl_write_w(reg_val,DATX1IOCR(9));
	reg_val = mctl_read_w(DATX1IOCR(10));//dqs2_n
	reg_val |= (((((para->dram_tpr11)&(0xf<<20))>>20 )<<9)|((((para->dram_tpr12)&(0xf<<20))>>20 )<<1));
	mctl_write_w(reg_val,DATX1IOCR(10));
	//dqs2 read and write delay
	reg_val = mctl_read_w(DATX2IOCR(9));//dqs2_p
	reg_val |= (((((para->dram_tpr11)&(0xf<<24))>>24 )<<9)|((((para->dram_tpr12)&(0xf<<24))>>24 )<<1));
	mctl_write_w(reg_val,DATX2IOCR(9));
	reg_val = mctl_read_w(DATX2IOCR(10));//dqs2_n
	reg_val |= (((((para->dram_tpr11)&(0xf<<24))>>24 )<<9)|((((para->dram_tpr12)&(0xf<<24))>>24 )<<1));
	mctl_write_w(reg_val,DATX2IOCR(10));
	//dqs3 read and write delay
	reg_val = mctl_read_w(DATX3IOCR(9));//dqs3_p
	reg_val |= (((((para->dram_tpr11)&(0xfU<<28))>>28 )<<9)|((((para->dram_tpr12)&(0xfU<<28))>>28 )<<1));
	mctl_write_w(reg_val,DATX3IOCR(9));
	reg_val = mctl_read_w(DATX3IOCR(10));//dqs3_n
	reg_val |= (((((para->dram_tpr11)&(0xfU<<28))>>28 )<<9)|((((para->dram_tpr12)&(0xfU<<28))>>28 )<<1));
	mctl_write_w(reg_val,DATX3IOCR(10));

	//DQS0/DM0/DQ0~7 Output Enable Bit Delay
	reg_val = mctl_read_w(DXnSDLR6(0));//
	reg_val |= (((((para->dram_tpr11)&(0xfU<<16))>>16)<<25));
	mctl_write_w(reg_val,DXnSDLR6(0));

	//DQS1/DM1/DQ8~15 Output Enable Bit Delay
	reg_val = mctl_read_w(DXnSDLR6(1));//
	reg_val |= (((((para->dram_tpr11)&(0xfU<<20))>>20)<<25));
	mctl_write_w(reg_val,DXnSDLR6(1));

	//DQS2/DM2/DQ15~23 Output Enable Bit Delay
	reg_val = mctl_read_w(DXnSDLR6(2));//
	reg_val |= (((((para->dram_tpr11)&(0xfU<<24))>>24)<<25));
	mctl_write_w(reg_val,DXnSDLR6(2));

	//DQS3/DM3/DQ24~31 Output Enable Bit Delay
	reg_val = mctl_read_w(DXnSDLR6(3));//
	reg_val |= (((((para->dram_tpr11)&(0xfU<<28))>>28)<<25));
	mctl_write_w(reg_val,DXnSDLR6(3));

	reg_val=mctl_read_w(PGCR0);
	reg_val|=(0x1<<26);
	mctl_write_w(reg_val,PGCR0);
	dram_udelay(1);

	//CA0~CA9 delay
	for(i=0;i<10;i++)
	{
			reg_val = mctl_read_w(ACIOCR1(12+i));
			reg_val |= ((((para->dram_tpr10)&(0xf<<4))>>4)<<8);
			mctl_write_w(reg_val,ACIOCR1(12+i));
	}
	//CK CS0 CS1 delay
	reg_val = mctl_read_w(ACIOCR1(2));//CK
	reg_val |= ((para->dram_tpr10&0xf)<<8);
	mctl_write_w(reg_val,ACIOCR1(2));
	reg_val = mctl_read_w(ACIOCR1(3));//CS0
	reg_val |= (((para->dram_tpr10&(0xf<<8))>>8)<<8);
	mctl_write_w(reg_val,ACIOCR1(3));
	reg_val = mctl_read_w(ACIOCR1(28));//CS1
	reg_val |= (((para->dram_tpr10&(0xf<<12))>>12)<<8);
	mctl_write_w(reg_val,ACIOCR1(28));

}
/*****************************************************************************
作用：设置DRAM_VCC电压
参数：__dram_para_t *para
返回值：0-表示调压失败  ， other-调压成功
*****************************************************************************/
unsigned int dram_vol_set( __dram_para_t *para)
{
#ifdef USE_PMU
	unsigned int vol_val = 0;
	int ret_val = 0;

	switch(para->dram_type){
		case 2:
			vol_val = ((para->dram_tpr3>>24)&0xff)*10;
			break;
		case 3: 
			vol_val = ((para->dram_tpr3>>0)&0xff)*10;
			break;
		case 6: 
			vol_val = ((para->dram_tpr3>>16)&0xff)*10;
			break;
		case 7: 
			vol_val = ((para->dram_tpr3>>8)&0xff)*10;
			break;
		default:
			return 0;
	}
	ret_val = set_ddr_voltage(vol_val);	

	if(ret_val){
		dram_dbg_error("[ERROR DEBUG]: POWER SETTING ERROR!\n");
		return 0;
	}else{
		dram_dbg_4("DRAM_VCC set to %d mv\n",vol_val);
		return vol_val;
	}
#else
	return 1;
#endif
}

/*****************************************************************************
作用：设置master优先级及带宽限制
参数：void
返回值：void
*****************************************************************************/
void set_master_priority(void)
{
	//enable bandwidth limit windows and set windows size 1us
	mctl_write_w(0x18F,MC_TMR);
	mctl_write_w(0x00010000,MC_BWCR);
	//set cpu high priority
//	mctl_write_w(0x1,MC_MAPR);
	//set cpu
	mctl_write_w(0x00a0000d,MC_MnCR0(0));
	mctl_write_w(0x00500064,MC_MnCR1(0));
	//set gpu
	mctl_write_w(0x06000009,MC_MnCR0(1));
	mctl_write_w(0x01000578,MC_MnCR1(1));
	//set MAHB
	mctl_write_w(0x0200000d,MC_MnCR0(2));
	mctl_write_w(0x00600100,MC_MnCR1(2));
	//set DMA
	mctl_write_w(0x01000009,MC_MnCR0(3));
	mctl_write_w(0x00500064,MC_MnCR1(3));
	//set VE
	mctl_write_w(0x07000009,MC_MnCR0(4));
	mctl_write_w(0x01000640,MC_MnCR1(4));
	//set CSI
	mctl_write_w(0x01000009,MC_MnCR0(5));
	mctl_write_w(0x00000080,MC_MnCR1(5));
	//set NAND
	mctl_write_w(0x01000009,MC_MnCR0(6));
	mctl_write_w(0x00400080,MC_MnCR1(6));
	//set ss
	mctl_write_w(0x0100000d,MC_MnCR0(7));
	mctl_write_w(0x00400080,MC_MnCR1(7));
	//set TS
	mctl_write_w(0x0100000d,MC_MnCR0(8));
	mctl_write_w(0x00400080,MC_MnCR1(8));
	//set De-Interlace
	mctl_write_w(0x04000009,MC_MnCR0(9));
	mctl_write_w(0x00400100,MC_MnCR1(9));
	//set DE
	mctl_write_w(0x20000209,MC_MnCR0(10));
	mctl_write_w(0x08001800,MC_MnCR1(10));
	//set ROT
	mctl_write_w(0x05000009,MC_MnCR0(11));
	mctl_write_w(0x00400090,MC_MnCR1(11));
	// set ROT BW limit
	dram_dbg_8("DRAM master priority setting ok.\n");
}
/*****************************************************************************
作用：DRAM timing配置函数
参数：__dram_para_t *para
返回值：void
*****************************************************************************/
void auto_set_timing_para(__dram_para_t *para)
{
	unsigned int  ctrl_freq;//half speed mode :ctrl_freq=1/2 ddr_fre
	unsigned int  type;
	unsigned int  reg_val        =0;
	unsigned int  tdinit0       = 0;
	unsigned int  tdinit1       = 0;
	unsigned int  tdinit2       = 0;
	unsigned int  tdinit3       = 0;
	unsigned int  mr0           = 0;
	unsigned int  mr1           = 0;
	unsigned int  mr2           = 0;
	unsigned int  mr3           = 0;
	unsigned char t_rdata_en    = 1;    //ptimg0
	unsigned char wr_latency    = 1;	//ptimg0
	unsigned char tcl 			= 3;	//6
	unsigned char tcwl			= 3;	//6
	unsigned char tmrw			= 0;	//0
	unsigned char tmrd			= 2;	//4;
	unsigned char tmod			= 6;	//12;
	unsigned char tccd			= 2;	//4;
	unsigned char tcke			= 2;	//3;
	unsigned char trrd			= 3;	//6;
	unsigned char trcd			= 6;	//11;
	unsigned char trc			= 20;	//39;
	unsigned char tfaw			= 16;	//32;
	unsigned char tras			= 14;	//28;
	unsigned char trp			= 6;	//11;
	unsigned char twtr			= 3;	//6;
	unsigned char twr			= 8;	//15；
	unsigned char trtp			= 3;	//6;
	unsigned char txp			= 10;	//20;
	unsigned short trefi		= 98;	//195;
	unsigned short trfc		    = 128;
	unsigned char twtp			= 12;	//24;	//write to pre_charge
	unsigned char trasmax		= 27;	//54;	//54*1024ck
	unsigned char twr2rd		= 8;	//16;
	unsigned char trd2wr		= 4;	//7;
	unsigned char tckesr		= 3;	//5;
	unsigned char tcksrx		= 4;	//8;
	unsigned char tcksre		= 4;	//8;

	ctrl_freq = para->dram_clk/2;	//Controller work in half rate mode
	type      = para->dram_type;
	//add the time user define
	if(para->dram_tpr13&0x2)
	{
		dram_dbg_4("User define timing parameter!\n");
		//dram_tpr0
		tccd = ( (para->dram_tpr0 >> 21) & 0x7  );//[23:21]
		tfaw = ( (para->dram_tpr0 >> 15) & 0x3f );//[20:15]
		trrd = ( (para->dram_tpr0 >> 11) & 0xf  );//[14:11]
		trcd = ( (para->dram_tpr0 >>  6) & 0x1f );//[10:6 ]
		trc  = ( (para->dram_tpr0 >>  0) & 0x3f );//[ 5:0 ]
		//dram_tpr1
		txp =  ( (para->dram_tpr1 >> 23) & 0x1f );//[27:23]
		twtr = ( (para->dram_tpr1 >> 20) & 0x7  );//[22:20]
		trtp = ( (para->dram_tpr1 >> 15) & 0x1f );//[19:15]
		twr =  ( (para->dram_tpr1 >> 11) & 0xf  );//[14:11]
		trp =  ( (para->dram_tpr1 >>  6) & 0x1f );//[10:6 ]
		tras = ( (para->dram_tpr1 >>  0) & 0x3f );//[ 5:0 ]
		//dram_tpr2
		trfc  = ( (para->dram_tpr2 >> 12)& 0x1ff);//[20:12]
		trefi = ( (para->dram_tpr2 >> 0) & 0xfff);//[11:0 ]
	}//add finish
	else
	{
		dram_dbg_4("Auto calculate timing parameter!\n");
		if(type==3)
		{
			//dram_tpr0
			tccd=2;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);
			trrd=(10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trrd<2) trrd=2;	//max(4ck,10ns)
			trcd= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			trc	= (53*ctrl_freq)/1000 + ( ( ((53*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr1
			txp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(txp<2) txp = 2;//max(3ck,7.5ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twtr<2) twtr=2;	//max(4ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trtp<2) trtp=2;	//max(4ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twr<2) twr=2;
			trp = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			tras= (38*ctrl_freq)/1000 + ( ( ((35*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr2
			trefi	= ( (7800*ctrl_freq)/1000 + ( ( ((7800*ctrl_freq)%1000) != 0) ? 1 :0) )/32;
			trfc = (350*ctrl_freq)/1000 + ( ( ((350*ctrl_freq)%1000) != 0) ? 1 :0);
		}else if(type==2)
		{
			tccd=2;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);
			trrd= (10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			trcd= (20*ctrl_freq)/1000 + ( ( ((20*ctrl_freq)%1000) != 0) ? 1 :0);
			trc	= (65*ctrl_freq)/1000 + ( ( ((65*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr1
			txp	= 2;		//2nclk;
			twtr = (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			trtp = (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			twr = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			trp = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			tras= (45*ctrl_freq)/1000 + ( ( ((45*ctrl_freq)%1000) != 0) ? 1 :0);
			//trc	= trp + tras;	//65ns
			//dram_tpr2
			trefi = ((7800*ctrl_freq)/1000 + ( ( ((7800*ctrl_freq)%1000) != 0) ? 1 :0) )/32;
			trfc = (328*ctrl_freq)/1000 + ( ( ((328*ctrl_freq)%1000) != 0) ? 1 :0);

		}	else if(type==6)
		{
			tccd=2;//BL = 8
			tfaw	= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);
			if(tfaw<4) tfaw	= 4;
			trrd	= (10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trrd<1) trrd	= 1;
			trcd	= (24*ctrl_freq)/1000 + ( ( ((24*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trcd<2) trcd	= 2;
			trc	= (70*ctrl_freq)/1000 + ( ( ((70*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr1
			txp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(txp<1) txp = 1;//max(2ck,10ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twtr<2) twtr=2;	//max(2ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trtp<2) trtp=2;	//max(2ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twr<2) twr=2;
			trp = (27*ctrl_freq)/1000 + ( ( ((27*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trp<2) trp=2;
			tras= (42*ctrl_freq)/1000 + ( ( ((42*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr2
			trefi	= ( (3900*ctrl_freq)/1000 + ( ( ((3900*ctrl_freq)%1000) != 0) ? 1 :0) )/32;
			trfc = (210*ctrl_freq)/1000 + ( ( ((210*ctrl_freq)%1000) != 0) ? 1 :0);
		}else if(type==7)
		{
			tccd=2;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);
			if(tfaw<4) tfaw	= 4;
			trrd=(10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trrd<1) trrd=1;	//max(4ck,10ns)
			trcd= (24*ctrl_freq)/1000 + ( ( ((24*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trcd<2) trcd	= 2;
			trc	= (70*ctrl_freq)/1000 + ( ( ((70*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr1
			txp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(txp<2) txp = 2;//max(3ck,7.5ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twtr<2) twtr=2;	//max(4ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trtp<2) trtp=2;	//max(4ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);
			if(twr<2) twr=2;
			trp = (27*ctrl_freq)/1000 + ( ( ((27*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trp<2) trp=2;
			tras= (42*ctrl_freq)/1000 + ( ( ((42*ctrl_freq)%1000) != 0) ? 1 :0);
			//dram_tpr2
			trefi	= ( (3900*ctrl_freq)/1000 + ( ( ((3900*ctrl_freq)%1000) != 0) ? 1 :0) )/32;
			trfc = (210*ctrl_freq)/1000 + ( ( ((210*ctrl_freq)%1000) != 0) ? 1 :0);
		}
		//assign the value back to the DRAM structure
		dram_dbg_timing("[TIMING DEBUG] DRAM TYPE : %d \n",para->dram_type);
		para->dram_tpr0 = (trc<<0) | (trcd<<6) | (trrd<<11) | (tfaw<<15) | (tccd<<21) ;
		dram_dbg_timing("para_dram_tpr0 = 0x%x\n",para->dram_tpr0);
		para->dram_tpr1 = (tras<<0) | (trp<<6) | (twr<<11) | (trtp<<15) | (twtr<<20)|(txp<<23);
		dram_dbg_timing("para_dram_tpr1 = 0x%x\n",para->dram_tpr1);
		para->dram_tpr2 = (trefi<<0) | (trfc<<12);
		dram_dbg_timing("para_dram_tpr2 = 0x%x\n",para->dram_tpr2);
	}
	switch(type)
	{
	case 2://DDR2
		//the time we no need to calculate
		tmrw=0x0;
		tmrd=0x2;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=tcke + 1;
		trasmax =ctrl_freq/15 ;

		tcl	= 3;	//CL   6
		tcwl = 3;	//CWL  6
		t_rdata_en  =1;
		wr_latency  =1;
		mr0 	= 0xa63;// WR=6,CL=6,BL=8
		mr1 =   para->dram_mr1;
		mr2     = 0x0; //ODT disable
		mr3     = 0;

		tdinit0	= (400*para->dram_clk) + 1;	//400us
		tdinit1	= (500*para->dram_clk)/1000 + 1;//500ns
		tdinit2	= (200*para->dram_clk) + 1;	//200us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp=tcwl+2+twr;//WL+BL/2+tWR
		twr2rd= tcwl+2+twtr;//WL+BL/2+tWTR
		trd2wr= tcl+2+1-tcwl;//RL+BL/2+2-WL
		break;
	case 3://DDR3
		//the time we no need to calculate
		tmrw=0x0;
		tmrd=0x4;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=4;
		trasmax =ctrl_freq/15 ;

		tcl		= 6;	//CL   12
		tcwl	= 4;	//CWL  8
		t_rdata_en  =4;
		wr_latency  =2;
		mr0 	= 0x1c70;//CL=11,WR=12
		mr1     =   para->dram_mr1;
		mr2     = 0x18; //CWL=8
		mr3     = 0;

		tdinit0	= (500*para->dram_clk) + 1;	//500us
		tdinit1	= (360*para->dram_clk)/1000 + 1;//360ns
		tdinit2	= (200*para->dram_clk) + 1;	//200us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp=tcwl+2+twr;//WL+BL/2+tWR
		twr2rd= tcwl+2+twtr;//WL+BL/2+tWTR
		trd2wr= tcl+2+1-tcwl;//RL+BL/2+2-WL
		break;
	case 6 ://LPDDR2
		tmrw=0x3;
		tmrd=0x5;
		tmod=0x5;
		tcke=2;
		tcksrx=5;
		tcksre=5;
		tckesr=5;
		trasmax =ctrl_freq/30 ;
		//according to frequency
		tcl		= 4;
		tcwl	= 2;
		t_rdata_en  =3;    //if tcl odd,(tcl-3)/2;  if tcl even ,((tcl+1)-3)/2
		wr_latency  =1;
		mr0 = 0;
		mr1 = 0xc3;//twr=8;bl=8
		mr2 = 0x6;//RL=8,CWL=4
		mr3 =   para->dram_mr3;
		//end
		tdinit0	= (200*para->dram_clk) + 1;	//200us
		tdinit1	= (100*para->dram_clk)/1000 + 1;	//100ns
		tdinit2	= (11*para->dram_clk) + 1;	//11us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp	= tcwl + 2 + twr + 1;	// CWL+BL/2+tWR
		trd2wr	= tcl + 2 + 5 - tcwl + 1;//5?
		twr2rd	= tcwl + 2 + 1 + twtr;//wl+BL/2+1+tWTR??
		break;
	case 7 ://LPDDR3
		tmrw=0x5;
		tmrd=0x5;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=5;
		trasmax =ctrl_freq/30 ;
		//according to clock
		tcl		= 6;
		tcwl	= 3;
		t_rdata_en  =5;    //if tcl odd,(tcl-3)/2;  if tcl even ,((tcl+1)-3)/2
		wr_latency  =2;
		mr0 = 0;
		mr1 = 0xc3;//twr=8;bl=8
		mr2 = 0xa;//RL=12,CWL=6
		mr3 =   para->dram_mr3;
		//end
		tdinit0	= (200*para->dram_clk) + 1;	//200us
		tdinit1	= (100*para->dram_clk)/1000 + 1;	//100ns
		tdinit2	= (11*para->dram_clk) + 1;	//11us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp	= tcwl + 4 + twr + 1;	// CWL+BL/2+tWR
		trd2wr	= tcl + 4 + 5 - tcwl + 1;	//13;
		twr2rd	= tcwl + 4 + 1 + twtr;
		break;
	default:
		break;
	}
	//set mode register
	if(!(para->dram_mr0>>16)&0x1)
		para->dram_mr0 = mr0 ;
	if(!(para->dram_mr1>>16)&0x1)
		para->dram_mr1 = mr1 ;
	if(!(para->dram_mr2>>16)&0x1)
		para->dram_mr2 = mr2 ;
	if(!(para->dram_mr3>>16)&0x1)
		para->dram_mr3 = mr3 ;

	mctl_write_w((para->dram_mr0)&0xffff,DRAM_MR0);
	mctl_write_w((para->dram_mr1)&0xffff,DRAM_MR1);
	mctl_write_w((para->dram_mr2)&0xffff,DRAM_MR2);
	mctl_write_w((para->dram_mr3)&0xffff,DRAM_MR3);
	mctl_write_w(((para->dram_odt_en)>>4)&0x3,LP3MR11);

	dram_dbg_timing("[TIMING DEBUG] MR0= 0x%x\n",para->dram_mr0&0xffff);
	dram_dbg_timing("[TIMING DEBUG] MR1= 0x%x\n",para->dram_mr1&0xffff);
	dram_dbg_timing("[TIMING DEBUG] MR2= 0x%x\n",para->dram_mr2&0xffff);
	dram_dbg_timing("[TIMING DEBUG] MR3= 0x%x\n",para->dram_mr3&0xffff);
	//set dram timing
	reg_val= (twtp<<24)|(tfaw<<16)|(trasmax<<8)|(tras<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG0= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG0);//DRAMTMG0
	reg_val= (txp<<16)|(trtp<<8)|(trc<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG1= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG1);//DRAMTMG1
	reg_val= (tcwl<<24)|(tcl<<16)|(trd2wr<<8)|(twr2rd<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG2= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG2);//DRAMTMG2
	reg_val= (tmrw<<16)|(tmrd<<12)|(tmod<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG3= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG3);//DRAMTMG3
	reg_val= (trcd<<24)|(tccd<<16)|(trrd<<8)|(trp<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG4= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG4);//DRAMTMG4
	reg_val= (tcksrx<<24)|(tcksre<<16)|(tckesr<<8)|(tcke<<0);
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG5= 0x%x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG5);//DRAMTMG5
	//set two rank timing
	reg_val= mctl_read_w(DRAMTMG8);
	reg_val&=~(0xff<<8);
	reg_val&=~(0xff<<0);
	reg_val|=(0x66<<8);
	reg_val|=(0x10<<0);
	mctl_write_w(reg_val,DRAMTMG8);//DRAMTMG5
	dram_dbg_timing("[TIMING DEBUG] DRAMTMG8 = 0x%x\n",reg_val);
	//set phy interface time
	reg_val=(0x2<<24)|(t_rdata_en<<16)|(0x1<<8)|(wr_latency<<0);
	dram_dbg_timing("[TIMING DEBUG] PITMG0= 0x%x\n",reg_val);
	mctl_write_w(reg_val,PITMG0);	//PHY interface write latency and read latency configure
	//set phy time  PTR0-2 use default
	mctl_write_w(((tdinit0<<0)|(tdinit1<<20)),PTR3);
	mctl_write_w(((tdinit2<<0)|(tdinit3<<20)),PTR4);
//	mctl_write_w(0x01e007c3,PTR0);
//	mctl_write_w(0x00170023,PTR1);
//	mctl_write_w(0x00800800,PTR3);
//	mctl_write_w(0x01000500,PTR4);
	//set refresh timing
    reg_val =(trefi<<16)|(trfc<<0);
    dram_dbg_timing("[TIMING DEBUG] RFSHTMG = 0x%x\n",reg_val);
    mctl_write_w(reg_val,RFSHTMG);
}

/*****************************************************************************
作用：DRAM时钟展频设置函数
参数：__dram_para_t *para
返回值：1
1.该函数由SD1人员提供PLL展频配置方法
*****************************************************************************/
unsigned int ccm_set_pll_ddr1_sscg(__dram_para_t *para)
{
	unsigned int pll_ssc ,pll_step;
	unsigned int  ret,reg_val;
	/*计算PLL_STEP*/
	ret = (para->dram_tpr13>>16)&0xf;
	if(ret<12){
		pll_step = ret;
	}else{
		pll_step = 9;
	}
	/*计算PLL_SSC*/
	ret = (para->dram_tpr13>>20)&0x7;
	if(ret<6){
		if(ret == 0){
			if((para->dram_tpr13>>10)&0x1){
				pll_ssc = (1<<pll_step);
			}else{
				return 1;
			}
		}else{
			pll_ssc = (ret<<18)/10 - (1<<pll_step);
			pll_ssc = (pll_ssc/(1<<pll_step));
			pll_ssc = (pll_ssc*(1<<pll_step));
		}
	}else{
		pll_ssc = (8<<17)/10 - (1<<pll_step);
		pll_ssc = (pll_ssc/(1<<pll_step));
		pll_ssc = (pll_ssc*(1<<pll_step));
	}
	/*update PLL前切到自身24M时钟源*/
	reg_val = mctl_read_w(_CCM_PLL_DDR_AUX_REG);
	reg_val |= (0x1U<<4);
	mctl_write_w(reg_val, _CCM_PLL_DDR_AUX_REG);
	reg_val |= (0x1U<<1);
	mctl_write_w(reg_val, _CCM_PLL_DDR_AUX_REG);
	/*配置PLL_SSC与PLL_STEP*/
	reg_val = mctl_read_w(_CCM_PLL_DDR1_CFG_REG);
	reg_val &= ~(0x1ffff<<12);
	reg_val &= ~(0xf<<0);
	reg_val |=(pll_ssc<<12);
	reg_val |=(pll_step<<0);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_CFG_REG);
	/*Update PLL设置*/
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val |= (0x1U<<30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	dram_udelay(20);
	/*使能线性调频*/
	reg_val = mctl_read_w(_CCM_PLL_DDR1_CFG_REG);
	reg_val |= (0x1U<<31);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_CFG_REG);
	/*Update PLL设置*/
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val |= (0x1U<<30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	while((mctl_read_w(_CCM_PLL_DDR1_REG)>>30)&0x1);
	dram_udelay(20);
	return 1;
}
/*****************************************************************************
作用：DRAM时钟展频设置函数
参数：__dram_para_t *para
返回值：1
1.该函数由SD1人员提供PLL展频配置方法
*****************************************************************************/
unsigned int ccm_set_pll_ddr0_sscg(__dram_para_t *para)
{
	unsigned int  ret,reg_val;
	/*计算WAVE_BOT*/
	ret = (para->dram_tpr13>>20)&0x7;
	switch(ret){
	case 0:
		return 1;
	case 1:
		mctl_write_w((0xccccU|(0x3U<<17)|(0x48U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	case 2:
		mctl_write_w((0x9999U|(0x3U<<17)|(0x90U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	case 3:
		mctl_write_w((0x6666U|(0x3U<<17)|(0xD8U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	case 4:
		mctl_write_w((0x3333U|(0x3<<17)|(0x120U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	case 5:
		mctl_write_w(((0x3U<<17)|(0x158U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	default:
		mctl_write_w((0x3333U|(0x3<<17)|(0x120U<<20)|(0x3U<<29)|(0x1U<<31)),PLL_DDR0_PAT_CTL_REG);
		break;
	}
	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);//enable sscg
	reg_val |=(0x1U<<24);
	mctl_write_w(reg_val,_CCM_PLL_DDR0_REG);

	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);//updata
	reg_val |=(0x1U<<20);
	mctl_write_w(reg_val,_CCM_PLL_DDR0_REG);
	while(mctl_read_w(_CCM_PLL_DDR0_REG) & (0x1<<30));
	dram_udelay(20);
	return 1;
}
/*****************************************************************************
作用：DRAM时钟设置函数
参数：pll_clk -- 配置的频率
返回值：设置的PLL频率值
1.该函数由SD1人员提供PLL配置方法
*****************************************************************************/
unsigned int ccm_set_pll_ddr0_clk(__dram_para_t *para)
{
	unsigned int n, k, m = 1,rval,pll_clk;
	unsigned int div;
	unsigned int mod2, mod3;
	unsigned int min_mod = 0;

	/*启用PLL LOCK功能*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);

	if((para->dram_tpr13>>6)&0x1)
		pll_clk=(para->dram_tpr9<<1);
	else
		pll_clk=(para->dram_clk<<1);

	div = pll_clk/24;
	if (div <= 32) {
		n = div;
		k = 1;
	} else {
		/* when m=1, we cann't get absolutely accurate value for follow clock:
		 * 840(816), 888(864),
		 * 984(960)
		 */
		mod2 = div&1;
		mod3 = div%3;
		min_mod = mod2;
		k = 2;
		if (min_mod > mod3) {
			min_mod = mod3;
			k = 3;
		}
		n = div / k;
	}
	rval = mctl_read_w(_CCM_PLL_DDR0_REG);
	rval &= ~((0x1f << 8) | (0x3 << 4) | (0x3 << 0));
	rval = (1U << 31)  | ((n-1) << 8) | ((k-1) << 4) | (m-1);
	mctl_write_w(rval, _CCM_PLL_DDR0_REG);
	mctl_write_w(rval|(1U << 20), _CCM_PLL_DDR0_REG);
	/*使能PLL_DDR1的硬件LOCK功能*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval |= (0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	while(!((mctl_read_w(_CCM_PLL_DDR0_REG) & (1<<20 | 1<< 28)) == (0<<20 | 1<< 28)));
	/*清除设置值*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 4);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);

	dram_udelay(20);
	rval = ccm_set_pll_ddr0_sscg(para);
	return 24 * n * k / m;
}
/*****************************************************************************
作用：DRAM时钟设置函数
参数：pll_clk -- 配置的频率
返回值：设置的PLL频率值
1.该函数由SD1人员提供PLL配置方法
*****************************************************************************/
unsigned int ccm_set_pll_ddr1_clk(__dram_para_t *para)
{
	unsigned int rval;
	unsigned int div;
	/*启用PLL LOCK功能*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	/*计算除频因子*/
	if((para->dram_tpr13>>6)&0x1)
		div = para->dram_clk/12;
	else
		div = para->dram_tpr9/12;
	if (div < 12)
		div=12;
	/*配置除频因子并使能PLL*/
	rval = mctl_read_w(_CCM_PLL_DDR1_REG);
	rval &= ~(0x7f<<8);
	rval |=(((div-1)<<8)|(0x1U<<31));
	mctl_write_w(rval, _CCM_PLL_DDR1_REG);
	/*更新PLL配置*/
	mctl_write_w(rval|(1U << 30), _CCM_PLL_DDR1_REG);
	/*使能PLL_DDR1的硬件LOCK功能*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval |= (0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	while(!((mctl_read_w(_CCM_PLL_DDR1_REG) & (1<<30 | 1<< 28)) == (0<<30 | 1<< 28)));
	/*清除设置值*/
	rval = mctl_read_w(CCU_PLL_LOCK_CTRL_REG);
	rval &= ~(0x1 << 11);
	mctl_write_w(rval, CCU_PLL_LOCK_CTRL_REG);
	dram_udelay(100);
	/*设置展频*/
	rval = ccm_set_pll_ddr1_sscg(para);
	return 24*div;
}
/*****************************************************************************
作用：系统资源初始化
参数：__dram_para_t *para
返回值：1
*****************************************************************************/
unsigned int mctl_sys_init(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	unsigned int ret_val = 0;
	/*关闭mbus时钟*/
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val &=~(1U<<31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);
	/*mbus域reset*/
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val &=~(1U<<31);
	mctl_write_w(reg_val, MBUS_RESET_REG);
	/*关闭DRAM AHB域的时钟*/
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val &= ~(1U<<14);
	mctl_write_w(reg_val, BUS_CLK_GATE_REG0);
	/*DRAM AHB域reset*/
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val &= ~(1U<<14);
	mctl_write_w(reg_val, BUS_RST_REG0);
	/*关闭PLL_DDR0,无update不起作用*/
	reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);
	reg_val &=~(1U<<31);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	reg_val |= (0x1U<<20);
	mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	/*关闭PLL_DDR1,无update不起作用*/
	reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
	reg_val &=~(1U<<31);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	reg_val |= (0x1U<<30);
	mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	/*DRAM控制器reset*/
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U<<31);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	/*设置DRAM PLL频率*/
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x3<<20);
	if(para->dram_tpr13>>6 & 0x1){
		/*选择PLL_DDR1*/
		reg_val |= 0x1 << 20;
		ret_val = ccm_set_pll_ddr1_clk(para);
		para->dram_clk = ret_val/2;
		dram_dbg_4("pll_ddr1 = %d MHz\n",ret_val);
		/*如果使能DFS调频，开启另外一个PLL*/
		if(para->dram_tpr9 != 0){
			ret_val = ccm_set_pll_ddr0_clk(para);
			para->dram_tpr9 = ret_val/2;
			dram_dbg_4("pll_ddr0 = %d MHz\n",ret_val);
		}

	}else{
		/*选择PLL_DDR0*/
		reg_val |= 0x0 << 20;
		ret_val = ccm_set_pll_ddr0_clk(para);
		para->dram_clk = ret_val/2;
		dram_dbg_4("pll_ddr0 = %d MHz\n",ret_val);
		/*如果使能DFS调频，开启另外一个PLL*/
		if(para->dram_tpr9 != 0){
			ret_val = ccm_set_pll_ddr1_clk(para);
			para->dram_tpr9 = ret_val/2;
			dram_dbg_4("pll_ddr1 = %d MHz\n",ret_val);
		}
	}
	dram_udelay(1000);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	reg_val |= (0x1<<16);
	mctl_write_w(reg_val, _CCM_DRAMCLK_CFG_REG);
	/*释放DRAM AHB域的reset*/
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val |= (1U<<14);
	mctl_write_w(reg_val, BUS_RST_REG0);
	/*打开DRAM AHB域的时钟*/
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val |= (1U<<14);
	mctl_write_w(reg_val, BUS_CLK_GATE_REG0);
	/*保证mbus时钟开前关闭可能的访问*/
	mctl_write_w(0x1, MC_MAER);
	/*释放DRAM mbus域的reset*/
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val |=(1U<<31);
	mctl_write_w(reg_val, MBUS_RESET_REG);
	/*打开DRAM mbus域的时钟*/
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val |=(1U<<31);
	mctl_write_w(reg_val, MBUS_CLK_CTL_REG);
	/*释放DRAM控制器reset*/
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val |=(0x1U<<31);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	dram_udelay(10);
	/*使能DRAM控制器时钟*/
	mctl_write_w(0x8000,CLKEN);
	dram_udelay(10);
	return DRAM_RET_OK;
}

/*****************************************************************************
作用：配置DRAM控制器的配置
参数：__dram_para_t *para
返回值：void
*****************************************************************************/
void mctl_com_init(__dram_para_t *para)
{
	unsigned int reg_val,ret_val;
	unsigned int m=0,rank_num=1;
	if((para->dram_tpr13>>30)&0x1)//V40
	{
		reg_val = mctl_read_w(MC_WORK_MODE);
		reg_val &=(~(0x1<<27));
		mctl_write_w(reg_val,MC_WORK_MODE);
	}else{//A20E
		reg_val = mctl_read_w(MC_WORK_MODE);
		reg_val |=(0x1<<27);
		mctl_write_w(reg_val,MC_WORK_MODE);
	}
	/*if one rank should use A15*/
	if(((para->dram_para2>>12)&0xf)==0x0)
	{
		reg_val = mctl_read_w(MC_R1_WORK_MODE);
		reg_val |=(0x1<<21);
		mctl_write_w(reg_val,MC_R1_WORK_MODE);
	}
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val &= ~(0xfff000);
	reg_val |=(0x4<<20);
	reg_val |= ((para->dram_type & 0x07)<<16);//DRAM type
	reg_val |= (( ( (para->dram_para2) & 0x01 )? 0x0:0x1) << 12);	//DQ width
	if((para->dram_type==6)||(para->dram_type==7))
		reg_val |= (0x1U<<19);  //LPDDR2/3 must use 1T mode
	else
		reg_val |= (((para->dram_tpr13>>5)&0x1)<<19);//2T or 1T
	mctl_write_w(reg_val,MC_WORK_MODE);

	if((para->dram_para2 & (0x1<<8))&(((para->dram_para2>>12)&0xf)==0x1))
		rank_num = 2;
	for(m=0;m<rank_num;m++)
	{
		reg_val = mctl_read_w(MC_WORK_MODE + 0x4 * m);
		reg_val &= ~(0xfff);
		reg_val |= ( (para->dram_para2)>>12 & 0x03 );	//rank
		reg_val |= ((para->dram_para1>>(16*m + 12) & 0x01) << 2);//BANK
		reg_val |= ((( ( (para->dram_para1>>(16*m + 4)) & 0xff) - 1) & 0xf) << 4);//Row number

		switch(((para->dram_para1)>>(16*m + 0))& 0xf) 	//MCTL_PAGE_SIZE
		{//************************************IC should not half, have auto scan******************************************
		case 8:
			reg_val |= 0xA << 8;
			break;
		case 4:
			reg_val |= 0x9 << 8;
			break;
		case 2:
			reg_val |= 0x8 << 8;
			break;
		case 1:
			reg_val |= 0x7 << 8;
			break;
		default:
			reg_val |= 0x6 <<8;
			break;
		}
		mctl_write_w(reg_val,MC_WORK_MODE + 0x4 * m);
	}
	dram_dbg_8("MC_WORK_MODE is %x\n",mctl_read_w(MC_WORK_MODE));
	if(para->dram_para2 & (0x1<<8))
		dram_dbg_8("MC_WORK_MODE_rank1 is %x\n",mctl_read_w(MC_R1_WORK_MODE));

	/*ODT MAP */
	reg_val = (mctl_read_w(MC_WORK_MODE)&0x1);
	if(reg_val)
		ret_val = 0x303;
	else
		ret_val = 0x201;
	mctl_write_w(ret_val,ODTMAP);
	/*half DQ mode*/
	if(para->dram_para2&0x1){
		mctl_write_w(0,DXnGCR0(2));
		mctl_write_w(0,DXnGCR0(3));
	}
	/*address mapping */
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val |= ((para->dram_tpr4&0x3)<<25);
	mctl_write_w(reg_val,MC_WORK_MODE);

	reg_val = mctl_read_w(MC_R1_WORK_MODE);
	reg_val |= (((para->dram_tpr4>>2)&0x1ff)<<12);
	mctl_write_w(reg_val,MC_R1_WORK_MODE);
}


/*****************************************************************************
作用：DRAM控制器及DRAM初始化
参数：ch_index -- 无意义，__dram_para_t *para
返回值：0-表示初始化失败  ， 1-初始化成功
*****************************************************************************/
unsigned int mctl_channel_init(unsigned int ch_index,__dram_para_t *para)
{
	unsigned int reg_val = 0,ret_val=0;
	unsigned int dqs_gating_mode =0;
	unsigned int i =0;
	unsigned int rval = 1;
	unsigned int pad_hold = 0;
	dqs_gating_mode = (para->dram_tpr13>>2)&0x3;
/***********************************
 DPHY/APHY/DOUT相位选择-必须设置
 **********************************/
	reg_val = mctl_read_w(PGCR2);
	reg_val &= ~(0x3<<10);
	reg_val |= 0x0<<10;
	reg_val &= ~(0x3<<8);
	reg_val |= 0x3<<8;
	mctl_write_w(reg_val,PGCR2);
	dram_dbg_8("PGCR2 is %x\n",reg_val);
/***********************************
AC/DX IO设置-必须设置
 **********************************/
	ret_val = para->dram_odt_en & 0x1;
	dram_dbg_8("DRAMC read ODT type : %d (0: off  1: dynamic on).\n",ret_val);
	ret_val = ~(para->dram_odt_en)&0x1;
	for(i=0;i<4;i++)
	{
		//byte 0/byte 1/byte 3/byte 4
		reg_val = mctl_read_w(DXnGCR0(i));
		reg_val &= ~(0x3U<<4);
		reg_val |= (ret_val<<5);// ODT:2b'00 dynamic ,2b'10 off
		reg_val &= ~(0x1U<<1);	// SSTL IO mode
		reg_val &= ~(0x3U<<2);	//OE mode: 0 Dynamic
		reg_val &= ~(0x3U<<12);	//Power Down Receiver: Dynamic
		reg_val &= ~(0x3U<<14);	//Power Down Driver: Dynamic
		mctl_write_w(reg_val,DXnGCR0(i));
	}
	dram_dbg_8("DXnGCR0 = %x\n",reg_val);

	reg_val = mctl_read_w(ACIOCR0);
	reg_val |= (0x1<<1);
	reg_val &=~(0x1<<11);
	mctl_write_w(reg_val,ACIOCR0);
/***********************************
AC/DX 路径延时补偿-必须设置
 **********************************/
	bit_delay_compensation(para);
/***********************************
DQS GATE模式选择-必须设置
 **********************************/
	switch(dqs_gating_mode)
	{
		case 1://open DQS gating
			if((mctl_read_w(VDD_SYS_PWROFF_GATING)&0x3)==0x0)//standby should set after pad release
			{
				reg_val = mctl_read_w(PGCR2);
				reg_val &= ~(0x3<<6);
				mctl_write_w(reg_val,PGCR2);
			}
			reg_val = mctl_read_w(DQSGMR);
			reg_val &= ~((0x1<<8) | 0x7);
			mctl_write_w(reg_val,DQSGMR);
			dram_dbg_8("DRAM DQS gate is open.\n");
			break;
		case 2://auto gating pull up
			reg_val=mctl_read_w(PGCR2);
			reg_val&=(~(0x1<<6));
			mctl_write_w(reg_val,PGCR2);
			dram_udelay(10);

			reg_val = mctl_read_w(PGCR2);
			reg_val &= ~(0x3<<6);
			reg_val |= (0x2<<6);
			mctl_write_w(reg_val,PGCR2);

			ret_val = ((mctl_read_w(DRAMTMG2)>>16)&0x1f)-2;
			reg_val = mctl_read_w(DQSGMR);
			reg_val &= ~((0x1<<8) | (0x7));
			reg_val |= ((0x1<<8) | (ret_val));
			mctl_write_w(reg_val,DQSGMR);

			reg_val = mctl_read_w(DXCCR);//dqs pll up
			reg_val |= (0x1<<27);
			reg_val &= ~(0x1U<<31);
			mctl_write_w(reg_val,DXCCR);
			dram_dbg_8("DRAM DQS gate is PU mode.\n");
			break;
		default:
			//close DQS gating--auto gating pull down
			//for aw1680 standby problem,reset gate
			reg_val = mctl_read_w(PGCR2);
			reg_val &= ~(0x1<<6);
			mctl_write_w(reg_val,PGCR2);
			dram_udelay(10);

			reg_val = mctl_read_w(PGCR2);
			reg_val |= (0x3<<6);
			mctl_write_w(reg_val,PGCR2);
			dram_dbg_8("DRAM DQS gate is PD mode.\n");
			break;
	}
/***********************************
上下拉电阻强度选择-非必须
 **********************************/
	if((para->dram_type == 6) ||(para->dram_type == 7))
	{
		  reg_val =mctl_read_w(DXCCR);
		  reg_val &=~(0x7U<<28);
		  reg_val &=~(0x7U<<24);
		  reg_val |= (0x2U<<28);
		  reg_val |= (0x2U<<24);
		  mctl_write_w(reg_val,DXCCR);
	}
/***********************************
training设置-必须设置
 **********************************/
	if((para->dram_para2>>12)&0x1)
	{
		reg_val=mctl_read_w(DTCR);
		reg_val&=(0xfU<<28);
		reg_val|=0x03003087;
		mctl_write_w(reg_val,DTCR);  //two rank
	}
	else
	{
		reg_val=mctl_read_w(DTCR);
		reg_val&=(0xfU<<28);
		reg_val|=0x01003087;
		mctl_write_w(reg_val,DTCR);  //one rank
	}
/***********************************
核心初始化（ZQ\DDL calibration,training）
pad_hold = 0 ： boot启动
pad_hold = 3 ： standby唤醒
 **********************************/
	pad_hold = mctl_read_w(VDD_SYS_PWROFF_GATING)&0x3;
	if(pad_hold){
		/* ZQ pad release */
		reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
		reg_val &= (~( 0x1<<1));
		mctl_write_w( reg_val,VDD_SYS_PWROFF_GATING);
		dram_udelay(10);
		/* ZQ calibration */
		reg_val = mctl_read_w(ZQCR);
		reg_val &= ~(0x00ffffff);
		reg_val |= ( (para->dram_zq) & 0xffffff );
		mctl_write_w(reg_val,ZQCR);
		if(dqs_gating_mode == 1)
		{
			reg_val = 0x52;
			mctl_write_w(reg_val,PIR );
			reg_val |= (0x1<<0);
			mctl_write_w(reg_val,PIR );
			while((mctl_read_w(PGSR0 )&0x1) != 0x1);
			dram_udelay(10);
			reg_val = 0x20; //DDL CAL;
		}
		else
		{
			reg_val = 0x62;
		}
		mctl_write_w(reg_val,PIR );
		reg_val |= (0x1<<0);
		mctl_write_w(reg_val,PIR );
		dram_udelay(10);
		while((mctl_read_w(PGSR0 )&0x1) != 0x1);

		reg_val = mctl_read_w(PGCR3);
		reg_val &= (~(0x3<<25));
		reg_val |=(0x2<<25);
		mctl_write_w( reg_val,PGCR3 );
		dram_udelay(10);
		/* entry self-refresh */
		reg_val = mctl_read_w(PWRCTL);
		reg_val |= 0x1<<0;
		mctl_write_w( reg_val, PWRCTL );
		while(((mctl_read_w(STATR) & 0x7) != 0x3));

		/* pad release */
		reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);	//
		reg_val &= ~( 0x1<<0);
		mctl_write_w( reg_val,VDD_SYS_PWROFF_GATING);
		dram_udelay(10);

		/* exit self-refresh but no enable all master access */
		reg_val = mctl_read_w(PWRCTL);
		reg_val &= ~(0x1<<0);
		mctl_write_w(reg_val,PWRCTL);
		while(((mctl_read_w(STATR ) & 0x7) != 0x1))	;
		dram_udelay(15);

		/* training :DQS gate training */
		if (dqs_gating_mode == 1)
		{
			reg_val = mctl_read_w(PGCR2);
			reg_val &= ~(0x3<<6);
			mctl_write_w(reg_val,PGCR2);

			reg_val = mctl_read_w(PGCR3);
			reg_val &= (~(0x3<<25));
			reg_val |=(0x1<<25);
			mctl_write_w( reg_val,PGCR3 );
			dram_udelay(1);

			reg_val =0x401;
			mctl_write_w(reg_val,PIR);
			while((mctl_read_w(PGSR0 )&0x1) != 0x1);
		}
	}else{
#ifndef CPUS_STANDBY_CODE
		reg_val = mctl_read_w(ZQCR);
		reg_val &= ~(0x00ffffff);
		reg_val |= ( (para->dram_zq) & 0xffffff );
		mctl_write_w(reg_val,ZQCR);

		if(dqs_gating_mode == 1)
		{
			reg_val = 0x52;
			mctl_write_w(reg_val,PIR );
			reg_val |= (0x1<<0);
			mctl_write_w(reg_val,PIR );
			dram_dbg_8("GATE MODE PIR value is %x\n",reg_val);
			while((mctl_read_w(PGSR0 )&0x1) != 0x1);
			dram_udelay(10);

			reg_val = 0x520;
			if((para->dram_type) == 3)
				reg_val |= 0x1u<<7; //DDR3 RST
		}
		else
		{
			reg_val = 0x172;
			if((para->dram_type) == 3)
				reg_val |= 0x1u<<7; //DDR3 RST
		}
		mctl_write_w(reg_val,PIR );
		reg_val |= (0x1<<0);
		mctl_write_w(reg_val,PIR );
		dram_dbg_4("DRAM initial PIR value is %x\n",reg_val);
		dram_udelay(10);
		while((mctl_read_w(PGSR0 )&0x1) != 0x1);
#endif
	}
/***********************************
初始化完training信息
 **********************************/
	reg_val = mctl_read_w(PGSR0);
	if((reg_val>>20)&0xff)
	{
		/* training ERROR information */
		dram_dbg_4("[DEBUG_4]PGSR0 = 0x%x\n",reg_val);
		if((reg_val>>20)&0x1){
			dram_dbg_0("ZQ calibration error,check external 240 ohm resistor.\n");
		}
		rval = 0;
	}
/***********************************
初始化完控制器设置-必须设置
**********************************/
	//after initial done
	while((mctl_read_w(STATR )&0x1) != 0x1);
	//refresh update,from AW1680/1681
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,RFSHCTL0);
	dram_udelay(10);
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val&=~(0x1U<<31);
	mctl_write_w(reg_val,RFSHCTL0);
	dram_udelay(10);

	//after initial before write or read must clear credit value
	reg_val = mctl_read_w(MC_CCCR);
	reg_val |= (0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	dram_udelay(10);
	/*PHY choose to update PHY or command mode */
	reg_val = mctl_read_w(PGCR3);
	reg_val &= ~(0x3<<25);
	mctl_write_w(reg_val, PGCR3);
/***********************************
DQS GATE模式性能优化-非必须
**********************************/
	if((para->dram_type) == 6 || (para->dram_type) == 7)
	{
		if(dqs_gating_mode==1)
		{
			reg_val =mctl_read_w(DXCCR);
			reg_val &=~(0x3<<6);
			reg_val |= (0x1<<6);
			mctl_write_w(reg_val,DXCCR);
		}
	}
	return rval;
}

/*****************************************************************************
作用：获取DRAM容量
参数：void
返回值： DRAM容量
*****************************************************************************/
unsigned int DRAMC_get_dram_size(void)
{
	unsigned int reg_val;
	unsigned int dram_size0,dram_size1 = 0;
	unsigned int temp;

	reg_val = mctl_read_w(MC_WORK_MODE);

	temp = (reg_val>>8) & 0xf;	//page size code
	dram_size0 = (temp - 6);	//(1<<dram_size) * 512Bytes

	temp = (reg_val>>4) & 0xf;	//row width code
	dram_size0 += (temp + 1);	//(1<<dram_size) * 512Bytes

	temp = (reg_val>>2) & 0x3;	//bank number code
	dram_size0 += (temp + 2);	//(1<<dram_size) * 512Bytes

	dram_size0 = dram_size0 - 11;	//(1<<dram_size)MBytes
	dram_size0 = 1<< dram_size0;

	if(reg_val & 0x3)
	{
		reg_val = mctl_read_w(MC_R1_WORK_MODE);
		if(reg_val & 0x3)
		{
			temp = (reg_val>>8) & 0xf;	//page size code
			dram_size1 = (temp - 6);	//(1<<dram_size) * 512Bytes

			temp = (reg_val>>4) & 0xf;	//row width code
			dram_size1 += (temp + 1);	//(1<<dram_size) * 512Bytes

			temp = (reg_val>>2) & 0x3;	//bank number code
			dram_size1 += (temp + 2);	//(1<<dram_size) * 512Bytes

			dram_size1 = dram_size1 - 11;	//(1<<dram_size)MBytes
			dram_dbg_4("dram rank1 size is %d MB\n",0x1u<<dram_size1);
			dram_size1 = 1<< dram_size1;
		}
		else
			dram_size1 = dram_size0;
	}
	return (dram_size0 + dram_size1);
}
/*****************************************************************************
作用：特殊物料支持---硬件
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef USE_SPECIAL_DRAM
unsigned int dram_type_probe(__dram_para_t *para)
{
	unsigned int dram_type;
	dram_type = get_dram_type_by_gpio();
	dram_dbg_4("the gpio type set is 0x%x\n", dram_type);
	switch (dram_type)
	{
	case 0://ok dram
		break;
	case 1://can not use CS1(512M)
		{para->dram_tpr13|=0x1;para->dram_para1=0x10e40200;}
		break;
	case 2://can not use BA2(512M)
		{para->dram_tpr13|=0x1;para->dram_para1=0x00e40200;para->dram_para2 |= (0x1<<12);}
		break;
	case 3://can not use ROW13(512M)
		{para->dram_tpr13|=0x1;para->dram_para1=0x10D40200;para->dram_para2 |= (0x1<<12);}
		break;
	case 4://can not use CS1(1GB)
		{para->dram_tpr13|=0x1;para->dram_para1=0x10E80400;}
		break;
	case 5://can not use BA2(1GB)
		{para->dram_tpr13|=0x1;para->dram_para1=0x00E80400;para->dram_para2 |= (0x1<<12);}
		break;
	case 6://can not use ROW13(1GB)
		{para->dram_tpr13|=0x1;para->dram_para1=0x10D80400;para->dram_para2 |= (0x1<<12);}
		break;
	default:
		return 0;
	}
	return 1;
}
#endif
/*****************************************************************************
作用：自动配置DRAM SIZE
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_SIZE_SCAN
unsigned int auto_scan_dram_size(__dram_para_t *para)
{
	unsigned int i=0,j=0,m=0;
	unsigned int rank_num = 1,addr_line = 0;
	unsigned int reg_val=0,ret=0,cnt=0;
	unsigned int rank_base_addr = DRAM_BASE_ADDR;
	/*1.initial*/
	ret = mctl_core_init(para);
	if(ret == 0){
		 dram_dbg_error("[ERROR DEBUG] DRAM initial error : 0!\n");
		 return 0;
	}
		
	if((para->dram_para2 & (0x1<<8))&(((para->dram_para2>>12)&0xf)==0x1))
		rank_num=2;
	for(m=0;m<rank_num;m++)
	{
		for(i=0;i<64;i++)
		{
			mctl_write_w((i%2)?(rank_base_addr + 4*i):(~(rank_base_addr + 4*i)),rank_base_addr + 4*i);
		}
		reg_val=mctl_read_w(MC_WORK_MODE);
		paraconfig(&reg_val,0xf<<8,0x6<<8);	//page_sieze 512B
		paraconfig(&reg_val,0x3<<2,0x0<<2);	//4bank
		reg_val|=(0xf<<4);//16 row
		mctl_write_w(reg_val,MC_WORK_MODE);
		//row detect
		for(i=11;i<=16;i++)
		{
			ret = rank_base_addr + (1<<(i+2+9));//row-bank-column
			cnt = 0;
			for(j=0;j<64;j++)
			{
				reg_val = (j%2)?(rank_base_addr + 4*j):(~(rank_base_addr + 4*j));
				if(reg_val == mctl_read_w(ret + j*4))
				{
					cnt++;
				}
				else
					break;
			}
			if(cnt == 64)
			{
				break;
			}
		}
		if(i >= 16)
			i = 16;
		addr_line += i;	//add row size
		dram_dbg_auto("[AUTO DEBUG] rank %d row = %d \n",m,i);
		paraconfig(&(para->dram_para1), 0xffU<<(16*m + 4), i<<(16*m + 4));//row width confirmed

		//bank detect
		reg_val=mctl_read_w(MC_WORK_MODE);
		paraconfig(&reg_val,0xf<<4,0xa<<4);//11rows
		paraconfig(&reg_val,0x3<<2,0x1<<2);	// 8bank
		paraconfig(&reg_val,0xf<<8,0x6<<8);	//page_sieze 512B
		mctl_write_w(reg_val,MC_WORK_MODE);
		for(i=0;i<1;i++)
		{
			ret = rank_base_addr + (0x1U<<(i+2+9));//bank-column
			cnt = 0;
			for(j=0;j<64;j++)
			{
				reg_val = (j%2)?(rank_base_addr + 4*j):(~(rank_base_addr + 4*j));
				if(reg_val == mctl_read_w(ret + j*4))
				{
					cnt++;
				}
				else
					break;
			}
			if(cnt == 64)
			{
				break;
			}
		}
		addr_line += i + 2;	//add bank size
		dram_dbg_auto("[AUTO DEBUG] rank %d bank = %d \n",m,(4+i*4));
		paraconfig(&(para->dram_para1), 0xfU<<(16*m + 12), i<<(16*m + 12));//bank confirmed

		//pagesize(column)detect
		reg_val=mctl_read_w(MC_WORK_MODE);
		paraconfig(&reg_val,0xf<<4,0xa<<4);//11rows
		paraconfig(&reg_val,0x3<<2,0x0<<2);	//4bank
		paraconfig(&reg_val,0xf<<8,0xa<<8);//8KB
		mctl_write_w(reg_val,MC_WORK_MODE);
		for(i=9;i<=13;i++)
		{
			ret = rank_base_addr + (0x1U<<i);//column
			cnt = 0;
			for(j=0;j<64;j++)
			{
				reg_val = (j%2)?(rank_base_addr + 4*j):(~(rank_base_addr + 4*j));
				if(reg_val == mctl_read_w(ret + j*4))
				{
					cnt++;
				}
				else
					break;
			}
			if(cnt == 64)
			{
				break;
			}
		}
		if(i >= 13)
			i = 13;
		addr_line += i;	//add pagesize
		if(i==9)
			i = 0;
		else
			i = (0x1U<<(i-10));

		dram_dbg_auto("[AUTO DEBUG] rank %d page size = %d KB \n",m,i);
		paraconfig(&(para->dram_para1), 0xfU<<(16*m + 0), i<<(16*m + 0));//pagesize confirmed

		rank_base_addr += (0x1u<<(addr_line+1));	//rank1 base addr
	}
	if(rank_num > 1)
	{
		if((para->dram_para1>>16) == (para->dram_para1 & 0xffff))
		{
			para->dram_para2 &= ~(0xf<<8);
			dram_dbg_auto("rank1 config same as rank0\n");
		}
		else
		{
			para->dram_para2 &= ~(0xf<<8);
			para->dram_para2 |= 0x1<<8;
			dram_dbg_auto("rank1 config different from rank0\n");
		}
	}
	return 1;
}
#endif
/*****************************************************************************
作用：DQS GATE模式检测DRAM配置RANK&WIDTH
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_RANK_SCAN
unsigned int dqs_gate_detect(__dram_para_t *para)
{
	 unsigned int reg_val = 0;
	 unsigned int byte0_state,byte1_state,byte2_state,byte3_state;
		/* traditional gate mode use training information*/
	 reg_val = mctl_read_w(PGSR0);
	 if((reg_val>>22)&0x1){
			/* DQ group gate training state*/
			reg_val = mctl_read_w(DXnGSR0(0));
			byte0_state = (reg_val>>24)&0x3;
			reg_val = mctl_read_w(DXnGSR0(1));
			byte1_state = (reg_val>>24)&0x3;
			reg_val = mctl_read_w(DXnGSR0(2));
			byte2_state = (reg_val>>24)&0x3;
			reg_val = mctl_read_w(DXnGSR0(3));
			byte3_state = (reg_val>>24)&0x3;
	 }else{
		 /* update configuration to parameter list */
		 para->dram_para2 &=~(0xf<<0);
		 para->dram_para2 |= (0x1<<12);
		 dram_dbg_auto("[AUTO DEBUG] two rank and full DQ!\n");
		 return 1;
	 }
	 	/* state 1 : single rank,full DQ*/
		if((byte0_state==0x2)&&(byte1_state==0x2)&&(byte2_state==0x2)&&(byte3_state==0x2)){
				/* update configuration to parameter list */
				para->dram_para2 &=~(0xf<<12 | 0xf<<0);
				dram_dbg_auto("[AUTO DEBUG] single rank and full DQ!\n");
		}/* state 2 : single rank,half DQ*/
		else if((byte0_state==0x2)&&(byte1_state==0x2)){
				/* update configuration to parameter list */
				para->dram_para2 &= ~(0xf<<12);
				para->dram_para2 &=~(0xf<<0);
				para->dram_para2 |= (0x1<<0);
				dram_dbg_auto("[AUTO DEBUG] single rank and half DQ!\n");
		}/* state 3 : dual rank,half DQ*/
		else if((byte0_state==0x0)&&(byte1_state==0x0)){
				/* update configuration to parameter list */
				para->dram_para2 |= (0x1<<12);
				para->dram_para2 &=~(0xf<<0);
				para->dram_para2 |= (0x1<<0);
				dram_dbg_auto("[AUTO DEBUG] dual rank and half DQ!\n");
		}else{
			if((para->dram_tpr13>>29)&0x1){
				dram_dbg_error("DX0 state:%d\n",byte0_state);
				dram_dbg_error("DX1 state:%d\n",byte1_state);
				dram_dbg_error("DX2 state:%d\n",byte2_state);
				dram_dbg_error("DX3 state:%d\n",byte3_state);
			}
			return 0;
		}
	return 1;
}
#endif
/*****************************************************************************
作用：time out模式检测DRAM配置RANK&WIDTH以及DRAM type
参数：num - 0: type 检测   1：rank&width 检测 , __dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_RANK_SCAN
unsigned int time_out_detect(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	unsigned int ret_val = 0;
	unsigned int ret = 0;
	//unsigned int read_data = 0;
	unsigned int rank0_address = 0 ;
	unsigned int rank1_address = 0 ;
	unsigned int byte_rto[4]={0};
	unsigned int i = 0;;
	/*enable read time out*/
	reg_val = mctl_read_w(PGCR0);
	reg_val |= (0x1<<25);
	mctl_write_w(reg_val,PGCR0);

	rank0_address = DRAM_BASE_ADDR;
/***********************************
rank0 detect
 **********************************/
	mctl_write_w(0x12345678,rank0_address);
	/*read value back*/
	mctl_read_w(rank0_address);
	dram_udelay(10);
	/*trigger time out mark*/
	ret_val = (mctl_read_w(PGSR0)>>13)&0x1;
	if(ret_val){
		for(i=0;i<4;i++){
			byte_rto[i] = (mctl_read_w(DXnGSR0(i))>>28)&0x1;
		}
		if((byte_rto[0]==0)&&(byte_rto[1]==0)){
			para->dram_para2 &=~(0xf<<0);
			para->dram_para2 |= (0x1<<0);
			dram_dbg_auto("[AUTO DEBUG] DRAM width is half DQ!\n");
			ret = 1;
		}else{
			if((para->dram_tpr13>>29)&0x1){
				for(i=0;i<4;i++){
					dram_dbg_error("DX%d rank0 state:%d\n",i,byte_rto[i]);
				}
			}
			ret = 0;
		}
	}else{
		if(!(para->dram_para2&0xf)){
			 para->dram_para2 &=~(0xf<<0);
			 dram_dbg_auto("[AUTO DEBUG] DRAM width is full DQ!\n");
		}
		ret = 1;
	}
	/*clear PHY FIFO*/
	reg_val = mctl_read_w(PGCR0);
	reg_val &= ~(0x1<<26);//reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);
	dram_udelay(100);
	reg_val |= (0x1<<26);//clear reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);
	/*write 1 to clear error status*/
	reg_val = mctl_read_w(PGCR0);
	reg_val |= (0x1<<24);
	mctl_write_w(reg_val,PGCR0);
	/*clear time out error status*/
	reg_val = mctl_read_w(PGSR0);
	reg_val &= ~(0x1<<13);
	mctl_write_w(reg_val,PGSR0);
	dram_udelay(10);
	if(ret == 0){
		/*clear read time out*/
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0x1<<25);
		mctl_write_w(reg_val,PGCR0);
		return ret ;
	}
/***********************************
rank1&width detect
 **********************************/
	if((para->dram_para2>>12)&0xf){
		ret_val = ((para->dram_para1>>12)&0xf)+2;
		ret_val += ((para->dram_para1>>4)&0xff);
		ret_val += ((para->dram_para1&0xf)<<1)+9;
		rank1_address = (1<<ret_val)+0x40000000;
		mctl_write_w(0x1234abcd,rank1_address);
		/*read value back*/
		mctl_read_w(rank1_address);
		dram_udelay(10);
		ret_val = (mctl_read_w(PGSR0)>>13)&0x1;
		if(ret_val){
			for(i=0;i<4;i++){
				byte_rto[i] = (mctl_read_w(DXnGSR0(i))>>28)&0x1;
			}
			if(byte_rto[0]&&byte_rto[1]&&byte_rto[2]&&byte_rto[3]){
				/* update configuration to parameter list */
				para->dram_para2 &= ~(0xf<<12);
				dram_dbg_auto("[AUTO DEBUG] DRAM is single rank!\n");
				ret = 1;
			}else if((byte_rto[0]==0)&&(byte_rto[1]==0)){
				/* update configuration to parameter list */
				para->dram_para2 |= (0x1<<12);
				dram_dbg_auto("[AUTO DEBUG] dual rank and half DQ!\n");
				ret = 1;
			}else{
				if((para->dram_tpr13>>29)&0x1){
					for(i=0;i<4;i++){
						dram_dbg_error("DX%d rank1 state:%d\n",i,byte_rto[i]);
					}
				}
				ret = 0;
			}

		}else{
			/* update configuration to parameter list */
			 para->dram_para2 |= (0x1<<12);
			 dram_dbg_auto("[AUTO DEBUG] two rank and full DQ!\n");
			 ret = 1;
		}
		/*clear PHY FIFO*/
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0x1<<26);//reset PHY FIFO
		mctl_write_w(reg_val,PGCR0);
		dram_udelay(100);
		reg_val |= (0x1<<26);//clear reset PHY FIFO
		mctl_write_w(reg_val,PGCR0);
		/*write 1 to clear error status*/
		reg_val = mctl_read_w(PGCR0);
		reg_val |= (0x1<<24);
		mctl_write_w(reg_val,PGCR0);
		/*clear time out error status*/
		reg_val = mctl_read_w(PGSR0);
		reg_val &= ~(0x1<<13);
		mctl_write_w(reg_val,PGSR0);
		dram_udelay(10);
	}
	/*clear read time out*/
	reg_val = mctl_read_w(PGCR0);
	reg_val &= ~(0x1<<25);
	mctl_write_w(reg_val,PGCR0);

	return ret;
}
#endif
/*****************************************************************************
作用：自动配置DRAM RANK&WIDTH
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_RANK_SCAN
unsigned int auto_scan_dram_rank_width(__dram_para_t *para)
{
	 unsigned int ret_val = 0;
	 unsigned int reg_val = 0;
	 unsigned int temp_trp13 = para->dram_tpr13;
	 unsigned int temp_para1 = para->dram_para1;
	 /*use min config to detect */
	 para->dram_tpr13 |= (0x1<<0);
	 para->dram_para1 = 0x00B000B0;
	 para->dram_para2 = 0x1000;
	 if((para->dram_tpr13>>4)&0x1)
			para->dram_tpr13 |= (0x1<<2);

	ret_val = mctl_core_init(para);
	reg_val = mctl_read_w(PGSR0);
	if((reg_val>>20)&0x1)
	{
		return 0;
	}
	if(!((para->dram_tpr13>>4)&0x1)){
		ret_val =time_out_detect(para);
	}else{
			ret_val = dqs_gate_detect(para);
	}
	 if(ret_val == 0){
		  return 0;
	}
	 /*recovrey default config */
	 para->dram_tpr13 = temp_trp13;
	 para->dram_para1 = temp_para1;
	 return 1;
}
#endif
/*****************************************************************************
作用：自动配置DRAM TYPE
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_TYPE_SCAN
unsigned int auto_scan_dram_type(__dram_para_t *para)
{
		unsigned int ret_val = 0;	
		unsigned int temp_trp13 = para->dram_tpr13;
		unsigned int temp_para1 = para->dram_para1;
		unsigned int temp_para2 = para->dram_para2;
		/*use minimum config to detect,row 11,bank 4,page 512B,half DQ */
		para->dram_tpr13 |= (0x1<<0);
		para->dram_para1 = 0x00B000B0; 
		para->dram_para2 = 0x0;
		if((para->dram_tpr13>>4)&0x1)
			 para->dram_tpr13 |= (0x1<<2);
		/* try lpddr3 first,this time ddr voltage is 1.2v */
		para->dram_clk  = para->dram_tpr7&0x3ff ;
		para->dram_type = 7 ;
 		para->dram_zq   = 0x3b3bf9 ;
		para->dram_tpr10 = 0x5505;
		para->dram_tpr11 = 0x33330000;
		para->dram_tpr12 = 0x0000cccc;
		para->dram_mr3   = 0x2 ;
		dram_dbg_auto("[AUTO DEBUG] start detect lpddr3...\n");
		if(!dram_vol_set(para))
			return 0;
		ret_val = mctl_core_init(para);
		if(!((para->dram_tpr13>>4)&0x1)){
				ret_val =time_out_detect(para);
		}else{
			ret_val =dqs_gate_detect(para);
		}
		if(ret_val == 0){
			/* try lpddr3 failed ,try lpddr2*/
				para->dram_clk  = (para->dram_tpr7>>10)&0x3ff ;
				para->dram_type = 6 ;
	 		 	para->dram_zq   = 0x3b3bf9 ;
				para->dram_tpr10 = 0x5505;
				para->dram_tpr11 = 0x33330000;
				para->dram_tpr12 = 0x0000cccc;
				para->dram_mr3   = 0x2 ;
				dram_dbg_auto("[AUTO DEBUG] start detect lpddr2...\n");
				ret_val = mctl_core_init(para);
				if(!((para->dram_tpr13>>4)&0x1)){
					ret_val =time_out_detect(para);
				}else{
					ret_val =dqs_gate_detect(para);
				}
				if(ret_val == 0){
						para->dram_clk  = (para->dram_tpr7>>20)&0x3ff ;
						para->dram_type = 3 ;
	 		 			para->dram_zq   = 0x3b3bf9 ;
						para->dram_tpr10 = 0x5505;
						para->dram_tpr11 = 0x33330000;
						para->dram_tpr12 = 0x0000bbbb;
						para->dram_mr1 = 0x40;
						dram_dbg_auto("[AUTO DEBUG] start detect ddr3...\n");
						if(!dram_vol_set(para))
							return 0;
						ret_val = mctl_core_init(para);
						if(!((para->dram_tpr13>>4)&0x1)){
							ret_val =time_out_detect(para);
						}else{
							ret_val =dqs_gate_detect(para);
						}
						if(ret_val == 0){
							return 0;
						}else{
								//ddr3 try success.
							dram_dbg_auto("[AUTO DEBUG] ddr3 try success\n");
						}
				}else{
						//lpddr2 try success.
					dram_dbg_auto("[AUTO DEBUG] lpddr2 try success\n");
				}
		}else{
			//lpddr3 try success.
			dram_dbg_auto("[AUTO DEBUG] lpddr3 try success\n");
			
		}
	/*recovrey default config */
	para->dram_tpr13 = temp_trp13;
	para->dram_para1 = temp_para1;
	para->dram_para2 = temp_para2;
	return 1;	
}
#endif
/*****************************************************************************
作用：自动配置DRAM，include type,rank,width,row,col,bank
参数：__dram_para_t *para
返回值：0-表示自动配置失败  ， 1-自动配置成功
*****************************************************************************/
#ifdef DRAM_AUTO_SCAN
unsigned int auto_scan_dram_config(__dram_para_t *para)
{
	 unsigned int ret_val = 0;	
#ifdef DRAM_TYPE_SCAN
	 /*1.type detect*/
	 if(!((para->dram_tpr13>>13)&0x1)){
	 		ret_val = auto_scan_dram_type(para);
	 		if(ret_val == 0){
	 			dram_dbg_error("[ERROR DEBUG] auto scan dram type fail !\n");
	 			return 0;
	 		}
	 }
#endif
#ifdef DRAM_RANK_SCAN
	 /*2.rank&width detect*/
	 if(!((para->dram_tpr13>>14)&0x1)){
		 ret_val = auto_scan_dram_rank_width(para);
		 if(ret_val == 0){
			 dram_dbg_error("[ERROR DEBUG] auto scan dram rank&width fail !\n");
			 return 0;
		 }
	 }
#endif
#ifdef DRAM_SIZE_SCAN
	 /*3.size detect (row,col,bank)*/
	 ret_val = auto_scan_dram_size(para);
	 if(ret_val == 0){
		 dram_dbg_error("[ERROR DEBUG] auto scan dram size fail !\n");
	 		return 0;
	 }
#endif
	 /*4.disable auto detect function*/
	 if(!((para->dram_tpr13>>15)&0x1)){
		 para->dram_tpr13 |= (0x3<<0);
		 para->dram_tpr13 &= ~(0x3<<13);
	 }
	 return 1;
}
#endif
/*****************************************************************************
作用：DRAM初始化完后读写测试
参数：dram_size -- DRAM容量 ，test_length--测试长度
返回值：0-表示测试PASS  ， 其他-表示测试FAIL
*****************************************************************************/
unsigned int dramc_simple_wr_test(unsigned int dram_size,unsigned int test_length)
{
	/* DRAM Simple write_read test
	 * 2 ranks:  write_read rank0 and rank1;
	 * 1rank  : write_read rank0 base and half size;
	 * */
	unsigned int i;
	unsigned int half_size;
	unsigned int val;
	half_size = ((dram_size >> 1)<<20);

	for(i=0;i<test_length;i++)
	{
		mctl_write_w(0x01234567 + i,(DRAM_BASE_ADDR + i*4));//rank0
		mctl_write_w(0xfedcba98 + i,(DRAM_BASE_ADDR + half_size + i*4));//half size (rank1)
	}

	for(i=0;i<test_length;i++)
	{
		val = mctl_read_w(DRAM_BASE_ADDR + half_size + i*4);
		if(val !=(0xfedcba98 + i))	/* Write last,read first */
		{
			dram_dbg_error("DRAM simple test FAIL.\n");
			dram_dbg_error("%x != %x at address %x\n",val,0xfedcba98 + i,DRAM_BASE_ADDR + half_size + i*4);
			return DRAM_RET_FAIL;
		}
		val = mctl_read_w(DRAM_BASE_ADDR + i*4);
		if(val != (0x01234567+i))
		{
			dram_dbg_error("DRAM simple test FAIL.\n");
			dram_dbg_error("%x != %x at address %x\n",val,0x01234567 + i,DRAM_BASE_ADDR + i*4);
			return DRAM_RET_FAIL;
		}
	}
	dram_dbg_0("DRAM simple test OK.\n");
	return DRAM_RET_OK;
}

/*****************************************************************************
作用：核心初始化函数
参数：__dram_para_t *para
返回值：0-表示初始化失败  ， 1-初始化成功
*****************************************************************************/
unsigned int mctl_core_init(__dram_para_t *para)
{
	unsigned int ret_val = 0;
	mctl_sys_init(para);
	mctl_com_init(para);
	auto_set_timing_para(para);
	ret_val=mctl_channel_init(0,para);
	return ret_val;
}
/*****************************************************************************
作用：boot阶段DRAM初始化入口函数
参数：int type - 无意义   __dram_para_t *para
返回值：0-表示初始化失败  ， DRAM容量-初始化成功
*****************************************************************************/
signed int init_DRAM(int type, __dram_para_t *para)
{
	unsigned int ret_val=0;
	unsigned int reg_val=0;
	unsigned int pad_hold=0;
	unsigned int dram_size = 0;

	if((mctl_read_w(0x01c15000+0x4)&0x7)==0x1)//V40
	{
		para->dram_tpr13 |=(0x1<<30);
		dram_dbg_0("DRAMC IS V40\n");
	}else{//A20E
		para->dram_tpr13 &=(~(0x1<<30));
		dram_dbg_0("DRAMC IS A20E\n");
	}
/*****************************************************************************
boot启动&standby唤醒判断
1.BOOT阶段才执行chip_id\电压配置\自动配置\低成本配置
2.对于有CPUS/normal standby的平台，定义CPUS_STANDBY_CODE节省编译量
*****************************************************************************/
	pad_hold = mctl_read_w(VDD_SYS_PWROFF_GATING)&0x3;
	if(pad_hold){
		dram_dbg_0("DRAM STANDBY DRIVE INFO: V0.1\n");
	}else{
#ifndef CPUS_STANDBY_CODE
		dram_dbg_0("DRAM BOOT DRIVE INFO: V0.1\n");
/*****************************************************************************
chid_id功能块
1.平台无CHIPID的，需要在mctl_hal.h关闭USE_CHIPID宏定义
*****************************************************************************/
#ifndef IC_VERIFY
#ifdef USE_CHIPID
	if(!((para->dram_tpr13>>7)&0x1)){
		if(binding_chip_id()){
			dram_dbg_0("ic cant match axp, please check...\n");
			return 0;
		}
		dram_dbg_0("chip id check OK\n");
	}
#endif
#endif
/*****************************************************************************
电压配置功能块
1.平台无搭配PMU的，需要在mctl_hal.h关闭USE_PMU宏定义
*****************************************************************************/
#ifdef USE_PMU
	if(((para->dram_tpr13>>13)&0x1)|((para->dram_tpr13>>30)&0x1)){
		ret_val = dram_vol_set(para);
		if(ret_val == 0)
			return 0;
	}
#endif
/*****************************************************************************
DRAM自动配置功能块
1.可在mctl_hal.h关闭宏关闭对应的代码
*****************************************************************************/
#ifdef DRAM_AUTO_SCAN
	if(!(para->dram_tpr13&0x1)){
		ret_val=auto_scan_dram_config(para);
		if(ret_val == 0)
			return 0;
	}
#endif
/*****************************************************************************
特殊物料支持功能块
*****************************************************************************/
#ifdef USE_SPECIAL_DRAM
	ret_val = dram_type_probe(para);
	if(ret_val == 0)
		return 0;
#endif
#endif//END CPUS_STANDBY_CODE
	}//END BOOT DETECT
/*****************************************************************************
core初始化
*****************************************************************************/
	dram_dbg_0("DRAM CLK =%d MHZ\n", para->dram_clk);
	dram_dbg_0("DRAM Type =%d (2:DDR2,3:DDR3,6:LPDDR2,7:LPDDR3)\n", para->dram_type);
	dram_dbg_0("DRAM zq value: 0x%x\n",para->dram_zq);
	ret_val=mctl_core_init(para);
	if(ret_val==0){
		dram_dbg_0("DRAM initial error : 1 !\n");
		 return 0;
	}
/*****************************************************************************
DRAM空间配置功能块
1.可通过dram_para2 bit31 置1强制配置DRAM空间使用范围
*****************************************************************************/
	if((para->dram_para2>>31)&0x1){
		dram_size = (para->dram_para2>>16)&0x7fff;
	}else{
		dram_size = DRAMC_get_dram_size();
		dram_dbg_0("DRAM SIZE =%d M\n", dram_size);
		para->dram_para2 = (para->dram_para2 & 0xffff) | (dram_size<<16);
	}
/*****************************************************************************
功耗相关
1.HDR/DDR CLK动态
2.关闭ZQ校准模块
*****************************************************************************/
	if((para->dram_tpr13>>9) & 0x1){
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0xf<<12);
		reg_val |= (0x5<<12);
		mctl_write_w(reg_val,PGCR0);
		dram_dbg_8("HDR\DDR always on mode!\n");
	}else{
		reg_val = mctl_read_w(PGCR0);
		reg_val &= ~(0xf<<12);
		mctl_write_w(reg_val,PGCR0);
		dram_dbg_8("HDR\DDR dynamic mode!\n");
	}
	reg_val = mctl_read_w(ZQCR);
	reg_val |= (0x1U<<31);
	mctl_write_w(reg_val, ZQCR);
/*****************************************************************************
性能优化
1.VTF功能
2.PAD HOLD功能
3.LPDDR3 ODT delay
*****************************************************************************/
	if((para->dram_tpr13>>8)&0x1){
		reg_val=mctl_read_w(VTFCR);
		reg_val |= (0x1<<8);
		reg_val |= (0x1<<9);
		mctl_write_w(reg_val,VTFCR);
		dram_dbg_8("VTF enable\n");
	}
	if((para->dram_tpr13>>26)&0x1){
		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x1<<13);
		mctl_write_w(reg_val,PGCR2);
		dram_dbg_8("DQ hold disable!\n");
	}else{
		reg_val = mctl_read_w(PGCR2);
		reg_val |= (0x1<<13);
		mctl_write_w(reg_val,PGCR2);
		dram_dbg_8("DQ hold enable!\n");
	}
	if(para->dram_type == 7)
	{
		reg_val=mctl_read_w(ODTCFG);
		reg_val &= ~(0xf<<16);
		reg_val |= (0x1<<16);
		mctl_write_w(reg_val,ODTCFG);
	}
/*****************************************************************************
优先级设置功能块
1.可通过dram_tpr13 bit27关闭改功能
2.时间单元TMR在此依据MBUS时钟设置
*****************************************************************************/
	if(!((para->dram_tpr13>>27) & 0x1)){
		set_master_priority();
	}
/*****************************************************************************
简单读写测试功能块
1.standby唤醒不能进行读写测试
2.dram_tpr13 bit28置1可强制读写作为standby调试
3.在mctl_sys_init里禁止MASTER访问DRAM(),在此开放访问
4.standby唤醒master的开放使能由SW人员做完CRC校验开放
*****************************************************************************/
	if((pad_hold == 0)||((para->dram_tpr13>>28)&0x1)){
		mctl_write_w(0xffffffff,MC_MAER);
		ret_val = dramc_simple_wr_test(dram_size,0x100);
		if(ret_val)
			return 0;
	}
/*****************************************************************************
END
*****************************************************************************/
	return dram_size;
}

/*****************************************************************************
作用：IC验证阶段DRAM初始化入口函数
参数：void
返回值：0-表示初始化失败  ， 其他-初始化成功
1.做驱动使用时，屏蔽该代码
*****************************************************************************/
#ifndef SYSTEM_VERIFY
unsigned int mctl_init(void)
{
	signed int ret_val=0;
	__dram_para_t dram_para;
	/*DDR2参数*/
	/*
	dram_para.dram_clk			= 768;
	dram_para.dram_type			= 7;
	dram_para.dram_zq			= 0x3b3bf9;
	dram_para.dram_odt_en       = 0x31;
	dram_para.dram_para1		= 0x10e410e4;
	dram_para.dram_para2		= 0x0;
	dram_para.dram_mr0			= 0x0;
	dram_para.dram_mr1			= 0xc3;
	dram_para.dram_mr2			= 0x6;
	dram_para.dram_mr3			= 0x2;
	dram_para.dram_tpr0 		= 0x00461991;
	dram_para.dram_tpr1 		= 0x012121cb;
	dram_para.dram_tpr2 		= 0x0003301d;
	dram_para.dram_tpr3 		= 0xB4787896;
	dram_para.dram_tpr4 		= 0x0;
	dram_para.dram_tpr5         = 0x0;
	dram_para.dram_tpr6         = 0x0;
	dram_para.dram_tpr7         = 0x1e08a1e0;
	dram_para.dram_tpr8         = 0x0;
	dram_para.dram_tpr9         = 480;
	dram_para.dram_tpr10        = 0x5505;
	dram_para.dram_tpr11       	= 0x33330000;
	dram_para.dram_tpr12        = 0x00008888;
	dram_para.dram_tpr13       	= 0x040062d1;
	//	dram_para.dram_tpr13       	= 0x04496dd1;//CFS
    */

	/*DDR3参数*/
	dram_para.dram_clk			= 480;
	dram_para.dram_type			= 3;
	dram_para.dram_zq			= 0x3b3bf9;
	dram_para.dram_odt_en       = 0x1;
	dram_para.dram_para1		= 0x10e410e4;
	dram_para.dram_para2		= 0x0000;
	dram_para.dram_mr0			= 0x840;
	dram_para.dram_mr1			= 0x40;
	dram_para.dram_mr2			= 0x8;
	dram_para.dram_mr3			= 0x2;
	dram_para.dram_tpr0 		= 0x0047194f;
	dram_para.dram_tpr1 		= 0x01b1a94b;
	dram_para.dram_tpr2 		= 0x00061043;
	dram_para.dram_tpr3 		= 0xB4787896;
	dram_para.dram_tpr4 		= 0x0;
	dram_para.dram_tpr5         = 0x0;
	dram_para.dram_tpr6         = 0x0;
	dram_para.dram_tpr7         = 0x1e08a1e0;
	dram_para.dram_tpr8         = 0x0;
	dram_para.dram_tpr9         = 480;
	dram_para.dram_tpr10        = 0x5505;
	dram_para.dram_tpr11       	= 0x33330000;
	dram_para.dram_tpr12        = 0x00008888;
	dram_para.dram_tpr13       	= 0x0c0062d1;

	/*LPDDR2参数*/
	/*
	dram_para.dram_clk			= 480;
	dram_para.dram_type			= 6;
	dram_para.dram_zq			= 0x3b3bf9;
	dram_para.dram_odt_en       = 0x31;
	dram_para.dram_para1		= 0x10F410F4;
	dram_para.dram_para2		= 0x1000;
	dram_para.dram_mr0			= 0x0;
	dram_para.dram_mr1			= 0xc3;
	dram_para.dram_mr2			= 0x6;
	dram_para.dram_mr3			= 0x2;
	dram_para.dram_tpr0 		= 0x00461991;
	dram_para.dram_tpr1 		= 0x012121cb;
	dram_para.dram_tpr2 		= 0x0003301d;
	dram_para.dram_tpr3 		= 0xB4787896;
	dram_para.dram_tpr4 		= 0x0;
	dram_para.dram_tpr5         = 0x0;
	dram_para.dram_tpr6         = 0x0;
	dram_para.dram_tpr7         = 0x1e08a1e0;
	dram_para.dram_tpr8         = 0x0;
	dram_para.dram_tpr9         = 0x0;
	dram_para.dram_tpr10        = 0x446f;
	dram_para.dram_tpr11       	= 0x018634a8;
	dram_para.dram_tpr12        = 0x88770000;
	dram_para.dram_tpr13       	= 0x901;
	*/

	/*DDR2参数*/
	/*
	dram_para.dram_clk			= 408;
	dram_para.dram_type			= 2;
	dram_para.dram_zq			= 0x3b3bf9;
	dram_para.dram_odt_en       = 0x1;
	dram_para.dram_para1		= 0x10D410D4;
	dram_para.dram_para2		= 0x1000;
	dram_para.dram_mr0			= 0xa63;
	dram_para.dram_mr1			= 0x0;
	dram_para.dram_mr2			= 0x0;
	dram_para.dram_mr3			= 0x0;
	dram_para.dram_tpr0 		= 0x0045110d;
	dram_para.dram_tpr1 		= 0x00a118c9;
	dram_para.dram_tpr2 		= 0x00042030;
	dram_para.dram_tpr3 		= 0xB4787896;
	dram_para.dram_tpr4 		= 0x0;
	dram_para.dram_tpr5         = 0x0;
	dram_para.dram_tpr6         = 0x0;
	dram_para.dram_tpr7         = 0x1e08a1e0;
	dram_para.dram_tpr8         = 0x0;
	dram_para.dram_tpr9         = 0x0;
	dram_para.dram_tpr10        = 0x0;
	dram_para.dram_tpr11       	= 0x0;
	dram_para.dram_tpr12        = 0x0;
	dram_para.dram_tpr13       	= 0x901;
	*/
	ret_val = init_DRAM(0, &dram_para);//signed int init_DRAM(int type, void *para);

#ifdef IC_VERIFY
	ret_val = dramc_ic_test(&dram_para);
#endif
	return ret_val;
}
#endif
#else
/********************************************************************************
 *FPGA boot code
 ********************************************************************************/
void local_delay (unsigned int n)
{
	while(n--);
}
/*****************************************************************************
作用：FPGA 读相位training
参数：void
返回值：1
*****************************************************************************/
unsigned int mctl_soft_training(void)
{
#ifndef DRAM_FPGA_HALF_DQ_TEST
	int i, j;
	unsigned int k;
	unsigned int delay[4];
	const unsigned int words[64] = {	0x12345678, 0xaaaaaaaa, 0x55555555, 0x00000000, 0x11223344, 0xffffffff, 0x55aaaa55, 0xaa5555aa,
								0x23456789, 0x18481113, 0x01561212, 0x12156156, 0x32564661, 0x61532544, 0x62658451, 0x15564795,
								0x10234567, 0x54515152, 0x33333333, 0xcccccccc, 0x33cccc33, 0x3c3c3c3c, 0x69696969, 0x15246412,
								0x56324789, 0x55668899, 0x99887744, 0x00000000, 0x33669988, 0x66554477, 0x5555aaaa, 0x54546212,
								0x21465854, 0x66998877, 0xf0f0f0f0, 0x0f0f0f0f, 0x77777777, 0xeeeeeeee, 0x3333cccc, 0x52465621,
								0x24985463, 0x22335599, 0x78945623, 0xff00ff00, 0x00ff00ff, 0x55aa55aa, 0x66996699, 0x66544215,
								0x54484653, 0x66558877, 0x36925814, 0x58694712, 0x11223344, 0xffffffff, 0x96969696, 0x65448861,
								0x48898111, 0x22558833, 0x69584701, 0x56874123, 0x11223344, 0xffffffff, 0x99669966, 0x36544551};

	for(i=0;i<4;i++)
		delay[i]=0;
	for(i=0; i<0x10; i++)
		{
			for(j=0; j<0x4; j++)
			{
				mctl_write_w(((3-j)<<20)|((0xf-i)<<16)|0x400f,MCTL_CTL_BASE+0xc);
				for(k=0; k<0x10; k++);
				for(k=0; k<(1<<10); k++)
				{
					mctl_write_w(words[k%64],DRAM_BASE_ADDR+(k<<2));
				}

				for(k=0; k<(1<<10); k++)
				{
					if(words[k%64] != mctl_read_w(DRAM_BASE_ADDR+(k<<2)))
					break;
				}

				if(k==(1<<10))
				{
					delay[j]=((3-j)<<20)|((0xf-i)<<16)|0x400f;
				}
			}
		}

	if(delay[0]!=0)
	{
		mctl_write_w(delay[0],MCTL_CTL_BASE+0xc);
	}
	else if(delay[1]!=0)
	{
		mctl_write_w(delay[1],MCTL_CTL_BASE+0xc);
	}
	else if(delay[2]!=0)
	{
		mctl_write_w(delay[2],MCTL_CTL_BASE+0xc);
	}
	else if(delay[3]!=0)
	{
		mctl_write_w(delay[3],MCTL_CTL_BASE+0xc);
	}

		return 1;
#else
		mctl_write_w(0x27400f,MCTL_CTL_BASE+0xc);
		return 1;
#endif
}

/*****************************************************************************
作用：FPGA系统资源初始化
参数：__dram_para_t *para
返回值：1
*****************************************************************************/
unsigned int mctl_sys_init(__dram_para_t *para)
{

	unsigned int reg_val = 0;
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U<<31);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);

	mctl_write_w(0x25ffff,CLKEN);

	return 0;
}
/*****************************************************************************
作用：FPGA DRAM控制器及DRAM初始化
参数：ch_index -- 无意义，__dram_para_t *para
返回值：0-表示初始化失败  ， 1-初始化成功
*****************************************************************************/
unsigned int mctl_channel_init(unsigned int ch_index,__dram_para_t *para)
{
	unsigned int reg_val;
#if 1       //DDR2---col 10,row 14,bank 3,rank 1
	mctl_write_w(0x4219D5,MC_WORK_MODE);// 0x0x4219D5--map0 32bit //0x0x4299D5--map1 32bit
#ifdef DRAM_FPGA_HALF_DQ_TEST
	mctl_write_w(0x4208D5,MC_WORK_MODE);//map0;default 0x4208D5--16bit
#endif
	mctl_write_w(mctl_read_w(_CCM_DRAMCLK_CFG_REG)|(0x1U<<31),_CCM_DRAMCLK_CFG_REG);
	mctl_write_w(0x00070005,RFSHTMG);
	mctl_write_w(0xa63,DRAM_MR0);
	mctl_write_w(0x00,DRAM_MR1);
	mctl_write_w(0,DRAM_MR2);
	mctl_write_w(0,DRAM_MR3);
	mctl_write_w(0x01e007c3,PTR0);
	//mctl_write_w(0x00170023,PTR1);
	mctl_write_w(0x00800800,PTR3);
	mctl_write_w(0x01000500,PTR4);
	mctl_write_w(0x01000081,DTCR);
	mctl_write_w(0x03808620,PGCR1);
	mctl_write_w(0x02010101,PITMG0);
	mctl_write_w(0x06021b02,DRAMTMG0);
	mctl_write_w(0x00020102,DRAMTMG1);
	mctl_write_w(0x03030306,DRAMTMG2);
	mctl_write_w(0x00002006,DRAMTMG3);
	mctl_write_w(0x01020101,DRAMTMG4);
	mctl_write_w(0x05010302,DRAMTMG5);
#else  //DDR3
	mctl_write_w(0x004318e4,MC_WORK_MODE);
	mctl_write_w(mctl_read_w(_CCM_DRAMCLK_CFG_REG)|(0x1U<<31),_CCM_DRAMCLK_CFG_REG);
	mctl_write_w(0x00070005,RFSHTMG);
	mctl_write_w(0x420,DRAM_MR0);
	mctl_write_w(0,DRAM_MR1);
	mctl_write_w(0,DRAM_MR2);
	mctl_write_w(0,DRAM_MR3);
	mctl_write_w(0x01e007c3,PTR0);
	mctl_write_w(0x00170023,PTR1);
	mctl_write_w(0x00800800,PTR3);
	mctl_write_w(0x01000500,PTR4);
	mctl_write_w(0x01000081,DTCR);
	mctl_write_w(0x03808620,PGCR1);
	mctl_write_w(0x02010101,PITMG0);
	mctl_write_w(0x0b091b0b,DRAMTMG0);
	mctl_write_w(0x00040310,DRAMTMG1);
	mctl_write_w(0x03030308,DRAMTMG2);
	mctl_write_w(0x00002007,DRAMTMG3);
	mctl_write_w(0x04020204,DRAMTMG4);
	mctl_write_w(0x05050403,DRAMTMG5);
	reg_val = mctl_read_w(CLKEN);
	reg_val |= (0x3<<20);
	mctl_write_w(reg_val,CLKEN);
#endif


	reg_val = 0x000183;		//PLL enable, PLL6 should be dram_clk/2
	mctl_write_w(reg_val,PIR);	//for fast simulation
	while((mctl_read_w(PGSR0 )&0x1) != 0x1);	//for fast simulation
	while((mctl_read_w(STATR )&0x1) != 0x1);	//init done

	reg_val = mctl_read_w(MC_CCCR);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	local_delay(20);

	mctl_write_w(0x00aa0060,PGCR3);//

	reg_val = mctl_read_w(RFSHCTL0);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,RFSHCTL0);
	local_delay(200);
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val&=~(0x1U<<31);
	mctl_write_w(reg_val,RFSHCTL0);
	local_delay(200);

	reg_val = mctl_read_w(MC_CCCR);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	local_delay(20);

	return (mctl_soft_training());

}
/*****************************************************************************
作用：获取DRAM容量
参数：void
返回值： DRAM容量
*****************************************************************************/
unsigned int DRAMC_get_dram_size(void)
{
	unsigned int reg_val;
	unsigned int dram_size0,dram_size1 = 0;
	unsigned int temp;


	reg_val = mctl_read_w(MC_WORK_MODE);

	temp = (reg_val>>8) & 0xf;	//page size code
	dram_size0 = (temp - 6);	//(1<<dram_size) * 512Bytes

	temp = (reg_val>>4) & 0xf;	//row width code
	dram_size0 += (temp + 1);	//(1<<dram_size) * 512Bytes

	temp = (reg_val>>2) & 0x3;	//bank number code
	dram_size0 += (temp + 2);	//(1<<dram_size) * 512Bytes

	dram_size0 = dram_size0 - 11;	//(1<<dram_size)MBytes
	dram_size0 = 1<< dram_size0;

	if(reg_val & 0x3)
	{
		reg_val = mctl_read_w(MC_R1_WORK_MODE);
		if(reg_val & 0x3)
		{
			temp = (reg_val>>8) & 0xf;	//page size code
			dram_size1 = (temp - 6);	//(1<<dram_size) * 512Bytes

			temp = (reg_val>>4) & 0xf;	//row width code
			dram_size1 += (temp + 1);	//(1<<dram_size) * 512Bytes

			temp = (reg_val>>2) & 0x3;	//bank number code
			dram_size1 += (temp + 2);	//(1<<dram_size) * 512Bytes

			dram_size1 = dram_size1 - 11;	//(1<<dram_size)MBytes
			//dram_dbg("dram rank1 size is %d MB\n",0x1u<<dram_size1);
			dram_size1 = 1<< dram_size1;
		}
		else
			dram_size1 = dram_size0;
	}
	return (dram_size0 + dram_size1);
}

/*****************************************************************************
作用：FPGA normal standby唤醒函数
参数：void
返回值：1
*****************************************************************************/
unsigned int fpga_power_up_process()
{
	unsigned int reg_val = 0;
	mctl_write_w(0x4219D5,MC_WORK_MODE);// 0x0x4219D5--map0 32bit //0x0x4299D5--map1 32bit
	mctl_write_w(mctl_read_w(_CCM_DRAMCLK_CFG_REG)|(0x1U<<31),_CCM_DRAMCLK_CFG_REG);
	mctl_write_w(0x00070005,RFSHTMG);
	mctl_write_w(0xa63,DRAM_MR0);
	mctl_write_w(0x00,DRAM_MR1);
	mctl_write_w(0,DRAM_MR2);
	mctl_write_w(0,DRAM_MR3);
	mctl_write_w(0x01e007c3,PTR0);
	//mctl_write_w(0x00170023,PTR1);
	mctl_write_w(0x00800800,PTR3);
	mctl_write_w(0x01000500,PTR4);
	mctl_write_w(0x01000081,DTCR);
	mctl_write_w(0x03808620,PGCR1);
	mctl_write_w(0x02010101,PITMG0);
	mctl_write_w(0x06021b02,DRAMTMG0);
	mctl_write_w(0x00020102,DRAMTMG1);
	mctl_write_w(0x03030306,DRAMTMG2);
	mctl_write_w(0x00002006,DRAMTMG3);
	mctl_write_w(0x01020101,DRAMTMG4);
	mctl_write_w(0x05010302,DRAMTMG5);

	/* 1.pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= ~(0x3<<0);
	mctl_write_w(reg_val,VDD_SYS_PWROFF_GATING);

	reg_val = 0x000183;		//PLL enable, PLL6 should be dram_clk/2
	mctl_write_w(reg_val,PIR);	//for fast simulation
	while((mctl_read_w(PGSR0 )&0x1) != 0x1);	//for fast simulation
	while((mctl_read_w(STATR )&0x1) != 0x1);	//init done

	reg_val = mctl_read_w(MC_CCCR);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	local_delay(20);

	mctl_write_w(0x00aa0060,PGCR3);//

	reg_val = mctl_read_w(RFSHCTL0);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,RFSHCTL0);
	local_delay(200);
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val&=~(0x1U<<31);
	mctl_write_w(reg_val,RFSHCTL0);
	local_delay(200);

	reg_val = mctl_read_w(MC_CCCR);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	local_delay(20);

	mctl_write_w(0x27400f,MCTL_CTL_BASE+0xc);

	/*2.exit self refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1<<0);
	reg_val &= ~(0x1<<8);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has enter selfrefresh
	while(((mctl_read_w(STATR)&0x7) != 0x1));
	/*3.enable master access */
	mctl_write_w(0xffffffff,MC_MAER);
	return 1;
}
/*****************************************************************************
作用：DRAM初始化入口函数
参数：int type - 无意义   __dram_para_t *para
返回值：0-表示初始化失败  ， 其他-初始化成功
*****************************************************************************/
signed int init_DRAM(int type, __dram_para_t *para)
{
	unsigned int ret_val=0;
	unsigned int pad_hold = 0;
	pad_hold = mctl_read_w(VDD_SYS_PWROFF_GATING)&0x3;
	mctl_sys_init(para);

	if(pad_hold == 0x3){
		printf("DRAM standby version V0.2\n");
		fpga_power_up_process();
	}else{
		printf("DRAM boot version V0.2\n");
		ret_val=mctl_channel_init(0,para);
		if(ret_val==0)
		return 0;
	}

	ret_val = DRAMC_get_dram_size();
	return ret_val;
}

/*****************************************************************************
作用：FPGA boot验证阶段DRAM初始化入口函数
参数：void
返回值：0-表示初始化失败  ， 其他-初始化成功
*****************************************************************************/
unsigned int mctl_init()
{
	unsigned int ret_val = 0;
	__dram_para_t dram_para;
	ret_val = init_DRAM(0, &dram_para);
	return ret_val;
}
#endif
