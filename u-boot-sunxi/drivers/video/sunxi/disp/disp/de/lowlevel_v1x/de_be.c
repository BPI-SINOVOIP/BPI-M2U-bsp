/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "de_be.h"
#include "de_fe.h"

uintptr_t image_reg_base[2] = { 0, 0 };

__u32 csc_tab[192] = {

	/* Y/G   Y/G      Y/G      Y/G      U/R      U/R */
/*	U/R      U/R     V/B      V/B       V/B    V/B */
	/* bt601 */
	0x04a8, 0x1e70, 0x1cbf, 0x0878, 0x04a8, 0x0000,
	0x0662, 0x3211, 0x04a8, 0x0812, 0x0000, 0x2eb1,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0204, 0x0107, 0x0064, 0x0100, 0x1ed6, 0x1f68,
	0x01c2, 0x0800, 0x1e87, 0x01c2, 0x1fb7, 0x0800,

	/* bt709 */
	0x04a8, 0x1f26, 0x1ddd, 0x04d0, 0x04a8, 0x0000,
	0x072c, 0x307e, 0x04a8, 0x0876, 0x0000, 0x2dea,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0275, 0x00bb, 0x003f, 0x0100, 0x1ea6, 0x1f99,
	0x01c2, 0x0800, 0x1e67, 0x01c2, 0x1fd7, 0x0800,

	/* DISP_YCC */
	0x0400, 0x1e9e, 0x1d24, 0x087b, 0x0400, 0x0000,
	0x059c, 0x34c8, 0x0400, 0x0716, 0x0000, 0x31d5,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0259, 0x0132, 0x0075, 0x0000, 0x1ead, 0x1f53,
	0x0200, 0x0800, 0x1e54, 0x0200, 0x1fac, 0x0800,

	/* xvYCC */
	0x04a8, 0x1f26, 0x1ddd, 0x04d0, 0x04a8, 0x0000,
	0x072c, 0x307e, 0x04a8, 0x0876, 0x0000, 0x2dea,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000,
	0x0275, 0x00bb, 0x003f, 0x0100, 0x1ea6, 0x1f99,
	0x01c2, 0x0800, 0x1e67, 0x01c2, 0x1fd7, 0x0800,
};

__u32 image_enhance_tab[256] = {

	/* bt601(CONSTANT and COEFFICIENT in 12bit fraction) */
	0x0000041D, 0x00000810, 0x00000191, 0x00010000,
	0xFFFFFDA2, 0xFFFFFB58, 0x00000706, 0x00080000,
	0x00000706, 0xFFFFFA1D, 0xFFFFFEDD, 0x00080000,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x000012A0, 0x00000000, 0x00001989, 0xFFF21168,
	0x000012A0, 0xFFFFF9BE, 0xFFFFF2FE, 0x000877CF,
	0x000012A0, 0x0000204A, 0x00000000, 0xFFEEB127,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	/* bt709(CONSTANT and COEFFICIENT in 12bit fraction) */
	0x000002EE, 0x000009D3, 0x000000FE, 0x00010000,
	0xfffffe62, 0xfffffA98, 0x00000706, 0x00080000,
	0x00000706, 0xfffff99E, 0xffffff5C, 0x00080000,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x000012A0, 0x00000000, 0x00001CB0, 0xFFF07DF4,
	0x000012A0, 0xfffffC98, 0xfffff775, 0x0004CFDF,
	0x000012A0, 0x000021D7, 0x00000000, 0xFFEDEA7F,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	/* YCC(CONSTANT and COEFFICIENT in 12bit fraction) */
	0x000004C8, 0x00000963, 0x000001D5, 0x00000000,
	0xFFFFFD4D, 0xFFFFFAB3, 0x00000800, 0x00080000,
	0x00000800, 0xFFFFF94F, 0xFFFFFEB2, 0x00080000,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x00001000, 0x00000000, 0x0000166F, 0xFFF4C84B,
	0x00001000, 0xFFFFFA78, 0xFFFFF491, 0x00087B16,
	0x00001000, 0x00001C56, 0x00000000, 0xFFF1D4FE,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	/* //Dedicated CSC for SAT */
	0x00000368, 0x00000B71, 0x00000127, 0x00000000,
	0xFFFFFE29, 0xFFFFF9D7, 0x00000800, 0x00080000,
	0x00000800, 0xFFFFF8BC, 0xFFFFFF44, 0x00080000,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x00001000, 0x00000000, 0x00001933, 0xFFF36666,
	0x00001000, 0xFFFFFD02, 0xFFFFF883, 0x00053D71,
	0x00001000, 0x00001DB2, 0x00000000, 0xFFF126E9,
	0x00000000, 0x00000000, 0x00000000, 0x00001000,
	/* sin table */
	0xffffffbd, 0xffffffbf, 0xffffffc1, 0xffffffc2,
	0xffffffc4, 0xffffffc6, 0xffffffc8, 0xffffffca,
	0xffffffcc, 0xffffffce, 0xffffffd1, 0xffffffd3,
	0xffffffd5, 0xffffffd7, 0xffffffd9, 0xffffffdb,
	0xffffffdd, 0xffffffdf, 0xffffffe2, 0xffffffe4,
	0xffffffe6, 0xffffffe8, 0xffffffea, 0xffffffec,
	0xffffffef, 0xfffffff1, 0xfffffff3, 0xfffffff5,
	0xfffffff8, 0xfffffffa, 0xfffffffc, 0xfffffffe,
	0x00000000, 0x00000002, 0x00000004, 0x00000006,
	0x00000008, 0x0000000b, 0x0000000d, 0x0000000f,
	0x00000011, 0x00000014, 0x00000016, 0x00000018,
	0x0000001a, 0x0000001c, 0x0000001e, 0x00000021,
	0x00000023, 0x00000025, 0x00000027, 0x00000029,
	0x0000002b, 0x0000002d, 0x0000002f, 0x00000032,
	0x00000034, 0x00000036, 0x00000038, 0x0000003a,
	0x0000003c, 0x0000003e, 0x0000003f, 0x00000041,
	/* cos table */
	0x0000006c, 0x0000006d, 0x0000006e, 0x0000006f,
	0x00000071, 0x00000072, 0x00000073, 0x00000074,
	0x00000074, 0x00000075, 0x00000076, 0x00000077,
	0x00000078, 0x00000079, 0x00000079, 0x0000007a,
	0x0000007b, 0x0000007b, 0x0000007c, 0x0000007c,
	0x0000007d, 0x0000007d, 0x0000007e, 0x0000007e,
	0x0000007e, 0x0000007f, 0x0000007f, 0x0000007f,
	0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x00000080, 0x0000007f, 0x0000007f, 0x0000007f,
	0x0000007f, 0x0000007f, 0x0000007f, 0x0000007f,
	0x0000007e, 0x0000007e, 0x0000007e, 0x0000007d,
	0x0000007d, 0x0000007c, 0x0000007c, 0x0000007b,
	0x0000007b, 0x0000007a, 0x00000079, 0x00000079,
	0x00000078, 0x00000077, 0x00000076, 0x00000075,
	0x00000074, 0x00000074, 0x00000073, 0x00000072,
	0x00000071, 0x0000006f, 0x0000006e, 0x0000006d
};

__u32 fir_tab[1792] = {

	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd,
	0x00063efc, 0xff083dfc, 0x000a3bfb, 0xff0d39fb,
	0xff0f37fb, 0xff1136fa, 0xfe1433fb, 0xfe1631fb,
	0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb, 0xfc2127fc,
	0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd,
	0xfb2f19fd, 0xfb3116fe, 0xfb3314fe, 0xfa3611ff,
	0xfb370fff, 0xfb390dff, 0xfb3b0a00, 0xfc3d08ff,
	0xfc3e0600, 0xfd3f0400, 0xfe3f0300, 0xff400100,
	/* counter = 1 */
	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd,
	0x00063efc, 0xff083dfc, 0x000a3bfb, 0xff0d39fb,
	0xff0f37fb, 0xff1136fa, 0xfe1433fb, 0xfe1631fb,
	0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb, 0xfc2127fc,
	0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd,
	0xfb2f19fd, 0xfb3116fe, 0xfb3314fe, 0xfa3611ff,
	0xfb370fff, 0xfb390dff, 0xfb3b0a00, 0xfc3d08ff,
	0xfc3e0600, 0xfd3f0400, 0xfe3f0300, 0xff400100,
	/* counter = 2 */
	0xff053804, 0xff063803, 0xff083801, 0xff093701,
	0xff0a3700, 0xff0c3500, 0xff0e34ff, 0xff1033fe,
	0xff1232fd, 0xfe1431fd, 0xfe162ffd, 0xfe182dfd,
	0xfd1b2cfc, 0xfd1d2afc, 0xfd1f28fc, 0xfd2126fc,
	0xfd2323fd, 0xfc2621fd, 0xfc281ffd, 0xfc2a1dfd,
	0xfc2c1bfd, 0xfd2d18fe, 0xfd2f16fe, 0xfd3114fe,
	0xfd3212ff, 0xfe3310ff, 0xff340eff, 0x00350cff,
	0x00360a00, 0x01360900, 0x02370700, 0x03370600,
	/* counter = 3 */
	0xff083207, 0xff093206, 0xff0a3205, 0xff0c3203,
	0xff0d3103, 0xff0e3102, 0xfe113001, 0xfe132f00,
	0xfe142e00, 0xfe162dff, 0xfe182bff, 0xfe192aff,
	0xfe1b29fe, 0xfe1d27fe, 0xfe1f25fe, 0xfd2124fe,
	0xfe2222fe, 0xfe2421fd, 0xfe251ffe, 0xfe271dfe,
	0xfe291bfe, 0xff2a19fe, 0xff2b18fe, 0xff2d16fe,
	0x002e14fe, 0x002f12ff, 0x013010ff, 0x02300fff,
	0x03310dff, 0x04310cff, 0x05310a00, 0x06310900,
	/* counter = 4 */
	0xff0a2e09, 0xff0b2e08, 0xff0c2e07, 0xff0e2d06,
	0xff0f2d05, 0xff102d04, 0xff122c03, 0xfe142c02,
	0xfe152b02, 0xfe172a01, 0xfe182901, 0xfe1a2800,
	0xfe1b2700, 0xfe1d2500, 0xff1e24ff, 0xfe2023ff,
	0xff2121ff, 0xff2320fe, 0xff241eff, 0x00251dfe,
	0x00261bff, 0x00281afe, 0x012818ff, 0x012a16ff,
	0x022a15ff, 0x032b13ff, 0x032c12ff, 0x052c10ff,
	0x052d0fff, 0x062d0d00, 0x072d0c00, 0x082d0b00,
	/* counter = 5 */
	0xff0c2a0b, 0xff0d2a0a, 0xff0e2a09, 0xff0f2a08,
	0xff102a07, 0xff112a06, 0xff132905, 0xff142904,
	0xff162803, 0xff172703, 0xff182702, 0xff1a2601,
	0xff1b2501, 0xff1c2401, 0xff1e2300, 0xff1f2200,
	0x00202000, 0x00211f00, 0x01221d00, 0x01231c00,
	0x01251bff, 0x02251aff, 0x032618ff, 0x032717ff,
	0x042815ff, 0x052814ff, 0x052913ff, 0x06291100,
	0x072a10ff, 0x082a0e00, 0x092a0d00, 0x0a2a0c00,
	/* counter = 6 */
	0xff0d280c, 0xff0e280b, 0xff0f280a, 0xff102809,
	0xff112808, 0xff122708, 0xff142706, 0xff152705,
	0xff162605, 0xff172604, 0xff192503, 0xff1a2403,
	0x001b2302, 0x001c2202, 0x001d2201, 0x001e2101,
	0x011f1f01, 0x01211e00, 0x01221d00, 0x02221c00,
	0x02231b00, 0x03241900, 0x04241800, 0x04251700,
	0x052616ff, 0x06261400, 0x072713ff, 0x08271100,
	0x08271100, 0x09271000, 0x0a280e00, 0x0b280d00,
	/* counter = 7 */
	0xff0e260d, 0xff0f260c, 0xff10260b, 0xff11260a,
	0xff122609, 0xff132608, 0xff142508, 0xff152507,
	0x00152506, 0x00172405, 0x00182305, 0x00192304,
	0x001b2203, 0x001c2103, 0x011d2002, 0x011d2002,
	0x011f1f01, 0x021f1e01, 0x02201d01, 0x03211c00,
	0x03221b00, 0x04221a00, 0x04231801, 0x05241700,
	0x06241600, 0x07241500, 0x08251300, 0x09251200,
	0x09261100, 0x0a261000, 0x0b260f00, 0x0c260e00,
	/* counter = 8 */
	0xff0e250e, 0xff0f250d, 0xff10250c, 0xff11250b,
	0x0011250a, 0x00132409, 0x00142408, 0x00152407,
	0x00162307, 0x00172306, 0x00182206, 0x00192205,
	0x011a2104, 0x011b2004, 0x011c2003, 0x021c1f03,
	0x021e1e02, 0x031e1d02, 0x03201c01, 0x04201b01,
	0x04211a01, 0x05221900, 0x05221801, 0x06231700,
	0x07231600, 0x07241500, 0x08241400, 0x09241300,
	0x0a241200, 0x0b241100, 0x0c241000, 0x0d240f00,
	/* counter = 9 */
	0x000e240e, 0x000f240d, 0x0010240c, 0x0011240b,
	0x0013230a, 0x0013230a, 0x00142309, 0x00152308,
	0x00162208, 0x00172207, 0x01182106, 0x01192105,
	0x011a2005, 0x021b1f04, 0x021b1f04, 0x021d1e03,
	0x031d1d03, 0x031e1d02, 0x041e1c02, 0x041f1b02,
	0x05201a01, 0x05211901, 0x06211801, 0x07221700,
	0x07221601, 0x08231500, 0x09231400, 0x0a231300,
	0x0a231300, 0x0b231200, 0x0c231100, 0x0d231000,
	/* counter = 10 */
	0x000f220f, 0x0010220e, 0x0011220d, 0x0012220c,
	0x0013220b, 0x0013220b, 0x0015210a, 0x0015210a,
	0x01162108, 0x01172008, 0x01182007, 0x02191f06,
	0x02191f06, 0x021a1e06, 0x031a1e05, 0x031c1d04,
	0x041c1c04, 0x041d1c03, 0x051d1b03, 0x051e1a03,
	0x061f1902, 0x061f1902, 0x07201801, 0x08201701,
	0x08211601, 0x09211501, 0x0a211500, 0x0b211400,
	0x0b221300, 0x0c221200, 0x0d221100, 0x0e221000,
	/* counter = 11 */
	0x0010210f, 0x0011210e, 0x0011210e, 0x0012210d,
	0x0013210c, 0x0014200c, 0x0114200b, 0x0115200a,
	0x01161f0a, 0x01171f09, 0x02171f08, 0x02181e08,
	0x03181e07, 0x031a1d06, 0x031a1d06, 0x041b1c05,
	0x041c1c04, 0x051c1b04, 0x051d1a04, 0x061d1a03,
	0x071d1903, 0x071e1803, 0x081e1802, 0x081f1702,
	0x091f1602, 0x0a201501, 0x0b1f1501, 0x0b201401,
	0x0c211300, 0x0d211200, 0x0e201200, 0x0e211100,
	/* counter = 12 */
	0x00102010, 0x0011200f, 0x0012200e, 0x0013200d,
	0x0013200d, 0x01141f0c, 0x01151f0b, 0x01151f0b,
	0x01161f0a, 0x02171e09, 0x02171e09, 0x03181d08,
	0x03191d07, 0x03191d07, 0x041a1c06, 0x041b1c05,
	0x051b1b05, 0x051c1b04, 0x061c1a04, 0x071d1903,
	0x071d1903, 0x081d1803, 0x081e1703, 0x091e1702,
	0x0a1f1601, 0x0a1f1502, 0x0b1f1501, 0x0c1f1401,
	0x0d201300, 0x0d201300, 0x0e201200, 0x0f201100,
	/* counter = 13 */
	0x00102010, 0x0011200f, 0x00121f0f, 0x00131f0e,
	0x00141f0d, 0x01141f0c, 0x01141f0c, 0x01151e0c,
	0x02161e0a, 0x02171e09, 0x03171d09, 0x03181d08,
	0x03181d08, 0x04191c07, 0x041a1c06, 0x051a1b06,
	0x051b1b05, 0x061b1a05, 0x061c1a04, 0x071c1904,
	0x081c1903, 0x081d1803, 0x091d1703, 0x091e1702,
	0x0a1e1602, 0x0b1e1502, 0x0c1e1501, 0x0c1f1401,
	0x0d1f1400, 0x0e1f1300, 0x0e1f1201, 0x0f1f1200,
	/* counter = 14 */
	0x00111e11, 0x00121e10, 0x00131e0f, 0x00131e0f,
	0x01131e0e, 0x01141d0e, 0x02151d0c, 0x02151d0c,
	0x02161d0b, 0x03161c0b, 0x03171c0a, 0x04171c09,
	0x04181b09, 0x05181b08, 0x05191b07, 0x06191a07,
	0x061a1a06, 0x071a1906, 0x071b1905, 0x081b1805,
	0x091b1804, 0x091c1704, 0x0a1c1703, 0x0a1c1604,
	0x0b1d1602, 0x0c1d1502, 0x0c1d1502, 0x0d1d1402,
	0x0e1d1401, 0x0e1e1301, 0x0f1e1300, 0x101e1200,
	/* counter = 15 */
	0x00111e11, 0x00121e10, 0x00131d10, 0x01131d0f,
	0x01141d0e, 0x01141d0e, 0x02151c0d, 0x02151c0d,
	0x03161c0b, 0x03161c0b, 0x04171b0a, 0x04171b0a,
	0x05171b09, 0x05181a09, 0x06181a08, 0x06191a07,
	0x07191907, 0x071a1906, 0x081a1806, 0x081a1806,
	0x091a1805, 0x0a1b1704, 0x0a1b1704, 0x0b1c1603,
	0x0b1c1603, 0x0c1c1503, 0x0d1c1502, 0x0d1d1402,
	0x0e1d1401, 0x0f1d1301, 0x0f1d1301, 0x101e1200,
	/* counter = 16 */
	0x40000000, 0x00000000, 0x40fe0000, 0x00000002,
	0x3ffd0100, 0x0000ff04, 0x3efc0100, 0x0000ff06,
	0x3efb0100, 0x0000fe08, 0x3dfa0200, 0x0000fd0a,
	0x3cf90200, 0x0000fd0c, 0x3bf80200, 0x0000fc0f,
	0x39f70200, 0x0000fc12, 0x37f70200, 0x0001fb14,
	0x35f70200, 0x0001fa17, 0x33f70200, 0x0001fa19,
	0x31f70200, 0x0001f91c, 0x2ef70200, 0x0001f91f,
	0x2cf70200, 0x0001f822, 0x2af70200, 0x0001f824,
	0x27f70200, 0x0002f727, 0x24f80100, 0x0002f72a,
	0x22f80100, 0x0002f72c, 0x1ef90100, 0x0002f72f,
	0x1cf90100, 0x0002f731, 0x19fa0100, 0x0002f733,
	0x17fa0100, 0x0002f735, 0x14fb0100, 0x0002f737,
	0x11fc0000, 0x0002f73a, 0x0ffc0000, 0x0002f83b,
	0x0cfd0000, 0x0002f93c, 0x0afd0000, 0x0002fa3d,
	0x08fe0000, 0x0001fb3e, 0x05ff0000, 0x0001fc3f,
	0x03ff0000, 0x0001fd40, 0x02000000, 0x0000fe40,
	/*counter=1 */
	0x40000000, 0x00000000, 0x40fe0000, 0x00000002,
	0x3ffd0100, 0x0000ff04, 0x3efc0100, 0x0000ff06,
	0x3efb0100, 0x0000fe08, 0x3dfa0200, 0x0000fd0a,
	0x3cf90200, 0x0000fd0c, 0x3bf80200, 0x0000fc0f,
	0x39f70200, 0x0000fc12, 0x37f70200, 0x0001fb14,
	0x35f70200, 0x0001fa17, 0x33f70200, 0x0001fa19,
	0x31f70200, 0x0001f91c, 0x2ef70200, 0x0001f91f,
	0x2cf70200, 0x0001f822, 0x2af70200, 0x0001f824,
	0x27f70200, 0x0002f727, 0x24f80100, 0x0002f72a,
	0x22f80100, 0x0002f72c, 0x1ef90100, 0x0002f72f,
	0x1cf90100, 0x0002f731, 0x19fa0100, 0x0002f733,
	0x17fa0100, 0x0002f735, 0x14fb0100, 0x0002f737,
	0x11fc0000, 0x0002f73a, 0x0ffc0000, 0x0002f83b,
	0x0cfd0000, 0x0002f93c, 0x0afd0000, 0x0002fa3d,
	0x08fe0000, 0x0001fb3e, 0x05ff0000, 0x0001fc3f,
	0x03ff0000, 0x0001fd40, 0x02000000, 0x0000fe40,
	/*counter=2 */
	0x3806fc02, 0x0002fc06, 0x3805fc02, 0x0002fb08,
	0x3803fd01, 0x0002fb0a, 0x3801fe01, 0x0002fa0c,
	0x3700fe01, 0x0002fa0e, 0x35ffff01, 0x0003f910,
	0x35fdff01, 0x0003f912, 0x34fc0001, 0x0003f814,
	0x34fb0000, 0x0003f816, 0x33fa0000, 0x0003f719,
	0x31fa0100, 0x0003f71a, 0x2ff90100, 0x0003f71d,
	0x2df80200, 0x0003f71f, 0x2bf80200, 0x0003f721,
	0x2af70200, 0x0003f723, 0x28f70200, 0x0003f725,
	0x27f70200, 0x0002f727, 0x24f70300, 0x0002f729,
	0x22f70300, 0x0002f72b, 0x1ff70300, 0x0002f82d,
	0x1ef70300, 0x0002f82e, 0x1cf70300, 0x0001f930,
	0x1af70300, 0x0001fa31, 0x18f70300, 0x0000fa34,
	0x16f80300, 0x0000fb34, 0x13f80300, 0x0100fc35,
	0x11f90300, 0x01fffd36, 0x0ef90300, 0x01ffff37,
	0x0efa0200, 0x01fe0037, 0x0cfa0200, 0x01fe0138,
	0x0afb0200, 0x01fd0338, 0x08fb0200, 0x02fc0538,
	/*counter=3 */
	0x320bfa02, 0x0002fa0b, 0x3309fa02, 0x0002fa0c,
	0x3208fb02, 0x0002f90e, 0x3206fb02, 0x0002f910,
	0x3205fb02, 0x0002f911, 0x3104fc02, 0x0002f813,
	0x3102fc01, 0x0002f816, 0x3001fd01, 0x0002f817,
	0x3000fd01, 0x0002f818, 0x2ffffd01, 0x0002f81a,
	0x2efefe01, 0x0001f81c, 0x2dfdfe01, 0x0001f81e,
	0x2bfcff01, 0x0001f820, 0x29fcff01, 0x0001f921,
	0x28fbff01, 0x0001f923, 0x27fa0001, 0x0000f925,
	0x26fa0000, 0x0000fa26, 0x24f90000, 0x0100fa28,
	0x22f90100, 0x01fffb29, 0x20f90100, 0x01fffc2a,
	0x1ff80100, 0x01fffc2c, 0x1ef80100, 0x01fefd2d,
	0x1cf80100, 0x01fefe2e, 0x1af80200, 0x01fdff2f,
	0x18f80200, 0x01fd0030, 0x17f80200, 0x01fd0130,
	0x15f80200, 0x01fc0232, 0x12f80200, 0x02fc0432,
	0x11f90200, 0x02fb0532, 0x0ff90200, 0x02fb0633,
	0x0df90200, 0x02fb0833, 0x0cfa0200, 0x02fa0933,
	/*counter=4 */
	0x2e0efa01, 0x0001fa0e, 0x2f0dfa01, 0x0001f90f,
	0x2f0bfa01, 0x0001f911, 0x2e0afa01, 0x0001f913,
	0x2e09fa01, 0x0001f914, 0x2e07fb01, 0x0001f915,
	0x2d06fb01, 0x0000f918, 0x2d05fb01, 0x0000fa18,
	0x2c04fb01, 0x0000fa1a, 0x2b03fc01, 0x0000fa1b,
	0x2a02fc01, 0x0000fa1d, 0x2a01fc01, 0x00fffb1e,
	0x2800fd01, 0x01fffb1f, 0x28fffd01, 0x01fffb20,
	0x26fefd01, 0x01fffc22, 0x25fefe01, 0x01fefc23,
	0x24fdfe01, 0x01fefd24, 0x23fcfe01, 0x01fefe25,
	0x21fcff01, 0x01fdfe27, 0x20fbff01, 0x01fdff28,
	0x1efbff01, 0x01fd0029, 0x1efbff00, 0x01fc012a,
	0x1cfa0000, 0x01fc022b, 0x1bfa0000, 0x01fc032b,
	0x19fa0000, 0x01fb042d, 0x18fa0000, 0x01fb052d,
	0x17f90000, 0x01fb062e, 0x15f90100, 0x01fb072e,
	0x14f90100, 0x01fa092e, 0x12f90100, 0x01fa0a2f,
	0x11f90100, 0x01fa0b2f, 0x0ff90100, 0x01fa0d2f,
	/*counter=5 */
	0x2b10fa00, 0x0000fa11, 0x2b0ffa00, 0x0000fa12,
	0x2b0efa00, 0x0000fa13, 0x2b0cfa00, 0x0000fb14,
	0x2b0bfa00, 0x00fffb16, 0x2a0afb01, 0x00fffb16,
	0x2a09fb01, 0x00fffb17, 0x2908fb01, 0x00fffb19,
	0x2807fb01, 0x00fffc1a, 0x2806fb01, 0x00fefc1c,
	0x2805fb01, 0x00fefd1c, 0x2604fc01, 0x01fefd1d,
	0x2503fc01, 0x01fefe1e, 0x2502fc01, 0x01fdfe20,
	0x2401fc01, 0x01fdff21, 0x2301fc01, 0x01fdff22,
	0x2100fd01, 0x01fd0023, 0x21fffd01, 0x01fc0124,
	0x21fffd01, 0x01fc0124, 0x20fefd01, 0x01fc0225,
	0x1dfefe01, 0x01fc0326, 0x1cfdfe01, 0x01fc0427,
	0x1cfdfe00, 0x01fb0528, 0x1bfcfe00, 0x01fb0629,
	0x19fcff00, 0x01fb0729, 0x19fbff00, 0x01fb0829,
	0x17fbff00, 0x01fb092a, 0x16fbff00, 0x01fb0a2a,
	0x15fbff00, 0x00fa0b2c, 0x14fb0000, 0x00fa0c2b,
	0x13fa0000, 0x00fa0e2b, 0x11fa0000, 0x00fa0f2c,
	/*counter=6 */
	0x2811fcff, 0x00fffc11, 0x2810fcff, 0x00fffc12,
	0x280ffbff, 0x00fffc14, 0x280efbff, 0x00fffc15,
	0x270dfb00, 0x00fefd16, 0x270cfb00, 0x00fefd17,
	0x270bfb00, 0x00fefd18, 0x260afb00, 0x00fefe19,
	0x2609fb00, 0x00fefe1a, 0x2508fb00, 0x00fdfe1d,
	0x2507fb00, 0x00fdff1d, 0x2407fb00, 0x00fdff1e,
	0x2406fc00, 0x00fd001d, 0x2305fc00, 0x00fd011e,
	0x2204fc00, 0x00fd0120, 0x2203fc00, 0x00fc0221,
	0x2103fc00, 0x00fc0321, 0x2002fc00, 0x00fc0323,
	0x1f01fd00, 0x00fc0423, 0x1e01fd00, 0x00fc0523,
	0x1d00fd00, 0x00fc0624, 0x1dfffd00, 0x00fb0725,
	0x1cfffd00, 0x00fb0726, 0x1bfefd00, 0x00fb0827,
	0x1afefe00, 0x00fb0926, 0x19fefe00, 0x00fb0a26,
	0x18fdfe00, 0x00fb0b27, 0x17fdfe00, 0x00fb0c27,
	0x16fdfe00, 0x00fb0d27, 0x15fcff00, 0xfffb0e28,
	0x13fcff00, 0xfffb0f29, 0x12fcff00, 0xfffc1028,
	/*counter=7 */
	0x2512fdfe, 0x00fefd13, 0x2511fdff, 0x00fefd13,
	0x2410fdff, 0x00fefe14, 0x240ffdff, 0x00fefe15,
	0x240efcff, 0x00fefe17, 0x240dfcff, 0x00feff17,
	0x240dfcff, 0x00feff17, 0x240cfcff, 0x00fd0018,
	0x230bfcff, 0x00fd001a, 0x230afc00, 0x00fd001a,
	0x2209fc00, 0x00fd011b, 0x2108fc00, 0x00fd021c,
	0x2108fc00, 0x00fd021c, 0x2007fc00, 0x00fd031d,
	0x2006fc00, 0x00fc031f, 0x2005fc00, 0x00fc041f,
	0x1f05fc00, 0x00fc051f, 0x1e04fc00, 0x00fc0521,
	0x1e03fc00, 0x00fc0621, 0x1c03fd00, 0x00fc0721,
	0x1c02fd00, 0x00fc0821, 0x1b02fd00, 0x00fc0822,
	0x1b01fd00, 0x00fc0922, 0x1a00fd00, 0x00fc0a23,
	0x1900fd00, 0xfffc0b24, 0x1800fd00, 0xfffc0c24,
	0x17fffe00, 0xfffc0d24, 0x16fffe00, 0xfffc0d25,
	0x16fefe00, 0xfffc0e25, 0x14fefe00, 0xfffd0f25,
	0x13fefe00, 0xfffd1025, 0x13fdfe00, 0xfffd1125,
	/*counter=8 */
	0x2212fffe, 0x00feff12, 0x2211fefe, 0x00feff14,
	0x2211fefe, 0x00feff14, 0x2110fefe, 0x00fe0015,
	0x210ffeff, 0x00fe0015, 0x220efdff, 0x00fd0017,
	0x210dfdff, 0x00fd0118, 0x210dfdff, 0x00fd0118,
	0x210cfdff, 0x00fd0218, 0x210bfdff, 0x00fd0219,
	0x200afdff, 0x00fd031a, 0x200afdff, 0x00fd031a,
	0x1f09fdff, 0x00fd041b, 0x1f08fdff, 0x00fd041c,
	0x1d08fd00, 0x00fd051c, 0x1c07fd00, 0x00fd061d,
	0x1d06fd00, 0x00fd061d, 0x1b06fd00, 0x00fd071e,
	0x1b05fd00, 0x00fd081e, 0x1c04fd00, 0xfffd081f,
	0x1b04fd00, 0xfffd091f, 0x1a03fd00, 0xfffd0a20,
	0x1a03fd00, 0xfffd0a20, 0x1902fd00, 0xfffd0b21,
	0x1802fd00, 0xfffd0c21, 0x1801fd00, 0xfffd0d21,
	0x1701fd00, 0xfffd0d22, 0x1600fd00, 0xfffd0e23,
	0x1400fe00, 0xfffe0f22, 0x1400fe00, 0xfefe1022,
	0x14fffe00, 0xfefe1122, 0x13fffe00, 0xfefe1123,
	/*counter=9 */
	0x201200fe, 0x00fe0012, 0x201100fe, 0x00fe0013,
	0x1f11fffe, 0x00fe0114, 0x2010fffe, 0x00fe0114,
	0x1f0ffffe, 0x00fe0116, 0x1e0ffffe, 0x00fe0216,
	0x1f0efeff, 0x00fe0216, 0x1f0dfeff, 0x00fd0317,
	0x1f0dfeff, 0x00fd0317, 0x1e0cfeff, 0x00fd0418,
	0x1e0bfeff, 0x00fd0419, 0x1d0bfeff, 0x00fd0519,
	0x1d0afeff, 0x00fd051a, 0x1d09fdff, 0x00fd061b,
	0x1d09fdff, 0x00fd061b, 0x1c08fdff, 0x00fd071c,
	0x1c07fdff, 0xfffd071e, 0x1b07fd00, 0xfffd081d,
	0x1b06fd00, 0xfffd091d, 0x1a06fd00, 0xfffd091e,
	0x1a05fd00, 0xfffe0a1d, 0x1805fd00, 0xfffe0b1e,
	0x1904fd00, 0xfffe0b1e, 0x1804fd00, 0xfffe0c1e,
	0x1703fd00, 0xfffe0d1f, 0x1703fd00, 0xfffe0d1f,
	0x1602fe00, 0xfffe0e1f, 0x1502fe00, 0xfeff0f1f,
	0x1501fe00, 0xfeff0f20, 0x1401fe00, 0xfeff1020,
	0x1301fe00, 0xfeff1120, 0x1300fe00, 0xfe001120,
	/*counter=10 */
	0x1c1202fe, 0x00fe0212, 0x1c1102fe, 0x00fe0312,
	0x1b1102fe, 0x00fe0313, 0x1c1001fe, 0x00fe0314,
	0x1b1001fe, 0x00fe0414, 0x1b0f01ff, 0x00fe0414,
	0x1b0e00ff, 0x00fe0416, 0x1b0e00ff, 0x00fe0515,
	0x1b0d00ff, 0x00fe0516, 0x1a0d00ff, 0x00fe0616,
	0x1a0c00ff, 0x00fe0617, 0x1a0cffff, 0x00fe0717,
	0x1a0bffff, 0xfffe0719, 0x1a0bffff, 0xfffe0818,
	0x1a0affff, 0xffff0818, 0x180affff, 0xffff0919,
	0x1909ffff, 0xffff0919, 0x1809ffff, 0xffff0a19,
	0x1808ffff, 0xffff0a1a, 0x1808feff, 0xffff0b1a,
	0x1807feff, 0xffff0b1b, 0x1707fe00, 0xffff0c1a,
	0x1606fe00, 0xff000c1b, 0x1506fe00, 0xff000d1b,
	0x1605fe00, 0xff000d1b, 0x1505fe00, 0xff000e1b,
	0x1504fe00, 0xff000e1c, 0x1304fe00, 0xff010f1c,
	0x1304fe00, 0xfe01101c, 0x1303fe00, 0xfe01101d,
	0x1203fe00, 0xfe02111c, 0x1203fe00, 0xfe02111c,
	/*counter=11 */
	0x181104ff, 0x00ff0411, 0x191103ff, 0x00ff0411,
	0x191003ff, 0x00ff0412, 0x181003ff, 0x00ff0512,
	0x180f03ff, 0x00ff0513, 0x190f02ff, 0x00ff0513,
	0x190e02ff, 0x00ff0613, 0x180e02ff, 0x00ff0614,
	0x180d02ff, 0x00ff0714, 0x180d01ff, 0x00ff0715,
	0x180d01ff, 0x00ff0715, 0x180c01ff, 0xffff0816,
	0x180c01ff, 0xffff0816, 0x180b00ff, 0xff000916,
	0x170b00ff, 0xff000917, 0x170a00ff, 0xff000918,
	0x170a00ff, 0xff000a17, 0x170900ff, 0xff000a18,
	0x160900ff, 0xff000b18, 0x160900ff, 0xff000b18,
	0x1608ffff, 0xff010c18, 0x1508ffff, 0xff010c19,
	0x1507ff00, 0xff010d18, 0x1507ff00, 0xff010d18,
	0x1407ff00, 0xff020d18, 0x1306ff00, 0xff020e19,
	0x1306ff00, 0xff020e19, 0x1305ff00, 0xff020f19,
	0x1205ff00, 0xff030f19, 0x1105ff00, 0xff031019,
	0x1204ff00, 0xff031019, 0x1104ff00, 0xff031119,
	/*counter=12 */
	0x171005ff, 0x00ff0511, 0x171005ff, 0x00ff0511,
	0x171004ff, 0x00000511, 0x170f04ff, 0x00000611,
	0x160f04ff, 0x00000612, 0x170f03ff, 0x00000612,
	0x170e03ff, 0x00000712, 0x160e03ff, 0x00000713,
	0x160d03ff, 0x00000714, 0x160d02ff, 0x00000814,
	0x160d02ff, 0x00000814, 0x160c02ff, 0x00000914,
	0x160c02ff, 0x00000914, 0x160c02ff, 0xff010914,
	0x160b01ff, 0xff010a15, 0x150b01ff, 0xff010a16,
	0x150a01ff, 0xff010a17, 0x150a01ff, 0xff010b16,
	0x150a01ff, 0xff010b16, 0x140901ff, 0xff020c16,
	0x14090000, 0xff020c16, 0x14090000, 0xff020c16,
	0x14080000, 0xff020d16, 0x13080000, 0xff020d17,
	0x13070000, 0xff030d17, 0x12070000, 0xff030e17,
	0x12070000, 0xff030e17, 0x12060000, 0xff030f17,
	0x11060000, 0xff040f17, 0x11060000, 0xff040f17,
	0x11050000, 0xff041017, 0x1105ff00, 0xff051017,
	/* counter=13 */
	0x14100600, 0x00000610, 0x15100500, 0x00000610,
	0x150f0500, 0x00000611, 0x150f0500, 0x00000611,
	0x140f0500, 0x00000711, 0x150e0400, 0x00000712,
	0x140e0400, 0x00010712, 0x130e0400, 0x00010812,
	0x140d0400, 0x00010812, 0x150d0300, 0x00010812,
	0x130d0300, 0x00010913, 0x140c0300, 0x00010913,
	0x140c0300, 0x00010913, 0x140c0200, 0x00010a13,
	0x140b0200, 0x00020a13, 0x130b0200, 0x00020a14,
	0x120b0200, 0x00020b14, 0x130a0200, 0x00020b14,
	0x130a0200, 0x00020b14, 0x130a0100, 0x00020c14,
	0x13090100, 0x00030c14, 0x12090100, 0x00030c15,
	0x11090100, 0x00030d15, 0x12080100, 0x00030d15,
	0x11080100, 0x00040d15, 0x10080100, 0x00040e15,
	0x11070100, 0x00040e15, 0x11070000, 0x00040e16,
	0x10070000, 0x00050f15, 0x11060000, 0x00050f15,
	0x10060000, 0x00050f16, 0x10060000, 0x00051015,
	/* counter=14 */
	0x140f0600, 0x00000611, 0x140f0600, 0x00010610,
	0x130f0600, 0x00010710, 0x140f0500, 0x00010710,
	0x140e0500, 0x00010711, 0x130e0500, 0x00010811,
	0x130e0500, 0x00010811, 0x140d0400, 0x00010812,
	0x140d0400, 0x00010812, 0x130d0400, 0x00010912,
	0x120d0400, 0x00020912, 0x130c0400, 0x00020912,
	0x130c0300, 0x00020a12, 0x130c0300, 0x00020a12,
	0x130b0300, 0x00020a13, 0x130b0300, 0x00020a13,
	0x110b0300, 0x00030b13, 0x130a0200, 0x00030b13,
	0x120a0200, 0x00030b14, 0x120a0200, 0x00030c13,
	0x120a0200, 0x00030c13, 0x12090200, 0x00040c13,
	0x10090200, 0x00040d14, 0x11090100, 0x00040d14,
	0x11080100, 0x00040d15, 0x11080100, 0x00040d15,
	0x10080100, 0x00050e14, 0x10080100, 0x00050e14,
	0x10070100, 0x00050e15, 0x10070100, 0x00050f14,
	0x0f070100, 0x00060f14, 0x10060100, 0x00060f14,
	/* counter=15 */
	0x120f0701, 0x0001070f, 0x130f0601, 0x0001070f,
	0x130e0601, 0x00010710, 0x130e0601, 0x00010710,
	0x120e0601, 0x00010810, 0x130e0501, 0x00010810,
	0x130e0500, 0x00020810, 0x130d0500, 0x00020811,
	0x120d0500, 0x00020911, 0x120d0500, 0x00020911,
	0x130c0400, 0x00020912, 0x130c0400, 0x00020912,
	0x120c0400, 0x00020a12, 0x110c0400, 0x00030a12,
	0x120b0400, 0x00030a12, 0x120b0300, 0x00030b12,
	0x120b0300, 0x00030b12, 0x120b0300, 0x00030b12,
	0x120a0300, 0x00040b12, 0x110a0300, 0x00040c12,
	0x110a0200, 0x00040c13, 0x11090200, 0x00040c14,
	0x11090200, 0x00040c14, 0x10090200, 0x00050d13,
	0x10090200, 0x00050d13, 0x10080200, 0x00050d14,
	0x10080200, 0x00050e13, 0x10080100, 0x01050e13,
	0x0f080100, 0x01060e13, 0x10070100, 0x01060e13,
	0x0f070100, 0x01060e14, 0x0f070100, 0x01060f13,
	/* counter=16 */
	0x120f0701, 0x0001070f, 0x120e0701, 0x00010710,
	0x120e0601, 0x00020710, 0x120e0601, 0x0002080f,
	0x120e0601, 0x0002080f, 0x110e0601, 0x00020810,
	0x120d0601, 0x00020810, 0x120d0501, 0x00020910,
	0x120d0501, 0x00020910, 0x120d0501, 0x00020910,
	0x120c0500, 0x00030911, 0x110c0500, 0x00030a11,
	0x120c0400, 0x00030a11, 0x120c0400, 0x00030a11,
	0x120b0400, 0x00030a12, 0x110b0400, 0x00030b12,
	0x110b0400, 0x00040b11, 0x110b0300, 0x00040b12,
	0x110a0300, 0x00040b13, 0x110a0300, 0x00040c12,
	0x110a0300, 0x00040c12, 0x100a0300, 0x00050c12,
	0x11090300, 0x00050c12, 0x10090200, 0x01050d12,
	0x10090200, 0x01050d12, 0x10090200, 0x01050d12,
	0x10080200, 0x01060d12, 0x0f080200, 0x01060e12,
	0x0f080200, 0x01060e12, 0x0f080200, 0x01060e12,
	0x0f070200, 0x01060e13, 0x0f070100, 0x01070e13,
	/* counter=17 */
	0x120e0702, 0x0002070e, 0x120e0701, 0x0002070f,
	0x110e0701, 0x0002080f, 0x110e0701, 0x0002080f,
	0x120e0601, 0x0002080f, 0x120d0601, 0x00020810,
	0x120d0601, 0x0002090f, 0x110d0601, 0x00020910,
	0x110d0501, 0x00030910, 0x110c0501, 0x00030911,
	0x110c0501, 0x00030911, 0x110c0501, 0x00030a10,
	0x110c0501, 0x00030a10, 0x110c0401, 0x00030a11,
	0x110b0400, 0x00040a12, 0x110b0400, 0x00040b11,
	0x110b0400, 0x00040b11, 0x110b0400, 0x00040b11,
	0x110a0400, 0x00040b12, 0x100a0300, 0x01040c12,
	0x100a0300, 0x01050c11, 0x100a0300, 0x01050c11,
	0x10090300, 0x01050c12, 0x10090300, 0x01050c12,
	0x10090300, 0x01050d11, 0x0f090200, 0x01060d12,
	0x0f090200, 0x01060d12, 0x0f080200, 0x01060d13,
	0x0f080200, 0x01060e12, 0x0e080200, 0x01070e12,
	0x0e080200, 0x01070e12, 0x0f070200, 0x01070e12,
	/* counter=18 */
	0x100e0802, 0x0002080e, 0x110e0702, 0x0002080e,
	0x110e0702, 0x0002080e, 0x110d0702, 0x0002080f,
	0x100d0702, 0x0003080f, 0x0f0d0702, 0x0003090f,
	0x110d0601, 0x0003090f, 0x110d0601, 0x0003090f,
	0x110c0601, 0x00030910, 0x110c0601, 0x00030910,
	0x100c0601, 0x00030a10, 0x100c0501, 0x00040a10,
	0x100c0501, 0x00040a10, 0x100c0501, 0x00040a10,
	0x100b0501, 0x00040a11, 0x0f0b0501, 0x01040b10,
	0x100b0401, 0x01040b10, 0x0f0b0401, 0x01050b10,
	0x100a0400, 0x01050b11, 0x100a0400, 0x01050c10,
	0x100a0400, 0x01050c10, 0x100a0400, 0x01050c10,
	0x0f0a0300, 0x01060c11, 0x0f090300, 0x01060c12,
	0x0f090300, 0x01060c12, 0x0f090300, 0x01060d11,
	0x0f090300, 0x01060d11, 0x0d090300, 0x02070d11,
	0x0e080300, 0x02070d11, 0x0e080200, 0x02070d12,
	0x0e080200, 0x02070e11, 0x0e080200, 0x02070e11,
	/* counter=19 */
	0x100e0802, 0x0002080e, 0x100d0802, 0x0003080e,
	0x100d0702, 0x0003080f, 0x100d0702, 0x0003080f,
	0x100d0702, 0x0003090e, 0x100d0702, 0x0003090e,
	0x100d0702, 0x0003090e, 0x100c0602, 0x00030910,
	0x100c0602, 0x0004090f, 0x100c0601, 0x00040a0f,
	0x100c0601, 0x00040a0f, 0x100c0601, 0x00040a0f,
	0x0f0c0601, 0x01040a0f, 0x100b0501, 0x01040a10,
	0x100b0501, 0x01040a10, 0x0f0b0501, 0x01050b0f,
	0x0f0b0501, 0x01050b0f, 0x0e0b0501, 0x01050b10,
	0x0f0a0401, 0x01050b11, 0x0f0a0401, 0x01050b11,
	0x0e0a0401, 0x01060c10, 0x0f0a0400, 0x01060c10,
	0x0f0a0400, 0x01060c10, 0x0f0a0400, 0x01060c10,
	0x0f090400, 0x02060c10, 0x0f090300, 0x02060c11,
	0x0e090300, 0x02070d10, 0x0e090300, 0x02070d10,
	0x0e090300, 0x02070d10, 0x0e080300, 0x02070d11,
	0x0e080300, 0x02070d11, 0x0e080300, 0x02080d10,
	/* counter=20 */
};

__s32 DE_Set_Reg_Base(__u32 sel, uintptr_t address)
{
	image_reg_base[sel] = address;
	/* memset((void*)(image0_reg_base+0x800), 0,0x1000-0x800); */

	return 0;
}

__u32 DE_Get_Reg_Base(__u32 sel)
{

	return image_reg_base[sel];

}

__u32 DE_BE_Reg_Init(__u32 sel)
{
	DE_BE_ClearINT(sel, 0xffffffff);
	memset((void *)(image_reg_base[sel] + 0x800), 0, 0x1000 - 0x800);
	DE_BE_WUINT32(sel, DE_BE_DMA_CTRL, 0x33333333);

	return 0;
}

__s32 DE_BE_Set_SystemPalette(__u32 sel,
			      __u32 *pbuffer, __u32 offset, __u32 size)
{
	__u32 *pdest_end;
	__u32 *psrc_cur;
	__u32 *pdest_cur;

	if (size > DE_BE_PALETTE_TABLE_SIZE)
		size = DE_BE_PALETTE_TABLE_SIZE;

	psrc_cur = pbuffer;
	pdest_cur = (__u32 *) (DE_Get_Reg_Base(sel) +
			       DE_BE_PALETTE_TABLE_ADDR_OFF + offset);
	pdest_end = pdest_cur + (size >> 2);

	while (pdest_cur < pdest_end) {
		writel(*psrc_cur, (void *)pdest_cur);
		pdest_cur++;
		psrc_cur++;
	}

	return 0;
}

__s32 DE_BE_Get_SystemPalette(__u32 sel,
			      __u32 *pbuffer, __u32 offset, __u32 size)
{
	__u32 *pdest_end;
	__u32 *psrc_cur;
	__u32 *pdest_cur;

	if (size > DE_BE_PALETTE_TABLE_SIZE)
		size = DE_BE_PALETTE_TABLE_SIZE;

	psrc_cur = (__u32 *) (DE_Get_Reg_Base(sel) +
			      DE_BE_PALETTE_TABLE_ADDR_OFF + offset);
	pdest_cur = pbuffer;
	pdest_end = pdest_cur + (size >> 2);

	while (pdest_cur < pdest_end) {
		writel(*psrc_cur, (void *)pdest_cur);
		pdest_cur++;
		psrc_cur++;
	}

	return 0;
}

__s32 DE_BE_Enable(__u32 sel)
{
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) | (0x01 << 1));
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) | 0x01);

	return 0;
}

__s32 DE_BE_Disable(__u32 sel)
{
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) & 0xfffffffd);
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) & 0xfffffffe);

	return 0;
}

/* 0:lcd only */
/* 2:lcd0+fe0; 3:lcd1+fe0 */
/* 4:lcd0+fe1; 5:lcd1+fe1 */
/* 6:fe0 only;  7:fe1 only */
__s32 DE_BE_Output_Select(__u32 sel, __u32 out_sel)
{
	out_sel = (out_sel == 1) ? 0 : out_sel;
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      (DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) & 0xff8fffff) |
		      (out_sel << 20));

	if ((out_sel == 6) || (out_sel == 7))
		DE_BE_WUINT32(sel, DE_BE_ERROR_CORRECTION, 0xffffffff);
	else
		DE_BE_WUINT32(sel, DE_BE_ERROR_CORRECTION, 0);

	return 0;
}

__s32 DE_BE_Set_BkColor(__u32 sel, disp_color_info bkcolor)
{
	DE_BE_WUINT32(sel, DE_BE_COLOR_CTL_OFF, (bkcolor.alpha << 24) |
		      (bkcolor.red << 16) | (bkcolor.green << 8) | bkcolor.
		      blue);

	return 0;
}

__s32 DE_BE_Set_ColorKey(__u32 sel,
			 disp_color_info ck_max, disp_color_info ck_min,
			 __u32 ck_red_match, __u32 ck_green_match,
			 __u32 ck_blue_match)
{
	DE_BE_WUINT32(sel, DE_BE_CLRKEY_MAX_OFF,
		      (ck_max.alpha << 24) | (ck_max.red << 16) |
		      (ck_max.green << 8) | ck_max.blue);
	DE_BE_WUINT32(sel, DE_BE_CLRKEY_MIN_OFF,
		      (ck_min.alpha << 24) | (ck_min.red << 16) |
		      (ck_min.green << 8) | ck_min.blue);
	DE_BE_WUINT32(sel, DE_BE_CLRKEY_CFG_OFF,
		      (ck_red_match << 4) | (ck_green_match << 2) |
		      ck_blue_match);

	return 0;
}

__s32 DE_BE_reg_auto_load_en(__u32 sel, __u32 en)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_FRMBUF_CTL_OFF);
	/* bit1:enable, bit0:ready */
	DE_BE_WUINT32(sel, DE_BE_FRMBUF_CTL_OFF, tmp | ((1 - en) << 1));

	return 0;
}

__s32 DE_BE_Cfg_Ready(__u32 sel)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_FRMBUF_CTL_OFF);
	/* bit1:enable, bit0:ready */
	DE_BE_WUINT32(sel, DE_BE_FRMBUF_CTL_OFF, tmp | 0x1);

	return 0;
}

__s32 DE_BE_Sprite_Enable(__u32 sel, bool enable)
{
	DE_BE_WUINT32(sel, DE_BE_SPRITE_EN_OFF,
		      (DE_BE_RUINT32(sel, DE_BE_SPRITE_EN_OFF) & 0xfffffffe) |
		      enable);
	return 0;
}

__s32 DE_BE_Sprite_Disable(__u32 sel)
{
	DE_BE_WUINT32(sel, DE_BE_SPRITE_EN_OFF,
		      DE_BE_RUINT32(sel, DE_BE_SPRITE_EN_OFF) & 0xfffffffe);
	return 0;
}

__s32 DE_BE_Sprite_Set_Format(__u32 sel, __u8 pixel_seq, __u8 format)
{
	DE_BE_WUINT32(sel, DE_BE_SPRITE_FORMAT_CTRL_OFF,
		      (pixel_seq << 12) | (format << 8));
	return 0;
}

__s32 DE_BE_Sprite_Global_Alpha_Enable(__u32 sel, bool enable)
{
	DE_BE_WUINT32(sel, DE_BE_SPRITE_ALPHA_CTRL_OFF, enable);
	return 0;
}

__s32 DE_BE_Sprite_Set_Global_Alpha(__u32 sel, __u8 alpha_val)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_SPRITE_ALPHA_CTRL_OFF);
	tmp = (tmp & 0x00ffffff) | (alpha_val << 24);

	DE_BE_WUINT32(sel, DE_BE_SPRITE_ALPHA_CTRL_OFF, tmp);
	return 0;
}

__s32 DE_BE_Sprite_Block_Set_Pos(__u32 sel, __u8 blk_idx, __s16 x, __s16 y)
{
	DE_BE_WUINT32IDX(sel, DE_BE_SPRITE_POS_CTRL_OFF,
			 blk_idx, ((y & 0xffff) << 16) | (x & 0xffff));
	return 0;
}

__s32 DE_BE_Sprite_Block_Set_Size(__u32 sel,
				  __u8 blk_idx, __u32 xsize, __u32 ysize)
{
	__u32 tmp = 0;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_SPRITE_ATTR_CTRL_OFF,
			       blk_idx) & 0x0000003f;

	DE_BE_WUINT32IDX(sel, DE_BE_SPRITE_ATTR_CTRL_OFF,
			 blk_idx,
			 tmp | ((ysize - 1) << 20) | ((xsize - 1) << 8));
	return 0;
}

__s32 DE_BE_Sprite_Block_Set_fb(__u32 sel,
				__u8 blk_idx, __u32 addr, __u32 line_width)
{
	DE_BE_WUINT32IDX(sel, DE_BE_SPRITE_ADDR_OFF, blk_idx, addr);
	DE_BE_WUINT32IDX(sel, DE_BE_SPRITE_LINE_WIDTH_OFF,
			 blk_idx, line_width << 3);
	return 0;
}

__s32 DE_BE_Sprite_Block_Set_Next_Id(__u32 sel, __u8 blk_idx, __u8 next_blk_id)
{
	__u32 tmp = 0;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_SPRITE_ATTR_CTRL_OFF,
			       blk_idx) & 0xffffffc0;
	DE_BE_WUINT32IDX(sel, DE_BE_SPRITE_ATTR_CTRL_OFF,
			 blk_idx, tmp | next_blk_id);
	return 0;
}

__s32 DE_BE_Sprite_Set_Palette_Table(__u32 sel,
				     __u32 address, __u32 offset, __u32 size)
{
	__u32 *pdest_end;
	__u32 *psrc_cur;
	__u32 *pdest_cur;

	if (size > DE_BE_SPRITE_PALETTE_TABLE_SIZE)
		size = DE_BE_SPRITE_PALETTE_TABLE_SIZE;

	psrc_cur = (__u32 *) address;
	pdest_cur = (__u32 *) (DE_Get_Reg_Base(sel) +
			       DE_BE_SPRITE_PALETTE_TABLE_ADDR_OFF + offset);
	pdest_end = pdest_cur + (size >> 2);

	while (pdest_cur < pdest_end) {
		writel(*psrc_cur, (void *)pdest_cur);
		pdest_cur++;
		psrc_cur++;
	}

	return 0;
}

/* out_csc: 0:rgb, 1:yuv for tv, 2:yuv for hdmi, 3: yuv for drc */
/* out_color_range:  0:16~255, 1:0~255, 2:16~235 */
__s32 DE_BE_Set_Enhance_ex(__u8 sel,
			   __u32 out_csc, __u32 out_color_range,
			   __u32 enhance_en, __u32 brightness, __u32 contrast,
			   __u32 saturaion, __u32 hue)
{
	__s32 i_bright;
	__s32 i_contrast;
	__s32 i_saturaion;
	__s32 i_hue;		/* fix */
	struct __scal_matrix4x4 matrixEn;
	struct __scal_matrix4x4 matrixconv, *ptmatrix;
	struct __scal_matrix4x4 matrixresult;
	__s64 *pt;
	__s32 sinv, cosv;	/* sin_tab: 7 bit fractional */
	__s32 i;
	struct __scal_matrix4x4 tmpcoeff;

	brightness = brightness > 100 ? 100 : brightness;
	contrast = contrast > 100 ? 100 : contrast;
	saturaion = saturaion > 100 ? 100 : saturaion;

	i_bright = (__s32) (brightness * 64 / 100);
	i_saturaion = (__s32) (saturaion * 64 / 100);
	i_contrast = (__s32) (contrast * 64 / 100);
	i_hue = (__s32) (hue * 64 / 100);

	sinv = image_enhance_tab[8 * 16 + (i_hue & 0x3f)];
	cosv = image_enhance_tab[8 * 16 + 8 * 8 + (i_hue & 0x3f)];

	memset(&matrixconv, 0, sizeof(struct __scal_matrix4x4));

	/* calculate enhance matrix */
	matrixEn.x00 = i_contrast << 5;
	matrixEn.x01 = 0;
	matrixEn.x02 = 0;
	matrixEn.x03 = (((i_bright - 32) + 16) << 10) - (i_contrast << 9);
	matrixEn.x10 = 0;
	matrixEn.x11 = (i_contrast * i_saturaion * cosv) >> 7;
	matrixEn.x12 = (i_contrast * i_saturaion * sinv) >> 7;
	matrixEn.x13 = (1 << 17) - ((matrixEn.x11 + matrixEn.x12) << 7);
	matrixEn.x20 = 0;
	matrixEn.x21 = (-i_contrast * i_saturaion * sinv) >> 7;
	matrixEn.x22 = (i_contrast * i_saturaion * cosv) >> 7;
	matrixEn.x23 = (1 << 17) - ((matrixEn.x22 + matrixEn.x21) << 7);
	matrixEn.x30 = 0;
	matrixEn.x31 = 0;
	matrixEn.x32 = 0;
	matrixEn.x33 = 1024;

	if (out_csc == 0) {	/* RGB output */
		if (enhance_en == 1) {
			for (i = 0; i < 16; i++) {
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    ((__s64) *
				     (image_enhance_tab + 0x20 +
				      i) << 32) >> 32;
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    /* bt601 rgb2yuv coeff */
				    ((__s64) (*(image_enhance_tab + i)) << 32)
				    >> 32;
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    /* YCC rgb2yuv coeff */
				    ((__s64) *
				     (image_enhance_tab + 0x40 +
				      i) << 32) >> 32;
			}

			ptmatrix = &tmpcoeff;

			/* convolution of enhance matrix and rgb2yuv matrix */
			iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);

			for (i = 0; i < 16; i++) {
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    ((__s64) *
				     (image_enhance_tab + 0x30 +
				      i) << 32) >> 32;
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    /* bt601 yuv2rgb coeff */
				    ((__s64) *
				     (image_enhance_tab + 0x10 +
				      i) << 32) >> 32;
				*((__s64 *) (&tmpcoeff.x00) + i) =
				    /* YCC yuv2rgb coeff */
				    ((__s64) *
				     (image_enhance_tab + 0x50 +
				      i) << 32) >> 32;
			}

			ptmatrix = &tmpcoeff;

			/* convert to RGB */
			iDE_SCAL_Matrix_Mul(*ptmatrix, matrixconv, &matrixconv);

			/* rearrange CSC coeff */
			matrixresult.x00 = (matrixconv.x00 + 8) / 16;
			matrixresult.x01 = (matrixconv.x01 + 8) / 16;
			matrixresult.x02 = (matrixconv.x02 + 8) / 16;
			matrixresult.x03 = (matrixconv.x03 + 512) / 1024;
			matrixresult.x10 = (matrixconv.x10 + 8) / 16;
			matrixresult.x11 = (matrixconv.x11 + 8) / 16;
			matrixresult.x12 = (matrixconv.x12 + 8) / 16;
			matrixresult.x13 = (matrixconv.x13 + 512) / 1024;
			matrixresult.x20 = (matrixconv.x20 + 8) / 16;
			matrixresult.x21 = (matrixconv.x21 + 8) / 16;
			matrixresult.x22 = (matrixconv.x22 + 8) / 16;
			matrixresult.x23 = (matrixconv.x23 + 512) / 1024;
			matrixresult.x30 = (matrixconv.x30 + 8) / 16;
			matrixresult.x31 = (matrixconv.x31 + 8) / 16;
			matrixresult.x32 = (matrixconv.x32 + 8) / 16;
			matrixresult.x33 = (matrixconv.x33 + 512) / 1024;
		} else {
			matrixresult.x00 = 0x400;
			matrixresult.x01 = 0;
			matrixresult.x02 = 0;
			matrixresult.x03 = 0;
			matrixresult.x10 = 0;
			matrixresult.x11 = 0x400;
			matrixresult.x12 = 0;
			matrixresult.x13 = 0;
			matrixresult.x20 = 0;
			matrixresult.x21 = 0;
			matrixresult.x22 = 0x400;
			matrixresult.x23 = 0;
			matrixresult.x30 = 0;
			matrixresult.x31 = 0;
			matrixresult.x32 = 0;
			matrixresult.x33 = 0x400;
		}

		/* OUTPUT RANGE MODIFY */
		ptmatrix = &matrixresult;

		if (out_color_range == DISP_COLOR_RANGE_16_255) {
			matrixconv.x00 = 0x03c4;
			matrixconv.x01 = 0x0000;
			matrixconv.x02 = 0x0000;
			matrixconv.x03 = 0x0100;
			matrixconv.x10 = 0x0000;
			matrixconv.x11 = 0x03c4;
			matrixconv.x12 = 0x0000;
			matrixconv.x13 = 0x0100;
			matrixconv.x20 = 0x0000;
			matrixconv.x21 = 0x0000;
			matrixconv.x22 = 0x03c4;
			matrixconv.x23 = 0x0100;
			matrixconv.x30 = 0x0000;
			matrixconv.x31 = 0x0000;
			matrixconv.x32 = 0x0000;
			matrixconv.x33 = 0x0100;
		} else if (out_color_range == DISP_COLOR_RANGE_16_235) {
			matrixconv.x00 = 0x0370;
			matrixconv.x01 = 0x0000;
			matrixconv.x02 = 0x0000;
			matrixconv.x03 = 0x0100;
			matrixconv.x10 = 0x0000;
			matrixconv.x11 = 0x0370;
			matrixconv.x12 = 0x0000;
			matrixconv.x13 = 0x0100;
			matrixconv.x20 = 0x0000;
			matrixconv.x21 = 0x0000;
			matrixconv.x22 = 0x0370;
			matrixconv.x23 = 0x0100;
		} else {	/* DISP_COLOR_RANGE_0_255 */

			matrixconv.x00 = 0x0400;
			matrixconv.x01 = 0x0000;
			matrixconv.x02 = 0x0000;
			matrixconv.x03 = 0x0000;
			matrixconv.x10 = 0x0000;
			matrixconv.x11 = 0x0400;
			matrixconv.x12 = 0x0000;
			matrixconv.x13 = 0x0000;
			matrixconv.x20 = 0x0000;
			matrixconv.x21 = 0x0000;
			matrixconv.x22 = 0x0400;
			matrixconv.x23 = 0x0000;
		}

		iDE_SCAL_Matrix_Mul(matrixconv, *ptmatrix, &matrixresult);

		matrixresult.x00 = matrixresult.x00;
		matrixresult.x01 = matrixresult.x01;
		matrixresult.x02 = matrixresult.x02;
		matrixresult.x03 = matrixresult.x03 + 8;
		matrixresult.x10 = matrixresult.x10;
		matrixresult.x11 = matrixresult.x11;
		matrixresult.x12 = matrixresult.x12;
		matrixresult.x13 = matrixresult.x13 + 8;
		matrixresult.x20 = matrixresult.x20;
		matrixresult.x21 = matrixresult.x21;
		matrixresult.x22 = matrixresult.x22;
		matrixresult.x23 = matrixresult.x23 + 8;
	}

	else if (out_csc == 1) {	/* YUV for tv(range 16-235) */
		for (i = 0; i < 16; i++) {
			*((__s64 *) (&tmpcoeff.x00) + i) =
			    ((__s64) *(image_enhance_tab + i) << 32) >> 32;
		}

		if (enhance_en == 1) {
			/* convolution of enhance matrix and rgb2yuv matrix */

			ptmatrix = &tmpcoeff;

			iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);

			matrixresult.x00 = matrixconv.x00 / 4;
			matrixresult.x01 = matrixconv.x01 / 4;
			matrixresult.x02 = matrixconv.x02 / 4;
			matrixresult.x03 = matrixconv.x03 / 256 + 8;
			matrixresult.x10 = matrixconv.x10 / 4;
			matrixresult.x11 = matrixconv.x11 / 4;
			matrixresult.x12 = matrixconv.x12 / 4;
			matrixresult.x13 = matrixconv.x13 / 256 + 8;
			matrixresult.x20 = matrixconv.x20 / 4;
			matrixresult.x21 = matrixconv.x21 / 4;
			matrixresult.x22 = matrixconv.x22 / 4;
			matrixresult.x23 = matrixconv.x23 / 256 + 8;
		} else {
			matrixresult.x00 = tmpcoeff.x00 / 4;
			matrixresult.x01 = tmpcoeff.x01 / 4;
			matrixresult.x02 = tmpcoeff.x02 / 4;
			matrixresult.x03 = tmpcoeff.x03 / 256 + 8;
			matrixresult.x10 = tmpcoeff.x10 / 4;
			matrixresult.x11 = tmpcoeff.x11 / 4;
			matrixresult.x12 = tmpcoeff.x12 / 4;
			matrixresult.x13 = tmpcoeff.x13 / 256 + 8;
			matrixresult.x20 = tmpcoeff.x20 / 4;
			matrixresult.x21 = tmpcoeff.x21 / 4;
			matrixresult.x22 = tmpcoeff.x22 / 4;
			matrixresult.x23 = tmpcoeff.x23 / 256 + 8;
		}
	} else if (out_csc == 2) {	/* YUV for HDMI(range 16-235) */
		for (i = 0; i < 16; i++) {
			*((__s64 *) (&tmpcoeff.x00) + i) =
			    ((__s64) *(image_enhance_tab + i) << 32) >> 32;
		}

		if (enhance_en == 1) {
			/* convolution of enhance matrix and rgb2yuv matrix */

			ptmatrix = &tmpcoeff;

			iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);

			matrixresult.x00 = matrixconv.x20 / 4;
			matrixresult.x01 = matrixconv.x21 / 4;
			matrixresult.x02 = matrixconv.x22 / 4;
			matrixresult.x03 = matrixconv.x23 / 256 + 8;
			matrixresult.x10 = matrixconv.x00 / 4;
			matrixresult.x11 = matrixconv.x01 / 4;
			matrixresult.x12 = matrixconv.x02 / 4;
			matrixresult.x13 = matrixconv.x03 / 256 + 8;
			matrixresult.x20 = matrixconv.x10 / 4;
			matrixresult.x21 = matrixconv.x11 / 4;
			matrixresult.x22 = matrixconv.x12 / 4;
			matrixresult.x23 = matrixconv.x13 / 256 + 8;
		} else {
			matrixresult.x00 = tmpcoeff.x20 / 4;
			matrixresult.x01 = tmpcoeff.x21 / 4;
			matrixresult.x02 = tmpcoeff.x22 / 4;
			matrixresult.x03 = tmpcoeff.x23 / 256 + 8;
			matrixresult.x10 = tmpcoeff.x00 / 4;
			matrixresult.x11 = tmpcoeff.x01 / 4;
			matrixresult.x12 = tmpcoeff.x02 / 4;
			matrixresult.x13 = tmpcoeff.x03 / 256 + 8;
			matrixresult.x20 = tmpcoeff.x10 / 4;
			matrixresult.x21 = tmpcoeff.x11 / 4;
			matrixresult.x22 = tmpcoeff.x12 / 4;
			matrixresult.x23 = tmpcoeff.x13 / 256 + 8;
		}
	} else {		/* if(out_csc == 3)//YUV for DRC */

		for (i = 0; i < 16; i++) {
			*((__s64 *) (&tmpcoeff.x00) + i) =
			    ((__s64) *
			     (image_enhance_tab + 0x60 + i) << 32) >> 32;
		}

		if (enhance_en == 1) {
			/* convolution of enhance matrix and rgb2yuv matrix */

			ptmatrix = &tmpcoeff;

			iDE_SCAL_Matrix_Mul(matrixEn, *ptmatrix, &matrixconv);

			matrixresult.x00 = matrixconv.x00 / 4;
			matrixresult.x01 = matrixconv.x01 / 4;
			matrixresult.x02 = matrixconv.x02 / 4;
			matrixresult.x03 = matrixconv.x03 / 256 + 8;
			matrixresult.x10 = matrixconv.x10 / 4;
			matrixresult.x11 = matrixconv.x11 / 4;
			matrixresult.x12 = matrixconv.x12 / 4;
			matrixresult.x13 = matrixconv.x13 / 256 + 8;
			matrixresult.x20 = matrixconv.x20 / 4;
			matrixresult.x21 = matrixconv.x21 / 4;
			matrixresult.x22 = matrixconv.x22 / 4;
			matrixresult.x23 = matrixconv.x23 / 256 + 8;
		} else {
			matrixresult.x00 = tmpcoeff.x00 / 4;
			matrixresult.x01 = tmpcoeff.x01 / 4;
			matrixresult.x02 = tmpcoeff.x02 / 4;
			matrixresult.x03 = tmpcoeff.x03 / 256 + 8;
			matrixresult.x10 = tmpcoeff.x10 / 4;
			matrixresult.x11 = tmpcoeff.x11 / 4;
			matrixresult.x12 = tmpcoeff.x12 / 4;
			matrixresult.x13 = tmpcoeff.x13 / 256 + 8;
			matrixresult.x20 = tmpcoeff.x20 / 4;
			matrixresult.x21 = tmpcoeff.x21 / 4;
			matrixresult.x22 = tmpcoeff.x22 / 4;
			matrixresult.x23 = tmpcoeff.x23 / 256 + 8;
		}
	}

	/* range limited */
	iDE_SCAL_Csc_Lmt(&matrixresult.x00, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x01, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x02, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x03, -16383, 16383, 0, 32767);
	iDE_SCAL_Csc_Lmt(&matrixresult.x10, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x11, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x12, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x13, -16383, 16383, 0, 32767);
	iDE_SCAL_Csc_Lmt(&matrixresult.x20, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x21, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x22, -8191, 8191, 0, 16383);
	iDE_SCAL_Csc_Lmt(&matrixresult.x23, -16383, 16383, 0, 32767);

	pt = (__s64 *) &(matrixresult.x00);

	for (i = 0; i < 4; i++) {
		DE_BE_WUINT32(sel, DE_BE_OUT_COLOR_R_COEFF_OFF + 4 * i,
			      (__u32) (*(pt + i)));
		DE_BE_WUINT32(sel, DE_BE_OUT_COLOR_G_COEFF_OFF + 4 * i,
			      (__u32) (*(pt + 4 + i)));
		DE_BE_WUINT32(sel, DE_BE_OUT_COLOR_B_COEFF_OFF + 4 * i,
			      (__u32) (*(pt + 8 + i)));
	}

	DE_BE_enhance_enable(sel, 1);

	return 0;
}

__s32 DE_BE_enhance_enable(__u32 sel, bool enable)
{
	DE_BE_WUINT32(sel, DE_BE_OUT_COLOR_CTRL_OFF, enable);

	return 0;
}

__s32 DE_BE_deflicker_enable(__u32 sel, bool enable)
{
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      (DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) & (~(1 << 4))) |
		      (enable << 4));

	return 0;
}

__s32 DE_BE_Set_Outitl_enable(__u32 sel, bool enable)
{
	DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
		      (DE_BE_RUINT32(sel, DE_BE_MODE_CTL_OFF) & (~(1 << 28))) |
		      (enable << 28));

	return 0;
}

__s32 DE_BE_set_display_size(__u32 sel, __u32 width, __u32 height)
{
	DE_BE_WUINT32(sel, DE_BE_DISP_SIZE_OFF,
		      ((height - 1) << 16) | (width - 1));

	return 0;
}

__s32 DE_BE_get_display_width(__u32 sel)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_DISP_SIZE_OFF) & 0x0000ffff;

	return tmp + 1;
}

__s32 DE_BE_get_display_height(__u32 sel)
{
	__u32 tmp;

	tmp = (DE_BE_RUINT32(sel, DE_BE_DISP_SIZE_OFF) & 0xffff0000) >> 16;

	return tmp + 1;
}

__s32 DE_BE_EnableINT(__u8 sel, __u32 irqsrc)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_INT_EN_OFF);
	DE_BE_WUINT32(sel, DE_BE_INT_EN_OFF, tmp | irqsrc);

	return 0;
}

__s32 DE_BE_DisableINT(__u8 sel, __u32 irqsrc)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_INT_EN_OFF);
	DE_BE_WUINT32(sel, DE_BE_INT_EN_OFF, tmp & (~irqsrc));

	return 0;
}

__u32 DE_BE_QueryINT(__u8 sel)
{
	__u32 ret = 0;

	ret = DE_BE_RUINT32(sel, DE_BE_INT_FLAG_OFF);

	return ret;
}

__u32 DE_BE_ClearINT(__u8 sel, __u32 irqsrc)
{
	DE_BE_WUINT32(sel, DE_BE_INT_FLAG_OFF, irqsrc);

	return 0;
}
