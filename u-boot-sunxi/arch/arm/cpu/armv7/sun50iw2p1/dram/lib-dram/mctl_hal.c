//*****************************************************************************
//	Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)

//
//	File: 				mctl_hal.c
//
//	Description:  This file implements basic functions for AW1667 DRAM controller
//		 
//	History:
//				2013/09/18		Golden			0.10	Initial version
//*****************************************************************************

#include <configs/sun50iw2p1.h>
#include <asm/io.h>
#include <asm/arch/dram.h>
#include "mctl_hal.h"
#include "mctl_reg.h"


extern int printf(const char *fmt, ...);
extern void __usdelay (unsigned int n);
extern int set_ddr_voltage ( int n);
extern void * memcpy(void * dest,const void *src,size_t count);

void local_delay (unsigned int n)
{
	while(n--);
}
void paraconfig(unsigned int *para, unsigned int mask, unsigned int value)
{
	*para &= ~(mask);
	*para |= value;
}


#ifndef FPGA_PLATFORM
void auto_set_test_para(__dram_para_t *para)
{
	unsigned int ret = 0;
#if defined SOURCE_CLK_USE_DDR_PLL0
	para->dram_tpr13 &= ~(0x1<<6);
#endif

#if defined SOURCE_CLK_USE_DDR_PLL1
	para->dram_tpr13 &= ~(0x1<<6);
	para->dram_tpr13 |= (0x1<<6);
#endif
#if defined DRAMC_USE_AUTO_DQS_GATE_PD_MODE
	para->dram_tpr13 &= ~(0x3<<2);
	para->dram_tpr13 |= (0<<2);
#endif

#if defined DRAMC_USE_DQS_GATING_MODE
	para->dram_tpr13 &= ~(0x7<<2);
	para->dram_tpr13 |= (1<<2);
#endif

#if defined DRAMC_USE_AUTO_DQS_GATE_PU_MODE
	para->dram_tpr13 &= ~(0x7<<2);
	para->dram_tpr13 |= (2<<2);
#endif

#ifdef DRAMC_USE_1T_MODE
	para->dram_tpr13 &= ~(0x1<<5);
	para->dram_tpr13 |= (0x1<<5);
#endif

#ifdef DRAMC_USE_2T_MODE
	para->dram_tpr13 &= ~(0x1<<5);
#endif

#ifdef DRAMC_DX1_DX2_SWAP_TEST
	/*half DQ*/
	para->dram_para2 &= ~(0xf<<0);
	para->dram_para2 |= (0x1<<0);
	/*page size half*/
	ret = (para->dram_para1&(0xf<<16))>>1;
	para->dram_para1 &=~(0xf<<16) ;
	para->dram_para1 |=ret ;

	ret = (para->dram_para1&(0xf<<0))>>1;
	para->dram_para1 &=~(0xf<<0) ;
	para->dram_para1 |=ret ;
#endif

#ifdef DRAMC_HALF_DQ_TEST
	/*use 1t*/
	para->dram_tpr13 &= ~(0x1<<5);
	para->dram_tpr13 |= (0x1<<5);
	/*half DQ*/
	para->dram_para2 &= ~(0xf<<0);
	para->dram_para2 |= (0x1<<0);
	/*page size half*/
	ret = (para->dram_para1&(0xf<<16))>>1;
	para->dram_para1 &=~(0xf<<16) ;
	para->dram_para1 |=ret ;

	ret = (para->dram_para1&(0xf<<0))>>1;
	para->dram_para1 &=~(0xf<<0) ;
	para->dram_para1 |=ret ;
#endif
#ifdef DRAMC_MDFS_CFS_TEST
	para->dram_tpr13 |= (0x1<<10);
#endif
	para->dram_tpr13 |= (0x1<<0);
}

//***********************************************************************************************
//	void auto_set_timing_para(__dram_para_t *para)
//
//  Description:	auto set the timing para base on the DRAM Frequency in structure
//
//	Arguments:		DRAM parameter
//
//	Return Value:	None
//***********************************************************************************************
#ifndef FPGA_PLATFORM
void auto_set_timing_para(__dram_para_t *para)
{
	unsigned int  ctrl_freq;//half speed mode :ctrl_freq=1/2 ddr_fre
	unsigned int  type;
	unsigned int  reg_val        =0;
	unsigned int  tdinit0       = 0;
	unsigned int  tdinit1       = 0;
	unsigned int  tdinit2       = 0;
	unsigned int  tdinit3       = 0;
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
	unsigned char twr			= 8;	//15£»
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
		dram_dbg("User define timing parameter!\n");
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
	}{//add finish
		if(type==3){
#ifdef DRAM_TYPE_DDR3
			//dram_tpr0
			tccd=2;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns;
			trrd=(10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trrd<4) trrd=4;	//max(4ck,10ns)
			trcd= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);//15ns(10ns)
			trc	= (53*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns
			//dram_tpr1
			txp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(txp<3) txp = 3;//max(3ck,7.5ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(twtr<4) twtr=4;	//max(4ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(trtp<4) trtp=4;	//max(4ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);	//15ns;
			if(twr<3) twr=3;
			trp = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);//15ns(10ns)
			tras= (38*ctrl_freq)/1000 + ( ( ((35*ctrl_freq)%1000) != 0) ? 1 :0);	//38ns;
			//dram_tpr2
			trefi	= ( (7800*ctrl_freq)/1000 + ( ( ((7800*ctrl_freq)%1000) != 0) ? 1 :0) )/32;//7800ns
			trfc = (350*ctrl_freq)/1000 + ( ( ((350*ctrl_freq)%1000) != 0) ? 1 :0);	//350ns;
#endif
		}else if(type==2)
		{
#ifdef	DRAM_TYPE_DDR2
			tccd=1;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns;
			trrd= (10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);	//10ns;
			trcd= (20*ctrl_freq)/1000 + ( ( ((20*ctrl_freq)%1000) != 0) ? 1 :0);	//20ns(10ns)
			//trc	= (65*ctrl_freq)/1000 + ( ( ((65*ctrl_freq)%1000) != 0) ? 1 :0);	//65ns
			//dram_tpr1
			txp	= 2;		//2nclk;
			twtr = (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);		//7.5ns;
			trtp = (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);		//7.5ns;
			twr = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);	//15ns;
			trp = (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);	//15ns(10ns)
			tras= (45*ctrl_freq)/1000 + ( ( ((45*ctrl_freq)%1000) != 0) ? 1 :0);	//45ns;
			trc	= trp + tras;	//65ns
			//dram_tpr2
			trefi = ((7800*ctrl_freq)/1000 + ( ( ((7800*ctrl_freq)%1000) != 0) ? 1 :0) )/32;//7800ns
			trfc = (328*ctrl_freq)/1000 + ( ( ((328*ctrl_freq)%1000) != 0) ? 1 :0);		//328ns;
#endif
		}else if(type==6)
		{
#ifdef	DRAM_TYPE_LPDDR2
			tccd=1;
			tfaw	= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns;
			if(tfaw<4) tfaw	= 4;
			trrd	= (10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);	//10ns;
			if(trrd<2) trrd	= 2;
			trcd	= (24*ctrl_freq)/1000 + ( ( ((24*ctrl_freq)%1000) != 0) ? 1 :0);	//24ns;
			if(trcd<2) trcd	= 2;
			trc	= (70*ctrl_freq)/1000 + ( ( ((70*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns
			//dram_tpr1
			txp	= (10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(txp<2) txp = 2;//max(2ck,10ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(twtr<2) twtr=2;	//max(2ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(trtp<2) trtp=2;	//max(2ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);	//15ns;
			if(twr<3) twr=3;
			trp = (27*ctrl_freq)/1000 + ( ( ((27*ctrl_freq)%1000) != 0) ? 1 :0);//15ns(10ns)
			if(trp<2) trp=2;
			tras= (42*ctrl_freq)/1000 + ( ( ((42*ctrl_freq)%1000) != 0) ? 1 :0);	//38ns;
			//dram_tpr2
			trefi	= ( (3900*ctrl_freq)/1000 + ( ( ((3900*ctrl_freq)%1000) != 0) ? 1 :0) )/32;//7800ns
			trfc = (210*ctrl_freq)/1000 + ( ( ((210*ctrl_freq)%1000) != 0) ? 1 :0);	//210ns;
#endif
		}else if(type==7)
		{
#ifdef	DRAM_TYPE_LPDDR3
			tccd=2;
			tfaw= (50*ctrl_freq)/1000 + ( ( ((50*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns;
			if(tfaw<4) tfaw	= 4;
			trrd=(10*ctrl_freq)/1000 + ( ( ((10*ctrl_freq)%1000) != 0) ? 1 :0);
			if(trrd<2) trrd=2;	//max(4ck,10ns)
			trcd= (24*ctrl_freq)/1000 + ( ( ((24*ctrl_freq)%1000) != 0) ? 1 :0);//15ns(10ns)
			if(trcd<2) trcd	= 2;
			trc	= (70*ctrl_freq)/1000 + ( ( ((70*ctrl_freq)%1000) != 0) ? 1 :0);	//50ns
			//dram_tpr1
			txp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(txp<2) txp = 2;//max(3ck,7.5ns)
			twtr= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(twtr<2) twtr=2;	//max(4ck,7,5ns)
			trtp	= (8*ctrl_freq)/1000 + ( ( ((8*ctrl_freq)%1000) != 0) ? 1 :0);	//7.5ns;
			if(trtp<2) trtp=2;	//max(4ck,7.5ns)
			twr= (15*ctrl_freq)/1000 + ( ( ((15*ctrl_freq)%1000) != 0) ? 1 :0);	//15ns;
			if(twr<3) twr=3;
			trp = (27*ctrl_freq)/1000 + ( ( ((27*ctrl_freq)%1000) != 0) ? 1 :0);//15ns(10ns)
			if(trp<2) trp=2;
			tras= (42*ctrl_freq)/1000 + ( ( ((42*ctrl_freq)%1000) != 0) ? 1 :0);	//38ns;
			//dram_tpr2
			trefi	= ( (3900*ctrl_freq)/1000 + ( ( ((3900*ctrl_freq)%1000) != 0) ? 1 :0) )/32;//7800ns
			trfc = (210*ctrl_freq)/1000 + ( ( ((210*ctrl_freq)%1000) != 0) ? 1 :0);	//350ns;
#endif
		}
		//assign the value back to the DRAM structure
		para->dram_tpr0 = (trc<<0) | (trcd<<6) | (trrd<<11) | (tfaw<<15) | (tccd<<21) ;
		//dram_dbg("para_dram_tpr0 = %x\n",para->dram_tpr0);
		para->dram_tpr1 = (tras<<0) | (trp<<6) | (twr<<11) | (trtp<<15) | (twtr<<20)|(txp<<23);
		//dram_dbg("para_dram_tpr1 = %x\n",para->dram_tpr1);
		para->dram_tpr2 = (trefi<<0) | (trfc<<12);
		//dram_dbg("para_dram_tpr2 = %x\n",para->dram_tpr2);
	}
	switch(type){
	case 2://DDR2
#ifdef DRAM_TYPE_DDR2
		//the time we no need to calculate
		tmrw=0x0;
		tmrd=0x2;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=tcke + 1;
		trasmax =0x1b;

		tcl	= 3;	//CL   12
		tcwl = 3;	//CWL  8
		t_rdata_en  =1;
		wr_latency  =1;
		para->dram_mr0 	= 0x263;// WR=2,CL=6,BL=8
		para->dram_mr2  = 0x0; //ODT disable
		para->dram_mr3  = 0;

		tdinit0	= (400*para->dram_clk) + 1;	//400us
		tdinit1	= (500*para->dram_clk)/1000 + 1;//500ns
		tdinit2	= (200*para->dram_clk) + 1;	//200us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp=tcwl+2+twr;//WL+BL/2+tWR
		twr2rd= tcwl+2+twtr;//WL+BL/2+tWTR
		trd2wr= tcl+2+1-tcwl;//RL+BL/2+2-WL
		dram_dbg("tcl = %d,tcwl = %d\n",tcl,tcwl);
#endif
		break;
	case 3://DDR3
#ifdef DRAM_TYPE_DDR3
		//the time we no need to calculate
		tmrw=0x0;
		tmrd=0x4;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=4;
		trasmax =0x18;

		tcl		= 6;	//CL   12
		tcwl	= 4;	//CWL  8
		t_rdata_en  =4;
		wr_latency  =2;
		para->dram_mr0 	= 0x1c70;//CL=11,wr 12
		para->dram_mr2  = 0x18; //CWL=8,800M
		para->dram_mr3  = 0;

		tdinit0	= (500*para->dram_clk) + 1;	//500us
		tdinit1	= (400*para->dram_clk)/1000 + 1;//360ns
		tdinit2	= (200*para->dram_clk) + 1;	//200us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twr = (para->dram_mr0>>9) & 0x7;
		switch(twr)
		{
			case 0:	twr = 16;	break;
			case 1:	twr = 5;	break;
			case 2:	twr = 6;	break;
			case 3:	twr = 7;	break;
			case 4:	twr = 8;	break;
			case 5:	twr = 10;	break;
			case 6:	twr = 12;	break;
			case 7:	twr = 14;	break;
			default:twr = 16;	break;
		}
		twr = twr/2 + (((twr%2) != 0) ? 1 : 0);	//tWR/2;
		twtp=tcwl+2+twr;//WL+BL/2+tWR
		twr2rd= tcwl+2+twtr;//WL+BL/2+tWTR
		trd2wr= tcl+2+1-tcwl;//RL+BL/2+2-WL
		dram_dbg("tcl = %d,tcwl = %d\n",tcl,tcwl);
#endif
		break;
	case 6 ://LPDDR2
#ifdef	DRAM_TYPE_LPDDR2
		tmrw=0x3;
		tmrd=0x5;
		tmod=0x5;
		tcke=2;
		tcksrx=5;
		tcksre=5;
		tckesr=5;
		trasmax =0x18;
		//according to frequency
		tcl		= 4;
		tcwl	= 2;
		t_rdata_en  =3;    //if tcl odd,(tcl-3)/2;  if tcl even ,((tcl+1)-3)/2
		wr_latency  =1;
		para->dram_mr0 = 0;
		para->dram_mr1 = 0xc3;//twr=8;bl=8
		para->dram_mr2 = 0x6;//RL=8,CWL=4

		//end
		tdinit0	= (200*para->dram_clk) + 1;	//200us
		tdinit1	= (100*para->dram_clk)/1000 + 1;	//100ns
		tdinit2	= (11*para->dram_clk) + 1;	//11us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp	= tcwl + 2 + twr + 1;	// CWL+BL/2+tWR
		trd2wr	= tcl + 2 + 5 - tcwl + 1;//5?
		twr2rd	= tcwl + 2 + 1 + twtr;//wl+BL/2+1+tWTR??
#endif
		break;
	case 7 ://LPDDR3
#ifdef	DRAM_TYPE_LPDDR3
		tmrw=0x5;
		tmrd=0x5;
		tmod=0xc;
		tcke=3;
		tcksrx=5;
		tcksre=5;
		tckesr=5;
		trasmax =0x18;
		//according to clock
		tcl		= 6;
		tcwl	= 3;
//		tcwl	= 2; //for simulation
		t_rdata_en  =5;    //if tcl odd,(tcl-3)/2;  if tcl even ,((tcl+1)-3)/2
		wr_latency  =2;
//		wr_latency  =1;	//for lp3 simulation only
		para->dram_mr0 = 0;
		para->dram_mr1 = 0xc3;//twr=8;bl=8
		para->dram_mr2 = 0xa;//RL=12,CWL=6
//		para->dram_mr2 = 0x6;//RL=8,CWL=4	//for simulation limit,wl=4 only
		//end
		tdinit0	= (200*para->dram_clk) + 1;	//200us
		tdinit1	= (100*para->dram_clk)/1000 + 1;	//100ns
		tdinit2	= (11*para->dram_clk) + 1;	//11us
		tdinit3	= (1*para->dram_clk) + 1;	//1us
		twtp	= tcwl + 4 + twr + 1;	// CWL+BL/2+tWR
		trd2wr	= tcl + 4 + 5 - tcwl + 1;	//13;
		twr2rd	= tcwl + 4 + 1 + twtr;
#endif
		break;
	default:
		break;
	}
	//set work mode register before training,include 1t/2t DDR type,BL,rank number
	reg_val=mctl_read_w(MC_WORK_MODE);
	reg_val &=~((0xfff<<12)|(0xf<<0));
	reg_val|=(0x4<<20); //LPDDR2/LPDDR3/ddr3 all use BL8
	reg_val |= ((para->dram_type & 0x7)<<16);//DRAM type
	reg_val |= (( ( (para->dram_para2) & 0x1 )? 0x0:0x1) << 12);	//DQ width
	reg_val |= ( (para->dram_para2)>>12 & 0x03 );	//rank
	reg_val |= ((((para->dram_para1)>>28) & 0x01) << 2);//BANK
	if((para->dram_type==6)||(para->dram_type==7))
		reg_val |= (0x1U<<19);  //LPDDR2/3 must use 1T mode
	else
		reg_val |= (((para->dram_tpr13>>5)&0x1)<<19);//2T or 1T
	mctl_write_w(reg_val,MC_WORK_MODE);

#ifdef DRAMC_DX1_DX2_SWAP_TEST
	reg_val=mctl_read_w(MC_WORK_MODE);
	reg_val |=(0x1<<24);
	mctl_write_w(reg_val,MC_WORK_MODE);
#endif
	//set mode register
	mctl_write_w((para->dram_mr0),DRAM_MR0);
	mctl_write_w((para->dram_mr1),DRAM_MR1);
	mctl_write_w((para->dram_mr2),DRAM_MR2);
	mctl_write_w((para->dram_mr3),DRAM_MR3);
	if((para->dram_type == 6)||(para->dram_type == 7)){
		mctl_write_w(((para->dram_odt_en)>>4)&0x3,LP3MR11);
	}
	//set dram timing
	reg_val= (twtp<<24)|(tfaw<<16)|(trasmax<<8)|(tras<<0);
	//dram_dbg("DRAM TIMING PARA0 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG0);//DRAMTMG0
	reg_val= (txp<<16)|(trtp<<8)|(trc<<0);
	//dram_dbg("DRAM TIMING PARA1 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG1);//DRAMTMG1
	reg_val= (tcwl<<24)|(tcl<<16)|(trd2wr<<8)|(twr2rd<<0);
	//dram_dbg("DRAM TIMING PARA2 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG2);//DRAMTMG2
	reg_val= (tmrw<<16)|(tmrd<<12)|(tmod<<0);
	//dram_dbg("DRAM TIMING PARA3 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG3);//DRAMTMG3
	reg_val= (trcd<<24)|(tccd<<16)|(trrd<<8)|(trp<<0);
	//dram_dbg("DRAM TIMING PARA4 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG4);//DRAMTMG4
	reg_val= (tcksrx<<24)|(tcksre<<16)|(tckesr<<8)|(tcke<<0);
	//dram_dbg("DRAM TIMING PARA5 = %x\n",reg_val);
	mctl_write_w(reg_val,DRAMTMG5);//DRAMTMG5
	//set two rank timing
	reg_val= mctl_read_w(DRAMTMG8);
	reg_val&=~(0xff<<8);
	reg_val&=~(0xff<<0);
	reg_val|=(0x66<<8);//for lp,ddr3 0x33
	reg_val|=(0x10<<0);
	mctl_write_w(reg_val,DRAMTMG8);//DRAMTMG8
	//dram_dbg("DRAM TIMING PARA8 = %x\n",reg_val);
	//set phy interface time
	reg_val=(0x2<<24)|(t_rdata_en<<16)|(0x1<<8)|(wr_latency<<0);
	//dram_dbg("DRAM PHY INTERFACE PARA = %x\n",reg_val);
	mctl_write_w(reg_val,PITMG0);	//PHY interface write latency and read latency configure
	//set phy time  PTR0-2 use default
//	mctl_write_w(((tdinit0<<0)|(tdinit1<<20)),PTR3);
//	mctl_write_w(((tdinit2<<0)|(tdinit3<<20)),PTR4);
	/*only for simulation*/
	mctl_write_w(0x01e007c3,PTR0);
#ifdef DRAMC_MR_TEST
	mctl_write_w(0x05C00800,PTR3);
#else
	mctl_write_w(0x03C00800,PTR3);
#endif
	mctl_write_w(0x24000500,PTR4);
	//set refresh timing
    reg_val =(trefi<<16)|(trfc<<0);
    mctl_write_w(reg_val,RFSHTMG);
}
void auto_set_dram_para(__dram_para_t *para)
{
	unsigned int ret_val=2;
	ret_val=para->dram_type;
#ifdef SYSTEM_SIMULATION
	switch(ret_val)
	{
		case 2: //ddr2
#ifdef DRAM_TYPE_DDR2
			para->dram_zq			= 0x3b3bbb;
			para->dram_odt_en       = 1;
			para->dram_para1		= 0x10D410D4;
			para->dram_para2		= 0x1000;//half dq 1001;default 1000
			para->dram_mr0			= 0xa63;
			para->dram_mr1			= 0x0;
			para->dram_mr2			= 0x0;
			para->dram_mr3			= 0x0;
			para->dram_tpr0 		= 0x04911063;
			para->dram_tpr1 		= 0x00211089;
			para->dram_tpr2 		= 0x0428003A;
			para->dram_tpr3 		= 0x0AD3A981;
			para->dram_tpr4 		= 0x1E117701;
			para->dram_tpr5          = 0;
			para->dram_tpr6          = 0;
			para->dram_tpr7          = 0;
			para->dram_tpr8          = 0;
			para->dram_tpr9          = 0;
			para->dram_tpr10       	 = 0;
			para->dram_tpr13       	 = 0x1;
#endif
			break;
		case 3: //ddr3
#ifdef DRAM_TYPE_DDR3
			para->dram_zq			= 0x3b3bbb;
			para->dram_odt_en       = 1;
			para->dram_para1		= 0x10F81000;//0x11082000 A20E A15 IS CS1..should set A0~A14
			para->dram_para2		= 0x1000;
			para->dram_mr0			= 0x840;
			para->dram_mr1			= 0x0;
			para->dram_mr2			= 0x8;
			para->dram_mr3			= 0x0;
			para->dram_tpr0 		= 0x04911064;
			para->dram_tpr1 		= 0x00211089;
			para->dram_tpr2 		= 0x0828003A;
			para->dram_tpr3 		= 0x0AD3A981;
			para->dram_tpr4 		= 0x1E117701;
			para->dram_tpr5          = 0;
			para->dram_tpr6          = 0;
			para->dram_tpr7          = 0;
			para->dram_tpr8          = 0;
			para->dram_tpr9          = 0;
			para->dram_tpr10       	 = 0;
			para->dram_tpr13       	 = 0x1;
#endif
			break;
		case 6:
#ifdef DRAM_TYPE_LPDDR2
			para->dram_zq			= 0x3b3bbb;
			para->dram_odt_en       = 1;
			para->dram_para1		= 0x10F410F4;//according to design
			para->dram_para2		= 0x1000;
			para->dram_mr0			= 0x0;
			para->dram_mr1			= 0xc3;
			para->dram_mr2			= 0x6;
			para->dram_mr3			= 0x2;
			para->dram_tpr0 		= 0x02C19844;
			para->dram_tpr1 		= 0x002110EB;
			para->dram_tpr2 		= 0x0C28001D;
			para->dram_tpr3 		= 0x03117701;
			para->dram_tpr4 		= 0x1E1014A1;
			para->dram_tpr5          = 0;
			para->dram_tpr6          = 0;
			para->dram_tpr7          = 0;
			para->dram_tpr8          = 0;
			para->dram_tpr9          = 0;
			para->dram_tpr10       	 = 0;
			para->dram_tpr13       	 = 0x21;
#endif
			break;
		case 7:
#ifdef DRAM_TYPE_LPDDR3
			para->dram_zq			= 0x3b3bbb;
			para->dram_odt_en       = 1;
			para->dram_para1		= 0x10E410E4;//according to design
			para->dram_para2		= 0x1000;
			para->dram_mr0			= 0x0;
			para->dram_mr1			= 0xc3;
			para->dram_mr2			= 0x6;
			para->dram_mr3			= 0x2;
			para->dram_tpr0 		= 0x04C19844;
			para->dram_tpr1 		= 0x002110EB;
			para->dram_tpr2 		= 0x0C28001D;
			para->dram_tpr3 		= 0x03117701;
			para->dram_tpr4 		= 0x1E1014A1;
			para->dram_tpr5          = 0;
			para->dram_tpr6          = 0;
			para->dram_tpr7          = 0;
			para->dram_tpr8          = 0;
			para->dram_tpr9          = 0;
			para->dram_tpr10       	 = 0;
			para->dram_tpr13       	 = 0x21;
#endif
			break;
		default:
			break;
	}
#else //IC verify parameter
	switch(ret_val)
	{
		case 2:
			break
		case 3:
			break
		case 6:
			break
		case 7:
			break
		default:
			break;
	}
#endif
}
#endif
//***********************************************************************************************
//	unsigned int ccm_set_dram_div(u32 div)
//
//  Description:	set-dram-div
//
//	Return Value:	None
//***********************************************************************************************
#ifndef FPGA_PLATFORM
//***********************************************************************************************
//	unsigned int ccm_set_pll_ddr_clk(u32 pll_clk)
//
//  Description:	set-pll-ddr0-clk
//
//	Return Value:	None
//***********************************************************************************************

unsigned int ccm_set_pll_ddr0_clk(u32 pll_clk)
{
	unsigned int n, k, m = 1,rval;
	unsigned int div;
	unsigned int mod2, mod3;
	unsigned int min_mod = 0;

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
	local_delay(20);
	return 24 * n * k / m;
}



unsigned int ccm_set_pll_ddr1_clk(u32 pll_clk)
{
	u32 rval;
	u32 div;

	div = pll_clk/24;
	if (div < 12) //no less than 288M
	{
		div=12;
	}
	rval = mctl_read_w(_CCM_PLL_DDR1_REG);
	rval &= ~(0x3f<<8);
	rval |=(((div-1)<<8)|(0x1U<<31));
	mctl_write_w(rval, _CCM_PLL_DDR1_REG);
	mctl_write_w(rval|(1U << 30), _CCM_PLL_DDR1_REG);
	local_delay(1);
	return 24*div;
}

#endif
//***********************************************************************************************
//	unsigned int mctl_sys_init(__dram_para_t *para)
//
//  Description:	set pll and dram clk
//
//	Arguments:		DRAM parameter
//
//	Return Value:	None
//***********************************************************************************************
unsigned int mctl_sys_init(__dram_para_t *para)
{
	unsigned int reg_val = 0;
	unsigned int ret_val = 0;
	//trun off mbus clk gate
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val &=~(1U<<31);
	writel(reg_val, MBUS_CLK_CTL_REG);
	//mbus reset
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val &=~(1U<<31);
	writel(reg_val, MBUS_RESET_REG);
	// DISABLE DRAMC BUS GATING
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val &= ~(1U<<14);
	writel(reg_val, BUS_CLK_GATE_REG0);
	//DRAM BUS reset
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val &= ~(1U<<14);
	writel(reg_val, BUS_RST_REG0);
	//controller reset
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U<<31);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	local_delay(10);
	//set PLL source and divider
#if defined DRAMC_MDFS_DFS_TEST || defined DRAMC_HDR_CLK_TEST
	para->dram_tpr9 = 360;
	ccm_set_pll_ddr0_clk(para->dram_tpr9<<1);
#endif
	if((para->dram_tpr13>>6)&0x1)
	{
		ret_val = ccm_set_pll_ddr1_clk(para->dram_clk<<1);
		reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
		reg_val &=~(0x3<<20);
		reg_val |= (0x1<<20);
		mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
		reg_val |= (0x1<<16);
		mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	}else{
		ret_val = ccm_set_pll_ddr0_clk(para->dram_clk<<1);
		reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
		reg_val &=~(0x3<<20);
		mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
		reg_val |= (0x1<<16);
		mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	}
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1<<0);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	reg_val |= (0x1<<16);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);


#ifdef  DRAMC_MDFS_CFS_TEST
	/*set pk_2_pk 0.4 ,step 10*/
	writel(0x8C80000A, _CCM_PLL_DDR1_CFG_REG);
	local_delay(100);
	if((para->dram_tpr13>>6)&0x1)
	{
		reg_val = mctl_read_w(_CCM_PLL_DDR1_REG);
		reg_val |= (1<<30);	//update for valid CFS mode
		mctl_write_w(reg_val, _CCM_PLL_DDR1_REG);
	}else{
		reg_val = mctl_read_w(_CCM_PLL_DDR0_REG);
		reg_val |= (1<<30);	//update for valid CFS mode
		mctl_write_w(reg_val, _CCM_PLL_DDR0_REG);
	}
	local_delay(100);
#endif
	//release DRAM ahb BUS RESET
	reg_val = mctl_read_w(BUS_RST_REG0);
	reg_val |= (1U<<14);
	writel(reg_val, BUS_RST_REG0);
	//open AHB gating
	reg_val = mctl_read_w(BUS_CLK_GATE_REG0);
	reg_val |= (1U<<14);
	writel(reg_val, BUS_CLK_GATE_REG0);
	//release DRAM mbus RESET
	reg_val = mctl_read_w(MBUS_RESET_REG);
	reg_val |=(1U<<31);
	writel(reg_val, MBUS_RESET_REG);
	//open mbus gating
	reg_val = mctl_read_w(MBUS_CLK_CTL_REG);
	reg_val |=(1U<<31);
	writel(reg_val, MBUS_CLK_CTL_REG);

	//release the controller reset
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val |= 0x1U<<31;
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);
	local_delay(100);

#ifdef DRAMC_REFRESH_TEST
	//set to one rank mode before global clock enable for refresh test,liuke edit 20140902
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val &= ~(0x1<<0);
	mctl_write_w(reg_val,MC_WORK_MODE);
	//enable dramc clk
	mctl_write_w(0x8000,CLKEN);	//--------------------------------------------------------------------------------------
	para->dram_para2 |= 1<<12;
#else
	mctl_write_w(0x8000,CLKEN);
#endif

	return DRAM_RET_OK;
}
//***********************************************************************************************
//	void mctl_com_init(__dram_para_t *para)
//
//  Description:	set ddr para and enable clk
//
//	Arguments:		DRAM parameter
//
//	Return Value:	None
//***********************************************************************************************
void mctl_com_init(__dram_para_t *para)
{
	unsigned int reg_val,ret_val;
	unsigned int m=0,rank_num=1;
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val &= ~(0xfff000);
	if(para->dram_type==6)
		reg_val |=(0x4<<20);
	else
		reg_val |=(0x4<<20);
	reg_val |= ((para->dram_type & 0x07)<<16);//DRAM type
	reg_val |= (( ( (para->dram_para2) & 0x01 )? 0x0:0x1) << 12);	//DQ width
	reg_val |= (((para->dram_tpr13>>5)&0x1)<<19);//2T or 1T
	mctl_write_w(reg_val,MC_WORK_MODE);

	if(para->dram_para2 & (0x1<<8))
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
	dram_dbg("MC_WORK_MODE is %x\n",mctl_read_w(MC_WORK_MODE));
	if(para->dram_para2 & (0x1<<8))
	dram_dbg("MC_WORK_MODE_rank1 is %x\n",mctl_read_w(MC_R1_WORK_MODE));

	//set DRAM ODT MAP according to rank number
	reg_val = (mctl_read_w(MC_WORK_MODE)&0x1);
	if(reg_val)
		ret_val = 0x303;
	else
		ret_val = 0x201;
	mctl_write_w(ret_val,ODTMAP);
	dram_dbg("ODTMAP is %x\n",ret_val);

	/*half DQ must disable high DQ group*/
	if((para->dram_para2)&0x01)
	{
#ifdef DRAMC_DX1_DX2_SWAP_TEST
		mctl_write_w(0,DXnGCR0(1));
		mctl_write_w(0,DXnGCR0(3));
#else
		mctl_write_w(0,DXnGCR0(2));
		mctl_write_w(0,DXnGCR0(3));
#endif
	}
}

unsigned int mctl_channel_init(unsigned int ch_index,__dram_para_t *para)
{
	unsigned int reg_val = 0,ret_val,pad_hold_flag,value;
	unsigned int i = 0 ;
	unsigned int dqs_gating_mode =0;
	dqs_gating_mode = (para->dram_tpr13>>2)&0x7;
	auto_set_timing_para(para);

	/*1.setting timer base on MBUS CLK*/
	reg_val=mctl_read_w(MC_TMR);
	reg_val &= ~(0xfff<<0);
	reg_val |= 399;
	mctl_write_w(reg_val,MC_TMR);
	/*select dphy/aphy/dout phase*/
	reg_val = mctl_read_w(PGCR2);
	reg_val &= ~(0x3<<10);
	reg_val |= 0x0<<10;
	reg_val &= ~(0x3<<8);
	reg_val |= 0x3<<8;
	mctl_write_w(reg_val,PGCR2);
	/*3.setting DX and CA */
	ret_val = (~(para->dram_odt_en))&0x1;
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

	reg_val = mctl_read_w(ACIOCR0);
	//add for psram
	reg_val &=~(0x1<<11);
	reg_val |= (0x1<<1);
	mctl_write_w(reg_val,ACIOCR0);
//****************************************************************************************************//
	/*4.setting bit delay*/
	/*5.setting DQS mode */
	switch(dqs_gating_mode)
	{
		case 1://open DQS gating
			reg_val = mctl_read_w(PGCR2);
			reg_val &= ~(0x3<<6);
			mctl_write_w(reg_val,PGCR2);

			reg_val = mctl_read_w(DQSGMR);
			reg_val &= ~((0x1<<8) | 0x7);
			mctl_write_w(reg_val,DQSGMR);

			break;
		case 2://auto gating pull up
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
			dram_dbg("DRAM DQS gate is PU mode.\n");
			break;
		default:
			//close DQS gating--auto gating pull down
			//for aw1680 standby problem,reset gate
			reg_val = mctl_read_w(PGCR2);
			reg_val &= ~(0x1<<6);
			mctl_write_w(reg_val,PGCR2);

			reg_val = mctl_read_w(PGCR2);
			reg_val |= (0x3<<6);
			mctl_write_w(reg_val,PGCR2);
			dram_dbg("DRAM DQS gate is PD mode.\n");
			break;
	}
	/*6.for lpddr2/lpddr3,if use gqs gate,can extend the dqs gate after training*/
	if((para->dram_type) == 6 || (para->dram_type) == 7)
	{
		if(dqs_gating_mode==1)//open dqs gate and need dqs training
		{
			//this mdoe ,dqs pull down ,dqs# pull up,use extend 0 to training
			reg_val =mctl_read_w(DXCCR);
			reg_val |=(0x1U<<31);
			reg_val &=~(0x1<<27);
			reg_val &=~(0x3<<6);
			mctl_write_w(reg_val,DXCCR);
		}
		else
		{
			//add for lpddr2,change pull up/down register to 1.25k
			reg_val =mctl_read_w(DXCCR);
			reg_val &=~(0x7U<<28);
			reg_val &=~(0x7U<<24);
			reg_val |= (0x2U<<28);
			reg_val |= (0x2U<<24);
			mctl_write_w(reg_val,DXCCR);
		}
	}
//****************************************************************************************************//
	/*8.data training configuration */
	if((para->dram_para2>>12)&0x1)
	{
		reg_val=mctl_read_w(DTCR);
		reg_val&=(0xfU<<28);
		reg_val|=0x03000081;
		mctl_write_w(reg_val,DTCR);
	}
	else
	{
		reg_val=mctl_read_w(DTCR);
		reg_val&=(0xfU<<28);
		reg_val|=0x01000081;
		mctl_write_w(reg_val,DTCR);
	}

#ifdef  DRAMC_TIMING_TEST
	//DRAMTMG3 max timing test
	reg_val=mctl_read_w(DRAMTMG3);
	reg_val|=( (0x3ff)|(0x7<<12)|(0x3ff<<16)  );
	mctl_write_w(reg_val,DRAMTMG3);
#endif

	pad_hold_flag = (mctl_read_w(VDD_SYS_PWROFF_GATING) & 0x1);
	if(pad_hold_flag ==0)
	{
		pattern_goto(0x10);
		/*8.set ZQ parameter DX ODT/DX DRIVER /CK DRIVER/CA DRIVER*/
		reg_val = mctl_read_w(ZQCR);
		reg_val &= ~(0x00ffffff);
		reg_val |= ( (para->dram_zq) & 0xffffff );
		mctl_write_w(reg_val,ZQCR);

		/*9.trigger initial ,ZQ calibration ,training*/
		if(dqs_gating_mode == 1)
		{
			reg_val = 0x52;
			mctl_write_w(reg_val,PIR );
			reg_val |= (0x1<<0);
			mctl_write_w(reg_val,PIR );
			dram_dbg("DRAM initial PIR value is %x\n",reg_val);
			while((mctl_read_w(PGSR0 )&0x1) != 0x1);

			reg_val = 0x520; //DDL CAL;DRAM INIT;gate training
			if((para->dram_type) == 3)
				reg_val |= 0x1u<<7; //DDR3 RST
		}
		else
		{
			reg_val = 0x172 ;
			if((para->dram_type) == 3)
				reg_val |= 0x1u<<7; //DDR3 RST
		}
		mctl_write_w(reg_val,PIR );
		reg_val |= (0x1<<0);
		mctl_write_w(reg_val,PIR );
	}else{
#ifdef DRAMC_SUPER_STANDBY_TEST
	/* ZQ pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= (~( 0x1<<1));
	mctl_write_w( reg_val,VDD_SYS_PWROFF_GATING);

	reg_val = mctl_read_w(ZQCR);
	reg_val &= ~(0x00ffffff);
	reg_val |= ( (para->dram_zq) & 0xffffff );
	mctl_write_w(reg_val,ZQCR);
	/* training :DQS gate training */
	if(dqs_gating_mode == 1)
	{
		reg_val = 0x52; //zq calibration;PHY RST;phy init before DDL calibration
		mctl_write_w(reg_val,PIR );
		reg_val |= (0x1<<0);
		mctl_write_w(reg_val,PIR );
		while((mctl_read_w(PGSR0 )&0x1) != 0x1);
		reg_val = 0x20; //DDL CAL;
	}
	else
	{
		reg_val = 0x62;
	}
	mctl_write_w(reg_val,PIR );
	reg_val |= (0x1<<0);
	mctl_write_w(reg_val,PIR );
	while((mctl_read_w(PGSR0 )&0x1) != 0x1);

	reg_val = mctl_read_w(PGCR3);//MCTRL cmd
	reg_val &= (~(0x3<<25));
	reg_val |=(0x2<<25);
	mctl_write_w( reg_val,PGCR3 );

	/* entry self-refresh */
	reg_val = mctl_read_w(PWRCTL);
	reg_val |= 0x1<<0;
	mctl_write_w( reg_val, PWRCTL );
	//confirm dram controller has entry from selfrefresh
	while(((mctl_read_w(STATR) & 0x7) != 0x3));

	/* pad release */
	reg_val = mctl_read_w(VDD_SYS_PWROFF_GATING);
	reg_val &= ~( 0x1<<0);
	mctl_write_w( reg_val,VDD_SYS_PWROFF_GATING);

	/* exit self-refresh but no enable all master access */
	reg_val = mctl_read_w(PWRCTL);
	reg_val &= ~(0x1<<0);
	mctl_write_w(reg_val,PWRCTL);
	//confirm dram controller has exit from selfrefresh
	while(((mctl_read_w(STATR ) & 0x7) != 0x1))	;

	if (dqs_gating_mode == 1)
	{
		reg_val = mctl_read_w(PGCR2);
		reg_val &= ~(0x3<<6);
		mctl_write_w(reg_val,PGCR2);

		reg_val = mctl_read_w(PGCR3);//PHY cmd
		reg_val &= (~(0x3<<25));
		reg_val |=(0x1<<25);
		mctl_write_w( reg_val,PGCR3 );

		reg_val =0x401;
		mctl_write_w(reg_val,PIR);
		while((mctl_read_w(PGSR0 )&0x1) != 0x1);
	}
#endif
}
	while((mctl_read_w(PGSR0 )&0x1) != 0x1);
	reg_val = mctl_read_w(PGSR0);
	if((reg_val>>20)&0xff){
		return 0;
	}
	/*10.after initial before write or read must clear credit value*/
	reg_val = mctl_read_w(MC_CCCR);
	reg_val |= (0x1U)<<31;
	mctl_write_w(reg_val,MC_CCCR);
	while((mctl_read_w(STATR )&0x1) != 0x1);
	/*11.PHY choose to update PHY or command mode */
	reg_val = mctl_read_w(PGCR3);
	reg_val &= ~(0x3<<25);
	mctl_write_w(reg_val, PGCR3);

	/*12.refresh update,from AW1680/1681 */
	reg_val = mctl_read_w(RFSHCTL0);
	reg_val|=(0x1U)<<31;
	mctl_write_w(reg_val,RFSHCTL0);

	reg_val = mctl_read_w(RFSHCTL0);
	reg_val&=~(0x1U<<31);
	mctl_write_w(reg_val,RFSHCTL0);

	/*13.for lpddr2/lpddr3,if use gqs gate,can extend the dqs gate after training */
	if(dqs_gating_mode==1)//open dqs gate and need dqs training
	{
		//this mdoe ,dqs pull down ,dqs# pull up,extend 1
		reg_val =mctl_read_w(DXCCR);
		reg_val &=~(0x3<<6);
		if((para->dram_type) == 6 || (para->dram_type) == 7)
		{
			reg_val |= (0x1<<6);
		}
		if((para->dram_type) == 3)
		{
			reg_val |= (0x0<<6);
		}
		mctl_write_w(reg_val,DXCCR);
	}
	//enable master access
	mctl_write_w(0xffffffff,MC_MAER);
	return (1);
}



#ifndef SYSTEM_SIMULATION


void auto_detect_rank_dq(__dram_para_t *para)
{
	unsigned int reg_val;
	unsigned int byte,col;
	//-----------------------------------detect dq-------------------------------------------
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val|=(0x1<<12);//full DQ mode--32bit
	mctl_write_w(reg_val,MC_WORK_MODE);

	reg_val = mctl_read_w(PGCR0);
	reg_val|=(0x1<<25);//enable read Time-out
	mctl_write_w(reg_val,PGCR0);
	local_delay(10);

	mctl_write_w(0x12345678,0x40000000);

	if(mctl_read_w(0x40000000)==0)
	{
		paraconfig(&(para->dram_para2), 0xfU<<0, 1);//half DQ
	}
	else
	{
		paraconfig(&(para->dram_para2), 0xfU<<0, 0);//half DQ
	}

	reg_val = mctl_read_w(PGCR0);
	reg_val&=~(0x1<<26);//reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);
	local_delay(10);
	reg_val|=(0x1<<26);//clear reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);

	//-----------------------------------detect rank-------------------------------------------
	reg_val = mctl_read_w(MC_WORK_MODE);
	reg_val |= (( ( (para->dram_para2) & 0x01 )? 0x0:0x1) << 12);	//DQ width
	reg_val |=(0x1<<15);//Row, Rank, Bank, Column
	reg_val |=(0x1);//two rank
	paraconfig(&reg_val,0xf<<8,0x6<<8);//page_size is 512B
	mctl_write_w(reg_val,MC_WORK_MODE);

	if((para->dram_para2)&0xf==1)
	{
		byte=1;//half dq--16 bit
	}
	else
	{
		byte=2;//full dq--32bit
	}

	col=(byte)?((byte-1)?7:8):9 ;

	mctl_write_w(0x12345678,0x40000000+(0x1<<(col+byte+3)) );//write rank 1

	if(mctl_read_w(0x40000000+(0x1<<(col+byte+3)))==0)
	{
		paraconfig(&(para->dram_para2), 0xfU<<12, 0<<12);//one rank
	}
	else
	{
		paraconfig(&(para->dram_para2), 0xfU<<12, 1<<12);//two rank
	}

	reg_val = mctl_read_w(PGCR0);
	reg_val&=~(0x1<<26);//reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);
	local_delay(10);
	reg_val&=~(0x1<<25);//disable read Time-out
	reg_val|=(0x1<<26);//clear reset PHY FIFO
	mctl_write_w(reg_val,PGCR0);
	return;

}

void auto_detect_dram_size(__dram_para_t *para)
{

	unsigned int i=0,j=0;
	unsigned int reg_val=0,ret=0,cnt=0;
	for(i=0;i<64;i++)
	{
		mctl_write_w((i%2)?(0x40000000 + 4*i):(~(0x40000000 + 4*i)),0x40000000 + 4*i);
	}
	reg_val=mctl_read_w(MC_WORK_MODE);
	paraconfig(&reg_val,0xf<<8,0x6<<8);//512B
	reg_val|=(0x1<<15);
	reg_val|=(0xf<<4);//16 row
	mctl_write_w(reg_val,MC_WORK_MODE);
	//row detect
	for(i=11;i<=16;i++)
	{
		ret = 0x40000000 + (1<<(i+9));//row-column
		cnt = 0;
		for(j=0;j<64;j++)
		{
			if(mctl_read_w(0x40000000 + j*4) == mctl_read_w(ret + j*4))
			{
				cnt++;
			}
		}
		if(cnt == 64)
		{
			break;
		}
	}
	if(i >= 16)
		i = 16;
	paraconfig(&(para->dram_para1), 0xffU<<20, i<<20);//row width confirmed
	//pagesize(column)detect
	reg_val=mctl_read_w(MC_WORK_MODE);
	paraconfig(&reg_val,0xf<<4,0xa<<4);//11rows
	paraconfig(&reg_val,0xf<<8,0xa<<8);//8KB
	mctl_write_w(reg_val,MC_WORK_MODE);
	for(i=9;i<=13;i++)
	{
		ret = 0x40000000 + (0x1U<<i);//column
		cnt = 0;
		for(j=0;j<64;j++)
		{
			if(mctl_read_w(0x40000000 + j*4) == mctl_read_w(ret + j*4))
			{
				cnt++;
			}
		}
		if(cnt == 64)
		{
			break;
		}
	}
	if(i >= 13)
		i = 13;
	if(i==9)
		i = 0;
	else
		i = (0x1U<<(i-10));
	paraconfig(&(para->dram_para1), 0xfU<<16, i<<16);//pagesize confirmed

}
#endif
/*
**********************************************************************************************************************
*                                               GET DRAM SIZE
*
* Description: Get DRAM Size in MB unit;
*
* Arguments  : None
*
* Returns    : 32/64/128/...
*
* Notes      :
*
**********************************************************************************************************************
*/
unsigned int DRAMC_get_dram_size()
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
			dram_size1 = 1<< dram_size1;
		}
		else
			dram_size1 = dram_size0;
	}
	return (dram_size0 + dram_size1);
}

//*****************************************************************************
//	signed int init_DRAM(int type)
//  Description:	System init dram
//
//	Arguments:		type:	0: no lock		1: get the fixed parameters & auto detect & lock
//
//	Return Value:	0: fail
//					others: pass
//*****************************************************************************
signed int init_DRAM(int type, __dram_para_t *para)
{
	unsigned int ret_val=0;
	mctl_sys_init(para);
#if defined DRAMC_COM_REG_TEST || defined DRAMC_CTR_PRO_REG_TEST  || defined DRAMC_RAM_BIST
	return 0;
#endif
	ret_val=mctl_channel_init(0,para);
	if(ret_val==0)
		return 0;
	mctl_com_init(para);
	ret_val= DRAMC_get_dram_size();
	para->dram_para2 &= ~(0xffffU<<16);
	para->dram_para1|=(ret_val<<16);
	return ret_val;
}

//*****************************************************************************
//	unsigned int mctl_init()

//  Description:	for simulation and FPGA
//
//	Arguments:		None
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
unsigned int mctl_init(void)
{
	signed int ret_val=0;
	__dram_para_t dram_para;
	//set the parameter
#if defined DRAMC_MASTER_TEST && defined DRAM_TYPE_DDR3
	dram_para.dram_clk			= 796;
#else
	dram_para.dram_clk			= 480;
#endif
#if defined DRAM_TYPE_DDR3
	dram_para.dram_type			= 3;
#elif defined DRAM_TYPE_DDR2
	dram_para.dram_type			= 2;
#elif defined DRAM_TYPE_LPDDR2
	dram_para.dram_type			= 6;
#else
	dram_para.dram_type			= 7;
#endif
	auto_set_dram_para(&dram_para);
	auto_set_test_para(&dram_para);
	ret_val = init_DRAM(0, &dram_para);
#ifndef DRAMC_SUPER_STANDBY_TEST
	ret_val = dramc_basic_test();
#endif

	return ret_val;
}
#else
//***********************************************************************************************
//	unsigned int mctl_sys_init(__dram_para_t *para)
//
//  Description:	set pll and dram clk
//
//	Arguments:		DRAM parameter
//
//	Return Value:	None
//***********************************************************************************************
unsigned int mctl_sys_init(__dram_para_t *para)
{

	unsigned int reg_val = 0;
	reg_val = mctl_read_w(_CCM_DRAMCLK_CFG_REG);
	reg_val &= ~(0x1U<<31);
	mctl_write_w(reg_val,_CCM_DRAMCLK_CFG_REG);

	mctl_write_w(0x25ffff,CLKEN);

	return 0;
}
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
	reg_val = mctl_read_w(MC_CLKEN);
	reg_val |= (0x3<<20);
	mctl_write_w(reg_val,MC_CLKEN);
#endif


	reg_val = 0x000183;		//PLL enable, PLL6 should be dram_clk/2
	mctl_write_w(reg_val,PIR);	//for fast simulation
	while((mctl_read_w(PGSR0 )&0x1) != 0x1);	//for fast simulation
	while((mctl_read_w(STATR )&0x1) != 0x1);	//init done
//	para->dram_tpr13&=~(0x1<<0);//to detect DRAM space

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

/*
**********************************************************************************************************************
*                                               GET DRAM SIZE
*
* Description: Get DRAM Size in MB unit;
*
* Arguments  : None
*
* Returns    : 32/64/128/...
*
* Notes      :
*
**********************************************************************************************************************
*/
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



//*****************************************************************************
//	signed int init_DRAM(int type)
//  Description:	System init dram
//
//	Arguments:		type:	0: no lock		1: get the fixed parameters & auto detect & lock
//
//	Return Value:	0: fail
//					others: pass
//*****************************************************************************
signed int init_DRAM(int type, __dram_para_t *para)
{
	unsigned int ret_val=0;
	mctl_sys_init(para);
#ifdef FPGA_REGISTER_TEST
	return 0;
#endif
	ret_val=mctl_channel_init(0,para);
	if(ret_val==0)
		return 0;
	ret_val = DRAMC_get_dram_size();
	return ret_val;
}

//*****************************************************************************
//	unsigned int mctl_init()

//  Description:	FOR SD SIMULATION
//
//	Arguments:		None
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
unsigned int mctl_init(void *para)
{
	signed int ret_val=0;

	__dram_para_t dram_para;

	ret_val = init_DRAM(0, &dram_para);
	if(para != NULL)
	{
		memcpy(para,&dram_para, sizeof(dram_para) );
	}
//	ret_val = dramc_fpga_test();
	return ret_val;
}


#endif


