#ifndef   _MCTL_HAL_H
#define   _MCTL_HAL_H
/**********************
验证平台宏定义.FPGA_VERIFY --- FPGA验证打开
2.IC_VERIFY --- IC验证打开
3.SYSTEM_VERIFY --- 驱动使用打开
**********************/
//#define FPGA_VERIFY
//#define IC_VERIFY
#define SYSTEM_VERIFY

#ifdef SYSTEM_VERIFY
/*boot/standby外部头文件由SW人员添加
*/
#include <common.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/timer.h>
#include <asm/arch/ccmu.h>
#endif

/**********************
STANDBY宏定义standby唤醒不走boot流程
的可以使能该宏减少编译量
**********************/
//#define CPUS_STANDBY_CODE
/**********************
功能块宏定义
**********************/
#ifndef CPUS_STANDBY_CODE
#define USE_PMU
//#define USE_CHIPID
//#define USE_SPECIAL_DRAM
//#define USE_CHEAP_DRAM
#endif

/**********************
自动配置宏定义**********************/
#ifndef CPUS_STANDBY_CODE
#define DRAM_AUTO_SCAN
#ifdef DRAM_AUTO_SCAN
//#define DRAM_TYPE_SCAN
#define DRAM_RANK_SCAN
#define DRAM_SIZE_SCAN


#endif
#endif
/**********************
打印等级宏定义**********************/
#define DEBUG_LEVEL_1
//#define DEBUG_LEVEL_4
//#define DEBUG_LEVEL_8
//#define TIMING_DEBUG
#define ERROR_DEBUG
//#define AUTO_DEBUG

#if defined DEBUG_LEVEL_8
#define dram_dbg_8(fmt,args...)	 printf(fmt ,##args)
#define dram_dbg_4(fmt,args...)	 printf(fmt ,##args)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#elif defined DEBUG_LEVEL_4
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)  printf(fmt ,##args)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#elif defined DEBUG_LEVEL_1
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)
#define dram_dbg_0(fmt,args...)
#endif

#if defined TIMING_DEBUG
#define dram_dbg_timing(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_timing(fmt,args...)
#endif

#if defined ERROR_DEBUG
#define dram_dbg_error(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_error(fmt,args...)
#endif


#if defined AUTO_DEBUG
#define dram_dbg_auto(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_auto(fmt,args...)
#endif


typedef struct __DRAM_PARA
{
	//normal configuration
	unsigned int        dram_clk;
	//dram_type		DDR2:2 DDR3:3 LPDDR2:6 LPDDR3:7 DDR3L:31
	unsigned int        dram_type;
	//unsigned int        lpddr2_type;	//LPDDR2 type		S4:0	S2:1 	NVM:2
	unsigned int        dram_zq;		//do not need
	unsigned int		dram_odt_en;

	//control configuration
	unsigned int		dram_para1;
	unsigned int		dram_para2;

	//timing configuration
	unsigned int		dram_mr0;
	unsigned int		dram_mr1;
	unsigned int		dram_mr2;
	unsigned int		dram_mr3;
	unsigned int		dram_tpr0;	//DRAMTMG0
	unsigned int		dram_tpr1;	//DRAMTMG1
	unsigned int		dram_tpr2;	//DRAMTMG2
	unsigned int		dram_tpr3;	//DRAMTMG3
	unsigned int		dram_tpr4;	//DRAMTMG4
	unsigned int		dram_tpr5;	//DRAMTMG5
	unsigned int		dram_tpr6;	//DRAMTMG8

	//reserved for future use
	unsigned int		dram_tpr7;
	unsigned int		dram_tpr8;
	unsigned int		dram_tpr9;
	unsigned int		dram_tpr10;
	unsigned int		dram_tpr11;
	unsigned int		dram_tpr12;
	unsigned int		dram_tpr13;
}__dram_para_t;

extern unsigned int mctl_core_init(__dram_para_t *para);
extern unsigned int mctl_init(void);
extern signed int init_DRAM(int type, __dram_para_t *para);
#endif  //_MCTL_HAL_H
