#ifndef __HDMI_MANAGE_H__
#define __HDMI_MANAGE_H__

int get_saved_hdmi_vendor_id(void);
int edid_checksum(char const *edid_buf);
int hdmi_get_vendor_id(unsigned char *edid_buf);
int hdmi_verify_mode(int channel, int mode, int *vid, int check);

#endif /* #ifndef __HDMI_MANAGE_H__ */
