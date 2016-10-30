
#ifndef   _MCTL_HAL_H   
#define   _MCTL_HAL_H

/*=====================================
 *ѡ����ģʽ
 * ====================================
 */
/* DRAM type define */
#define   DRAM_TYPE_DDR3
//#define   DRAM_TYPE_LPDDR2
//#define   DRAM_TYPE_LPDDR3
//#define   DRAM_TYPE_DDR2
/*ʱ��Դ��DQSģʽ��1T/2Tģʽ����pattern�������*/
#define SOURCE_CLK_USE_DDR_PLL1
//#define SOURCE_CLK_USE_DDR_PLL0

#define DRAMC_USE_AUTO_DQS_GATE_PD_MODE
//#define DRAMC_USE_DQS_GATING_MODE
//#define DRAMC_USE_AUTO_DQS_GATE_PU_MODE


#if defined DRAM_TYPE_LPDDR2 || defined DRAM_TYPE_LPDDR3
#define DRAMC_USE_1T_MODE
#else
#define DRAMC_USE_2T_MODE
#endif

#ifdef SYSTEM_SIMULATION
#define dram_dbg(fmt,args...)
#else
#define dram_dbg(fmt,args...)	printf(fmt ,##args)
#endif


#endif  //_MCTL_HAL_H
