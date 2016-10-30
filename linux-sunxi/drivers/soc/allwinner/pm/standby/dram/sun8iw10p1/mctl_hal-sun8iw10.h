#ifndef _MCTL_HAL_H
#define  _MCTL_HAL_H
/**********************
��֤ƽ̨�궨��
1.FPGA_VERIFY --- FPGA��֤��
2.IC_VERIFY --- IC��֤��
3.SYSTEM_VERIFY --- ����ʹ�ô�
**********************/
/*#define FPGA_VERIFY*/
/*#define IC_VERIFY*/
#define SYSTEM_VERIFY

#ifdef SYSTEM_VERIFY
/*boot/standby�ⲿͷ�ļ�
��SW��Ա���
*/

#endif
/**********************
STANDBY�궨��
standby���Ѳ���boot����
�Ŀ���ʹ�ܸú���ٱ�����
**********************/
/*#define CPUS_STANDBY_CODE*/
/**********************
���ܿ�궨��
**********************/
#ifndef CPUS_STANDBY_CODE
/*#define USE_PMU*/
/*#define USE_CHIPID*/
/*#define USE_SPECIAL_DRAM*/
/*#define USE_CHEAP_DRAM*/
#endif

/**********************
�Զ����ú궨��
**********************/
#ifndef CPUS_STANDBY_CODE
/*#define DRAM_AUTO_SCAN*/
#ifdef DRAM_AUTO_SCAN
#define DRAM_TYPE_SCAN
#define DRAM_RANK_SCAN
#define DRAM_SIZE_SCAN

#endif
#endif
/**********************
��ӡ�ȼ��궨��
**********************/
#define DEBUG_LEVEL_0
/*#define DEBUG_LEVEL_1*/
/*#define DEBUG_LEVEL_4*/
/*#define DEBUG_LEVEL_8*/
/*#define TIMING_DEBUG*/
/*#define ERROR_DEBUG*/
/*#define AUTO_DEBUG*/

#if defined DEBUG_LEVEL_8
#define dram_dbg_8(fmt, args...)	 printk(fmt, ##args)
#define dram_dbg_4(fmt, args...)	 printk(fmt, ##args)
#define dram_dbg_1(fmt, args...)  printk(fmt, ##args)
#define dram_dbg_0(fmt, args...)  printk(fmt, ##args)
#elif defined DEBUG_LEVEL_4
#define dram_dbg_8(fmt, args...)
#define dram_dbg_4(fmt, args...)  printk(fmt, ##args)
#define dram_dbg_1(fmt, args...)  printk(fmt, ##args)
#define dram_dbg_0(fmt, args...)  printk(fmt, ##args)
#elif defined DEBUG_LEVEL_1
#define dram_dbg_8(fmt, args...)
#define dram_dbg_4(fmt, args...)
#define dram_dbg_1(fmt, args...)  printk(fmt, ##args)
#define dram_dbg_0(fmt, args...)  printk(fmt, ##args)
#elif defined DEBUG_LEVEL_0
#define dram_dbg_8(fmt, args...)
#define dram_dbg_4(fmt, args...)
#define dram_dbg_1(fmt, args...)
#define dram_dbg_0(fmt, args...)  printk(fmt, ##args)
#else
#define dram_dbg_8(fmt, args...)
#define dram_dbg_4(fmt, args...)
#define dram_dbg_1(fmt, args...)
#define dram_dbg_0(fmt, args...)
#endif

#if defined TIMING_DEBUG
#define dram_dbg_timing(fmt, args...)  printk(fmt, ##args)
#else
#define dram_dbg_timing(fmt, args...)
#endif

#if defined ERROR_DEBUG
#define dram_dbg_error(fmt, args...)  printk(fmt, ##args)
#else
#define dram_dbg_error(fmt, args...)
#endif

#if defined AUTO_DEBUG
#define dram_dbg_auto(fmt, args...)  printk(fmt, ##args)
#else
#define dram_dbg_auto(fmt, args...)
#endif


#endif	/*_MCTL_HAL_H*/
