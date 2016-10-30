#include "dev_manage.h"
#include "hdmi_manage.h"

int get_saved_hdmi_vendor_id(void)
{
	char data_buf[512] = {0};
	char *pvendor_id, *pdata, *pdata_end;
	int vendor_id = 0;
	int i = 0;
	int ret = 0;

	ret = hal_fat_fsload(DISPLAY_PARTITION_NAME, "tv_vdid.fex", data_buf, 512);
	if (0 >= ret)
		return 0;

	pvendor_id = data_buf;
	pdata = data_buf;
	pdata_end = pdata + ret;
	for (; (i < 4) && (pdata != pdata_end); pdata++) {
		if ('\n' == *pdata) {
			*pdata = '\0';
			ret = (int)simple_strtoul(pvendor_id, NULL, 16);
			vendor_id |= ((ret & 0xFF) << (8 * (3 - i)));
			printf("pvendor_id=%s, ret=0x%x, vendorID=0x%x\n",
				pvendor_id, ret, vendor_id);
			pvendor_id = pdata + 1;
			i++;
		}
	}
	return vendor_id;
}

int edid_checksum(char const *edid_buf)
{
#define EDID_LENGTH 0x80
	char csum = 0;
	char all_null = 0;
	int i = 0;

	for (i = 0; i < EDID_LENGTH; i++) {
		csum += edid_buf[i];
		all_null |= edid_buf[i];
	}

	if (csum == 0x00 && all_null) {
	/* checksum passed, everything's good */
		return 0;
	} else if (all_null) {
		printf("edid all null\n");
		return -2;
	} else {
		printf("edid checksum err\n");
		return -1;
	}
}

int hdmi_get_vendor_id(unsigned char *edid_buf)
{
#define ID_VENDOR 0x08
	int vendor_id = 0;
	unsigned char *pVendor = NULL;

	pVendor = edid_buf + ID_VENDOR;
	vendor_id = (pVendor[0] << 24);
	vendor_id |= (pVendor[1] << 16);
	vendor_id |= (pVendor[2] << 8);
	vendor_id |= pVendor[3];
	printf("vendor_id=%08x\n", vendor_id);
	return vendor_id;
}

int hdmi_verify_mode(int channel, int mode, int *vid, int check)
{
	/* self-define hdmi mode list */
	const int HDMI_MODES[] = {
		DISP_TV_MOD_720P_60HZ,
		DISP_TV_MOD_720P_50HZ,
		DISP_TV_MOD_1080P_60HZ,
		DISP_TV_MOD_1080P_50HZ,
	};
	int i = 0;
	unsigned char edid_buf[256] = {0};
	int actual_vendor_id = 0;
	int saved_vendor_id = 0;

	hal_get_hdmi_edid(channel, edid_buf, 128); /* here we only get edid block0 */

	if (-2 == edid_checksum((char const *)edid_buf))
		check = 0;

	actual_vendor_id = hdmi_get_vendor_id(edid_buf);
	saved_vendor_id = get_saved_hdmi_vendor_id();
	*vid = actual_vendor_id ? actual_vendor_id : saved_vendor_id;
	if (2 == check) {
		/* if vendor id change , check mode: check = 1 */
		if (actual_vendor_id && (actual_vendor_id != saved_vendor_id)) {
			check = 1;
			*vid = actual_vendor_id;
			printf("vendor:0x%x, saved_vendor:0x%x\n",
				actual_vendor_id, saved_vendor_id);
		}
	}

	/* check if support the output_mode by television,
	 * return 0 is not support */
	if ((1 == check)
		&& (1 != hal_is_support_mode(channel,
			DISP_OUTPUT_TYPE_HDMI, mode))) {
		/* any mode of HDMI_MODES is not supported. fixme: use HDMI_MODES[0] ? */
		mode = HDMI_MODES[0];
		for (i = 0; i < sizeof(HDMI_MODES) / sizeof(HDMI_MODES[0]); i++) {
			if (1 == hal_is_support_mode(channel,
				DISP_OUTPUT_TYPE_HDMI, HDMI_MODES[i])) {
				printf("find mode[%d] in HDMI_MODES\n", HDMI_MODES[i]);
				mode = HDMI_MODES[i];
				break;
			}
		}
	}

	hal_save_int_to_kernel("tv_vdid", *vid);

	return mode;
}


