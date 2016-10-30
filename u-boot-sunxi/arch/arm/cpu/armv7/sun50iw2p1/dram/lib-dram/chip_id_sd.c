//*****************************************************************************
//	Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
//
//	File: 				chip_id.c
//
//	Description:  This file implements basic functions for AW1673 DRAM controller
//
//	History:
//				2015/03/23		YSZ			0.10	Initial version
//*****************************************************************************

//***********************************************************************************************
//	unsigned short crc_16_sd(unsigned short reg, unsigned char data_crc) 
//
//  Description:	CRC16 coding
//
//	Arguments:		16bit disturb_code, 8bit id_num. 
//								id_num is the label of the bonding_id, but the value of id_num and bonding_id may be different
//
//	Return Value:	CRC16 result
//***********************************************************************************************
#include "chip_id_sd.h"

extern unsigned int id_judge_fun(unsigned short disturb_code);

unsigned short crc_16_sd(unsigned short reg, unsigned char data_crc) 
//reg is crc register,data_crc is 8bit data stream 
{ 
	unsigned short msb;
	unsigned short data; 
	unsigned short gx = 0x8005, i = 0; //i: the num of left shift, gx:polynomial 
	
	data = (unsigned short)data_crc; 
	data = data << 8; 
	reg = reg ^ data; 
	
	do 
	{ 
		msb = reg & 0x8000; 
		reg = reg << 1; 
		if(msb == 0x8000) 
		{
			reg = reg ^ gx; 
		} 
		i++; 
	}
	while(i < 8); 
	
	return (reg); 
}

//***********************************************************************************************
//	unsigned int disturb_coding_sd(unsigned short disturb_code,unsigned char id_num)
//
//  Description:	generate a scrambling code depend on the data input
//
//	Arguments:		16bit disturb_code, 8bit id_num. 
//								id_num is the label of the bonding_id, but the value of id_num and bonding_id may be different
//
//	Return Value:	scrambling code
//***********************************************************************************************
unsigned int disturb_coding_sd(unsigned short disturb_code,unsigned char id_num)//
{
	unsigned int crc_result = 0;
	unsigned int disturb_result = 0;
	
	crc_result = crc_16_sd(disturb_code,id_num);
	
	disturb_result = ( (crc_result << 16) | (disturb_code + id_num) );
	
	return disturb_result;
}

////***********************************************************************************************
////	void disturb_coding_test(void)
////
////  Description:	test pattern of disturb_coding_sd function
////
////	Arguments:		NONE
////
////	Return Value:	NONE
////***********************************************************************************************
//void disturb_coding_test(void)
//{
//		unsigned short i=0;
//		unsigned char j=0;
//		unsigned int disturb_result=0;
//		
//		for(i=0;i<16;i++)
//		{
//			for(j=1;j<17;j++)
//			{
//				disturb_result = disturb_coding_sd(i,j);
//				printk("disturb=0x%x  id_num==0x%x  disturb_result=disturb_result\n",i,j,disturb_result);
//			}	
//		}
//}
///*test result
//disturb=0x0  id_num==0x1  disturb_result=0x80050001
//disturb=0x0  id_num==0x2  disturb_result=0x800f0002
//disturb=0x0  id_num==0x3  disturb_result=0xa0003
//disturb=0x0  id_num==0x4  disturb_result=0x801b0004
//disturb=0x0  id_num==0x5  disturb_result=0x1e0005
//disturb=0x0  id_num==0x6  disturb_result=0x140006
//disturb=0x0  id_num==0x7  disturb_result=0x80110007
//disturb=0x0  id_num==0x8  disturb_result=0x80330008
//disturb=0x0  id_num==0x9  disturb_result=0x360009
//disturb=0x0  id_num==0xa  disturb_result=0x3c000a
//disturb=0x0  id_num==0xb  disturb_result=0x8039000b
//disturb=0x0  id_num==0xc  disturb_result=0x28000c
//disturb=0x0  id_num==0xd  disturb_result=0x802d000d
//disturb=0x0  id_num==0xe  disturb_result=0x8027000e
//disturb=0x0  id_num==0xf  disturb_result=0x22000f
//disturb=0x0  id_num==0x10  disturb_result=0x80630010
//disturb=0x1  id_num==0x1  disturb_result=0x81050002
//disturb=0x1  id_num==0x2  disturb_result=0x810f0003
//disturb=0x1  id_num==0x3  disturb_result=0x10a0004
//disturb=0x1  id_num==0x4  disturb_result=0x811b0005
//disturb=0x1  id_num==0x5  disturb_result=0x11e0006
//disturb=0x1  id_num==0x6  disturb_result=0x1140007
//disturb=0x1  id_num==0x7  disturb_result=0x81110008
//disturb=0x1  id_num==0x8  disturb_result=0x81330009
//disturb=0x1  id_num==0x9  disturb_result=0x136000a
//disturb=0x1  id_num==0xa  disturb_result=0x13c000b
//disturb=0x1  id_num==0xb  disturb_result=0x8139000c
//disturb=0x1  id_num==0xc  disturb_result=0x128000d
//disturb=0x1  id_num==0xd  disturb_result=0x812d000e
//disturb=0x1  id_num==0xe  disturb_result=0x8127000f
//disturb=0x1  id_num==0xf  disturb_result=0x1220010
//disturb=0x1  id_num==0x10  disturb_result=0x81630011
//disturb=0x2  id_num==0x1  disturb_result=0x82050003
//disturb=0x2  id_num==0x2  disturb_result=0x820f0004
//disturb=0x2  id_num==0x3  disturb_result=0x20a0005
//disturb=0x2  id_num==0x4  disturb_result=0x821b0006
//disturb=0x2  id_num==0x5  disturb_result=0x21e0007
//disturb=0x2  id_num==0x6  disturb_result=0x2140008
//disturb=0x2  id_num==0x7  disturb_result=0x82110009
//disturb=0x2  id_num==0x8  disturb_result=0x8233000a
//disturb=0x2  id_num==0x9  disturb_result=0x236000b
//disturb=0x2  id_num==0xa  disturb_result=0x23c000c
//disturb=0x2  id_num==0xb  disturb_result=0x8239000d
//disturb=0x2  id_num==0xc  disturb_result=0x228000e
//disturb=0x2  id_num==0xd  disturb_result=0x822d000f
//disturb=0x2  id_num==0xe  disturb_result=0x82270010
//disturb=0x2  id_num==0xf  disturb_result=0x2220011
//disturb=0x2  id_num==0x10  disturb_result=0x82630012
//disturb=0x3  id_num==0x1  disturb_result=0x83050004
//disturb=0x3  id_num==0x2  disturb_result=0x830f0005
//disturb=0x3  id_num==0x3  disturb_result=0x30a0006
//disturb=0x3  id_num==0x4  disturb_result=0x831b0007
//disturb=0x3  id_num==0x5  disturb_result=0x31e0008
//disturb=0x3  id_num==0x6  disturb_result=0x3140009
//disturb=0x3  id_num==0x7  disturb_result=0x8311000a
//disturb=0x3  id_num==0x8  disturb_result=0x8333000b
//disturb=0x3  id_num==0x9  disturb_result=0x336000c
//disturb=0x3  id_num==0xa  disturb_result=0x33c000d
//disturb=0x3  id_num==0xb  disturb_result=0x8339000e
//disturb=0x3  id_num==0xc  disturb_result=0x328000f
//disturb=0x3  id_num==0xd  disturb_result=0x832d0010
//disturb=0x3  id_num==0xe  disturb_result=0x83270011
//disturb=0x3  id_num==0xf  disturb_result=0x3220012
//disturb=0x3  id_num==0x10  disturb_result=0x83630013
//disturb=0x4  id_num==0x1  disturb_result=0x84050005
//disturb=0x4  id_num==0x2  disturb_result=0x840f0006
//disturb=0x4  id_num==0x3  disturb_result=0x40a0007
//disturb=0x4  id_num==0x4  disturb_result=0x841b0008
//disturb=0x4  id_num==0x5  disturb_result=0x41e0009
//disturb=0x4  id_num==0x6  disturb_result=0x414000a
//disturb=0x4  id_num==0x7  disturb_result=0x8411000b
//disturb=0x4  id_num==0x8  disturb_result=0x8433000c
//disturb=0x4  id_num==0x9  disturb_result=0x436000d
//disturb=0x4  id_num==0xa  disturb_result=0x43c000e
//disturb=0x4  id_num==0xb  disturb_result=0x8439000f
//disturb=0x4  id_num==0xc  disturb_result=0x4280010
//disturb=0x4  id_num==0xd  disturb_result=0x842d0011
//disturb=0x4  id_num==0xe  disturb_result=0x84270012
//disturb=0x4  id_num==0xf  disturb_result=0x4220013
//disturb=0x4  id_num==0x10  disturb_result=0x84630014
//disturb=0x5  id_num==0x1  disturb_result=0x85050006
//disturb=0x5  id_num==0x2  disturb_result=0x850f0007
//disturb=0x5  id_num==0x3  disturb_result=0x50a0008
//disturb=0x5  id_num==0x4  disturb_result=0x851b0009
//disturb=0x5  id_num==0x5  disturb_result=0x51e000a
//disturb=0x5  id_num==0x6  disturb_result=0x514000b
//disturb=0x5  id_num==0x7  disturb_result=0x8511000c
//disturb=0x5  id_num==0x8  disturb_result=0x8533000d
//disturb=0x5  id_num==0x9  disturb_result=0x536000e
//disturb=0x5  id_num==0xa  disturb_result=0x53c000f
//disturb=0x5  id_num==0xb  disturb_result=0x85390010
//disturb=0x5  id_num==0xc  disturb_result=0x5280011
//disturb=0x5  id_num==0xd  disturb_result=0x852d0012
//disturb=0x5  id_num==0xe  disturb_result=0x85270013
//disturb=0x5  id_num==0xf  disturb_result=0x5220014
//disturb=0x5  id_num==0x10  disturb_result=0x85630015
//disturb=0x6  id_num==0x1  disturb_result=0x86050007
//disturb=0x6  id_num==0x2  disturb_result=0x860f0008
//disturb=0x6  id_num==0x3  disturb_result=0x60a0009
//disturb=0x6  id_num==0x4  disturb_result=0x861b000a
//disturb=0x6  id_num==0x5  disturb_result=0x61e000b
//disturb=0x6  id_num==0x6  disturb_result=0x614000c
//disturb=0x6  id_num==0x7  disturb_result=0x8611000d
//disturb=0x6  id_num==0x8  disturb_result=0x8633000e
//disturb=0x6  id_num==0x9  disturb_result=0x636000f
//disturb=0x6  id_num==0xa  disturb_result=0x63c0010
//disturb=0x6  id_num==0xb  disturb_result=0x86390011
//disturb=0x6  id_num==0xc  disturb_result=0x6280012
//disturb=0x6  id_num==0xd  disturb_result=0x862d0013
//disturb=0x6  id_num==0xe  disturb_result=0x86270014
//disturb=0x6  id_num==0xf  disturb_result=0x6220015
//disturb=0x6  id_num==0x10  disturb_result=0x86630016
//disturb=0x7  id_num==0x1  disturb_result=0x87050008
//disturb=0x7  id_num==0x2  disturb_result=0x870f0009
//disturb=0x7  id_num==0x3  disturb_result=0x70a000a
//disturb=0x7  id_num==0x4  disturb_result=0x871b000b
//disturb=0x7  id_num==0x5  disturb_result=0x71e000c
//disturb=0x7  id_num==0x6  disturb_result=0x714000d
//disturb=0x7  id_num==0x7  disturb_result=0x8711000e
//disturb=0x7  id_num==0x8  disturb_result=0x8733000f
//disturb=0x7  id_num==0x9  disturb_result=0x7360010
//disturb=0x7  id_num==0xa  disturb_result=0x73c0011
//disturb=0x7  id_num==0xb  disturb_result=0x87390012
//disturb=0x7  id_num==0xc  disturb_result=0x7280013
//disturb=0x7  id_num==0xd  disturb_result=0x872d0014
//disturb=0x7  id_num==0xe  disturb_result=0x87270015
//disturb=0x7  id_num==0xf  disturb_result=0x7220016
//disturb=0x7  id_num==0x10  disturb_result=0x87630017
//disturb=0x8  id_num==0x1  disturb_result=0x88050009
//disturb=0x8  id_num==0x2  disturb_result=0x880f000a
//disturb=0x8  id_num==0x3  disturb_result=0x80a000b
//disturb=0x8  id_num==0x4  disturb_result=0x881b000c
//disturb=0x8  id_num==0x5  disturb_result=0x81e000d
//disturb=0x8  id_num==0x6  disturb_result=0x814000e
//disturb=0x8  id_num==0x7  disturb_result=0x8811000f
//disturb=0x8  id_num==0x8  disturb_result=0x88330010
//disturb=0x8  id_num==0x9  disturb_result=0x8360011
//disturb=0x8  id_num==0xa  disturb_result=0x83c0012
//disturb=0x8  id_num==0xb  disturb_result=0x88390013
//disturb=0x8  id_num==0xc  disturb_result=0x8280014
//disturb=0x8  id_num==0xd  disturb_result=0x882d0015
//disturb=0x8  id_num==0xe  disturb_result=0x88270016
//disturb=0x8  id_num==0xf  disturb_result=0x8220017
//disturb=0x8  id_num==0x10  disturb_result=0x88630018
//disturb=0x9  id_num==0x1  disturb_result=0x8905000a
//disturb=0x9  id_num==0x2  disturb_result=0x890f000b
//disturb=0x9  id_num==0x3  disturb_result=0x90a000c
//disturb=0x9  id_num==0x4  disturb_result=0x891b000d
//disturb=0x9  id_num==0x5  disturb_result=0x91e000e
//disturb=0x9  id_num==0x6  disturb_result=0x914000f
//disturb=0x9  id_num==0x7  disturb_result=0x89110010
//disturb=0x9  id_num==0x8  disturb_result=0x89330011
//disturb=0x9  id_num==0x9  disturb_result=0x9360012
//disturb=0x9  id_num==0xa  disturb_result=0x93c0013
//disturb=0x9  id_num==0xb  disturb_result=0x89390014
//disturb=0x9  id_num==0xc  disturb_result=0x9280015
//disturb=0x9  id_num==0xd  disturb_result=0x892d0016
//disturb=0x9  id_num==0xe  disturb_result=0x89270017
//disturb=0x9  id_num==0xf  disturb_result=0x9220018
//disturb=0x9  id_num==0x10  disturb_result=0x89630019
//disturb=0xa  id_num==0x1  disturb_result=0x8a05000b
//disturb=0xa  id_num==0x2  disturb_result=0x8a0f000c
//disturb=0xa  id_num==0x3  disturb_result=0xa0a000d
//disturb=0xa  id_num==0x4  disturb_result=0x8a1b000e
//disturb=0xa  id_num==0x5  disturb_result=0xa1e000f
//disturb=0xa  id_num==0x6  disturb_result=0xa140010
//disturb=0xa  id_num==0x7  disturb_result=0x8a110011
//disturb=0xa  id_num==0x8  disturb_result=0x8a330012
//disturb=0xa  id_num==0x9  disturb_result=0xa360013
//disturb=0xa  id_num==0xa  disturb_result=0xa3c0014
//disturb=0xa  id_num==0xb  disturb_result=0x8a390015
//disturb=0xa  id_num==0xc  disturb_result=0xa280016
//disturb=0xa  id_num==0xd  disturb_result=0x8a2d0017
//disturb=0xa  id_num==0xe  disturb_result=0x8a270018
//disturb=0xa  id_num==0xf  disturb_result=0xa220019
//disturb=0xa  id_num==0x10  disturb_result=0x8a63001a
//disturb=0xb  id_num==0x1  disturb_result=0x8b05000c
//disturb=0xb  id_num==0x2  disturb_result=0x8b0f000d
//disturb=0xb  id_num==0x3  disturb_result=0xb0a000e
//disturb=0xb  id_num==0x4  disturb_result=0x8b1b000f
//disturb=0xb  id_num==0x5  disturb_result=0xb1e0010
//disturb=0xb  id_num==0x6  disturb_result=0xb140011
//disturb=0xb  id_num==0x7  disturb_result=0x8b110012
//disturb=0xb  id_num==0x8  disturb_result=0x8b330013
//disturb=0xb  id_num==0x9  disturb_result=0xb360014
//disturb=0xb  id_num==0xa  disturb_result=0xb3c0015
//disturb=0xb  id_num==0xb  disturb_result=0x8b390016
//disturb=0xb  id_num==0xc  disturb_result=0xb280017
//disturb=0xb  id_num==0xd  disturb_result=0x8b2d0018
//disturb=0xb  id_num==0xe  disturb_result=0x8b270019
//disturb=0xb  id_num==0xf  disturb_result=0xb22001a
//disturb=0xb  id_num==0x10  disturb_result=0x8b63001b
//disturb=0xc  id_num==0x1  disturb_result=0x8c05000d
//disturb=0xc  id_num==0x2  disturb_result=0x8c0f000e
//disturb=0xc  id_num==0x3  disturb_result=0xc0a000f
//disturb=0xc  id_num==0x4  disturb_result=0x8c1b0010
//disturb=0xc  id_num==0x5  disturb_result=0xc1e0011
//disturb=0xc  id_num==0x6  disturb_result=0xc140012
//disturb=0xc  id_num==0x7  disturb_result=0x8c110013
//disturb=0xc  id_num==0x8  disturb_result=0x8c330014
//disturb=0xc  id_num==0x9  disturb_result=0xc360015
//disturb=0xc  id_num==0xa  disturb_result=0xc3c0016
//disturb=0xc  id_num==0xb  disturb_result=0x8c390017
//disturb=0xc  id_num==0xc  disturb_result=0xc280018
//disturb=0xc  id_num==0xd  disturb_result=0x8c2d0019
//disturb=0xc  id_num==0xe  disturb_result=0x8c27001a
//disturb=0xc  id_num==0xf  disturb_result=0xc22001b
//disturb=0xc  id_num==0x10  disturb_result=0x8c63001c
//disturb=0xd  id_num==0x1  disturb_result=0x8d05000e
//disturb=0xd  id_num==0x2  disturb_result=0x8d0f000f
//disturb=0xd  id_num==0x3  disturb_result=0xd0a0010
//disturb=0xd  id_num==0x4  disturb_result=0x8d1b0011
//disturb=0xd  id_num==0x5  disturb_result=0xd1e0012
//disturb=0xd  id_num==0x6  disturb_result=0xd140013
//disturb=0xd  id_num==0x7  disturb_result=0x8d110014
//disturb=0xd  id_num==0x8  disturb_result=0x8d330015
//disturb=0xd  id_num==0x9  disturb_result=0xd360016
//disturb=0xd  id_num==0xa  disturb_result=0xd3c0017
//disturb=0xd  id_num==0xb  disturb_result=0x8d390018
//disturb=0xd  id_num==0xc  disturb_result=0xd280019
//disturb=0xd  id_num==0xd  disturb_result=0x8d2d001a
//disturb=0xd  id_num==0xe  disturb_result=0x8d27001b
//disturb=0xd  id_num==0xf  disturb_result=0xd22001c
//disturb=0xd  id_num==0x10  disturb_result=0x8d63001d
//disturb=0xe  id_num==0x1  disturb_result=0x8e05000f
//disturb=0xe  id_num==0x2  disturb_result=0x8e0f0010
//disturb=0xe  id_num==0x3  disturb_result=0xe0a0011
//disturb=0xe  id_num==0x4  disturb_result=0x8e1b0012
//disturb=0xe  id_num==0x5  disturb_result=0xe1e0013
//disturb=0xe  id_num==0x6  disturb_result=0xe140014
//disturb=0xe  id_num==0x7  disturb_result=0x8e110015
//disturb=0xe  id_num==0x8  disturb_result=0x8e330016
//disturb=0xe  id_num==0x9  disturb_result=0xe360017
//disturb=0xe  id_num==0xa  disturb_result=0xe3c0018
//disturb=0xe  id_num==0xb  disturb_result=0x8e390019
//disturb=0xe  id_num==0xc  disturb_result=0xe28001a
//disturb=0xe  id_num==0xd  disturb_result=0x8e2d001b
//disturb=0xe  id_num==0xe  disturb_result=0x8e27001c
//disturb=0xe  id_num==0xf  disturb_result=0xe22001d
//disturb=0xe  id_num==0x10  disturb_result=0x8e63001e
//disturb=0xf  id_num==0x1  disturb_result=0x8f050010
//disturb=0xf  id_num==0x2  disturb_result=0x8f0f0011
//disturb=0xf  id_num==0x3  disturb_result=0xf0a0012
//disturb=0xf  id_num==0x4  disturb_result=0x8f1b0013
//disturb=0xf  id_num==0x5  disturb_result=0xf1e0014
//disturb=0xf  id_num==0x6  disturb_result=0xf140015
//disturb=0xf  id_num==0x7  disturb_result=0x8f110016
//disturb=0xf  id_num==0x8  disturb_result=0x8f330017
//disturb=0xf  id_num==0x9  disturb_result=0xf360018
//disturb=0xf  id_num==0xa  disturb_result=0xf3c0019
//disturb=0xf  id_num==0xb  disturb_result=0x8f39001a
//disturb=0xf  id_num==0xc  disturb_result=0xf28001b
//disturb=0xf  id_num==0xd  disturb_result=0x8f2d001c
//disturb=0xf  id_num==0xe  disturb_result=0x8f27001d
//disturb=0xf  id_num==0xf  disturb_result=0xf22001e
//disturb=0xf  id_num==0x10  disturb_result=0x8f63001f
//*/
//
////***********************************************************************************************
////	unsigned int id_judge_fun(unsigned short disturb_code)
////
////  Description:	main interface of the ID check lib, called by init_DRAM function
////
////	Arguments:		16bit disturb_code
////
////	Return Value:	scrambling code after ID check
////***********************************************************************************************
//unsigned int id_judge_fun(unsigned short disturb_code)
//{
//	unsigned char legal_id_num = 0;//
//	unsigned int id_judge_ret	= 0;
//	unsigned char id_check_ok = 1;
//	
//	
//	//*******************************
//	//	There are 16 legal ID number available, that is 1~16
//	//	Note: ID number is not always the same as bonding_id, for example, bonding_id of H8 is "3", we can define the ID number to "0".
//	//*******************************
//	//	add your bonding id judgement function here, after your judgement, legal_id_num & id_check_ok should be set a new value
//
////********************************************************************************//
////****																																			 *****//
////****										your function here!!!!														 *****//
////****																																			 *****//
////****																																			 *****//
////********************************************************************************//
//																																								
//	
//	//	if bonding_id check passed, please define a legal ID number(1~16), and call the coding function with ID number as Arguments
//	//	else if bonding_id check fail, please return a constant value, any constant may be OK.
//	if(id_check_ok == 1)	//id check pass by PD
//	{
//		if( (legal_id_num==0) || (legal_id_num>16) )
//			printk("Please assign a legal ID number!\n");
//			
//		id_judge_ret = disturb_coding_sd(disturb_code,legal_id_num);//legal_id_num should between 1~16
//	}
//	else	//id check failed, return a constant value
//	{
//		return 0;
//	}
//	
//	//after id_check pass, return the disturb code to dram function
//	return id_judge_ret;
//	
//}

//***********************************************************************************************
//	unsigned int bond_id_check(void)
//
//  Description:	called by init_DRAM function, check whether the ID judgment is OK
//
//	Arguments:		None
//
//	Return Value:	check result
//***********************************************************************************************
unsigned int bond_id_check(void)
{
	unsigned char i=0;
	unsigned int id_judge_ret=0;
	unsigned char legal_id_num=0;
	unsigned int reg_val=0;
	
	id_judge_ret = id_judge_fun(0);//(unsigned short disturb_code)
	
	//check the ID number of the platform: ID number is not always the bonding_id
	for(i=1;i<17;i++)//no more than 16 IDs available
	{
		reg_val = disturb_coding_sd(0,i);//(unsigned short disturb_code,unsigned char id_num)
		if(reg_val == id_judge_ret)
		{
			legal_id_num = i;
			break;
		}
	}
	
	if(i>=17)	//return value not legality 
		return 0;
		
	//after bonding_id found, do more check about the ID
	for(i=1;i<5;i++)	//call the function with different disturb code, to avoid 
	{
			id_judge_ret = id_judge_fun(i);
			reg_val = disturb_coding_sd(i,legal_id_num);
			if(id_judge_ret != reg_val)	//if fail one time, the id check not pass
			{
				return 0;
			}
	}
	
	return 1;	//id check OK
}
