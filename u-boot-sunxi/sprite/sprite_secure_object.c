/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * Allwinner secure storage data format
 */
#include <common.h>
#include <command.h>
#include <asm-generic/errno.h>
#include <malloc.h>
#include <asm/io.h>
#include <securestorage.h>
#include <sunxi_board.h>
#include <smc.h>
#include "sprite_storage_crypt.h"
#include <asm/arch/ss.h>
#include <linux/ctype.h>

DECLARE_GLOBAL_DATA_PTR;

#include "sprite_storage_crypt.h"

extern int smc_set_sst_crypt_name(char *name); 

static  char write_protect_item[MAX_OEM_STORE_NUM][64] ;

static int __write_protect_item_id(const char *item)
{

	return 0 ;  //default can't be place in burn-key flow
#if 0
	int id ;
	for(id =0; id <MAX_OEM_STORE_NUM; id ++ ){
		if( write_protect_item[id][0] != 0 && \
			!strncmp(item, write_protect_item[id],strnlen(item, 64) )){
			return id ;
		}
	}
	return -1 ;
#endif 
}

static int __set_write_protect_name(const char *name)
{
	int id ;
	
	if( (id = __write_protect_item_id(name)) >0 )
		return 0 ;/*The name had been set*/

	printf("set name[%s] to write_protect list\n", name);

	for(id =0; id <MAX_OEM_STORE_NUM; id ++ ){
		if( write_protect_item[id][0] == 0 ){
			memcpy(write_protect_item[id],name, strnlen(name, 64) );
			break ;
		}
	}

	return 0 ;
}
//#define _SO_TEST_ 

/*Store source data to secure_object struct
 *
 * src		: payload data to secure_object
 * name		: input payloader data name
 * len		: input payload data length
 * tagt		: taregt secure_object
 * retLen	: target secure_object length
 * */
static int wrap_secure_object(void * src, const char *name,  unsigned int len, void * tagt, int *retLen )
{
	store_object_t *obj;

	if(len >MAX_STORE_LEN){
		printf("Input length larger then secure object payload size\n");
		return -1 ;
	}

	obj = (store_object_t *) tagt ;
	*retLen= sizeof( store_object_t );

	obj->magic = STORE_OBJECT_MAGIC ;
	strncpy( obj->name, name, 64 );
	obj->re_encrypt = 0 ;
	obj->version = SUNXI_SECSTORE_VERSION;
	obj->id = 0;
	obj->write_protect = (__write_protect_item_id(name) <0) ? 0:STORE_WRITE_PROTECT_MAGIC;
	memset(obj->reserved, 0, ARRAY_SIZE(obj->reserved) );
	obj->actual_len = len ;
	memcpy( obj->data, src, len);
	
	obj->crc = crc32( 0 , (void *)obj, sizeof(*obj)-4 );

	return 0 ;
}

/*load source data to secure_object struct
 *
 * src		: secure_object 
 * len		: secure_object buffer len 
 * payload	: taregt payload 
 * retLen	: target payload actual length
 * */
static int unwrap_secure_object(void * src,  unsigned int len, void * payload,   int *retLen )
{
	store_object_t *obj;

	if(len != sizeof(store_object_t)){
		printf("Input length not equal secure object size 0x%x\n",len);
		return -1 ;
	}

	obj = (store_object_t *) src ;

	if( obj->magic != STORE_OBJECT_MAGIC ){
		printf("Input object magic fail [0x%x]\n", obj->magic);
		return -1 ;
	}
	
	if( obj->re_encrypt == STORE_REENCRYPT_MAGIC){
		printf("secure object is encrypt by chip\n");
	}

	if( obj->crc != crc32( 0 , (void *)obj, sizeof(*obj)-4 ) ){
		printf("Input object crc fail [0x%x]\n", obj->crc);
		return -1 ;
	}

	memcpy(payload, obj->data ,obj->actual_len);
	*retLen = obj->actual_len ;

	return 0 ;
}

int sunxi_secure_object_read(const char *item_name, char *buffer, int buffer_len, int *data_len)
{
	char secure_object[4096];
	int retLen ,ret ;
	store_object_t *so ;

	memset(secure_object, 0, 4096);

	ret = sunxi_secure_storage_read(item_name, secure_object, 4096, &retLen);
	if(ret){
		printf("sunxi storage read fail\n");
		return -1 ;
	}
	so = (store_object_t *)secure_object;

#if 0
    //data in secure storage does not need decrypt
	ret = smc_tee_ssk_decrypt(so->name, (char *)so->data,so->actual_len);
	if(ret <0){
		printf("smc load sst decrypt fail\n");
		return -1 ;
	}else if(ret >0){
		printf("Don't need to decrypt object\n");
	}else
		printf("Decrypt request done\n");
	
#endif
	return unwrap_secure_object((char *)so, retLen, buffer, data_len);
}
int sunxi_secure_object_set( const char *name , int encrypt, int write_protect, int res1, int res2, int res3)
{
	if(encrypt)	
		smc_set_sst_crypt_name((char *)name);
	if(write_protect)
		__set_write_protect_name(name);

	return 0;

}
int sunxi_secure_object_write(const char *item_name, char *buffer, int length)
{
	char secure_object[4096];
	int retLen ,ret;
	store_object_t *so ;

	memset(secure_object, 0, 4096);
	ret = wrap_secure_object((void *)buffer, item_name, length,secure_object,&retLen);
	if(ret <0 || retLen >4096){
		printf("wrap fail before secure storage write\n");
		return -1 ;
	}
	so = (store_object_t *)secure_object;

#if 0
	//data in secure storage does not need decrypt
	store_object_t *en_so ;
	en_so = malloc(sizeof(*en_so));
	if(!en_so){
		printf("wrap fail out of memory\n");
		return -1 ;
	}

	ret = smc_tee_ssk_encrypt(
			so->name,
			(char *)so->data,so->actual_len, 
			(char *)en_so, (unsigned int *)&retLen);
	if(ret <0){
		printf("smc load sst encrypt fail\n");
		free(en_so);
		return -1 ;
	}else if(ret >0){
		printf("Don't need to encrypt object\n");
	}else{
		memcpy(so->data, en_so, retLen);
		so->actual_len = retLen;
		so->id = sst_oem_item_id((char *)item_name); 
		so->re_encrypt = STORE_REENCRYPT_MAGIC;
		so->write_protect = (__write_protect_item_id(item_name) <0) ? 0:STORE_WRITE_PROTECT_MAGIC;
		so->crc = crc32( 0 , (void *)so, sizeof(*so)-4 ); 
	}

	free(en_so);
#endif 
	return sunxi_secure_storage_write(item_name, (char *)so,sizeof(*so) );
}

/*
 * Test data and function
 */ 
#ifdef _SO_TEST_ 
static unsigned char hdmi_data[]={
	0x44,0xe5,0xb5,0xac,0x2b,0x53,0xbc,0xb9,0xbf,0x89,0x67,0x96,0x1e,0xbb,0xbd,0xfb
};

static unsigned char widevine_data[] ={
	0xeb,0x8f,0x55,0x26,0x0d,0x7a,0xab,0xf3,0x58,0x3b,0xf9,0xc0,0x5e,0x12,0x79,0x85
};

static unsigned char sec_buf[4096];
static int secure_object_op_test(void)
{
	unsigned char * tagt ;
	int LEN = 4096 , retLen ;
	int ret ;

	tagt = (unsigned char *)malloc(LEN);
	if(!tagt){
		printf("out of memory\n");
		return -1 ;
	}

	ret = wrap_secure_object( hdmi_data, "HDMI",  
							ARRAY_SIZE(hdmi_data), tagt, &retLen ) ;
	if( ret <0){
		printf("Error: wrap secure object fail\n");
		free(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_write( "HDMI" , (void *)tagt, retLen )	;
	if(ret <0){
		printf("Error: store HDMI object fail\n");
		free(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_read( "HDMI" , (void *)sec_buf, 
			4096 , (int *)&retLen )	;
	if(ret <0){
		printf("Error: store HDMI object read fail\n");
		free(tagt);
		return -1 ;
	}
	
	if( memcmp(tagt, sec_buf, retLen ) !=0 ){
		printf("Error: HDMI write/read fail\n");
		return -1 ;
	}
	
	printf("HDMI dump:\n");
			print_buffer((u32)sec_buf,
					(void*)sec_buf,
					 1,
					 retLen, 
					 16);

	ret = wrap_secure_object(widevine_data, "Widevine",  
							ARRAY_SIZE(widevine_data), tagt, &retLen ) ;
	if( ret <0){
		printf("Error: wrap secure object fail\n");
		free(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_write( "Widevine" , (void *)tagt, retLen )	;
	if(ret <0){
		printf("Error: store Widevine object fail\n");
		free(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_read( "Widevine" , (void *)sec_buf, 
			4096 , (int *)&retLen )	;
	if(ret <0){
		printf("Error: store Widevine object read fail\n");
		free(tagt);
		return -1 ;
	}
	
	if( memcmp(tagt, sec_buf, retLen ) !=0 ){
		printf("Error: Widevine write/read fail\n");
		return -1 ;
	}
			
	printf("Widevine dump:\n");
	print_buffer((u32)sec_buf,
					(void*)sec_buf,
					 1,
					 retLen, 
					 16);
	sunxi_secure_storage_exit();
		
	free(tagt);
	return 0 ;
}
#endif 

extern int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len);
extern int sunxi_secstorage_write(int item, unsigned char *buf, unsigned int len);

static unsigned char _inner_buffer[4096];
static int clear_secure_store(int index)
{
	memset( _inner_buffer, 0, 4096);

	if(index == 0xffff){
		int i =0 ;
		printf("clean whole secure store");
		for( ; i<32 ;i++){
			printf("..");
		sunxi_secstorage_write(i, (void *)_inner_buffer, 4096);
			printf("clearn %d done\n",i);
		}

	}else{
		printf("clean secure store %d\n", index);
		printf("..");
		sunxi_secstorage_write(index, (void *)_inner_buffer, 4096);
		printf("clearn %d done\n",index);
	}
	return 0;
}

static int sunxi_secure_object_list(void)
{
	int ret, index = 1;
	unsigned char *buf_start = _inner_buffer;
	unsigned char  buffer[4096];
	int retLen;

	if(sunxi_secure_storage_init())
	{
		printf("%s secure storage init err\n", __func__);

		return -1;
	}
	
	if( sunxi_secstorage_read( 0 , _inner_buffer,4096  )<0){
		printf("read map fail\n");
		return -1 ;
	}

	char  name[64], length[32];
	int   i,j, len;

	printf("Map: \n");
	sunxi_dump(_inner_buffer,0x100 );

	while(*buf_start != '\0')
	{
		memset(name, 0, 64);
		memset(length, 0, 32);
		i=0;
		while(buf_start[i] != ':')
		{
			name[i] = buf_start[i];
			i ++;
		}
		i ++;j=0;
		while( (buf_start[i] != ' ') && (buf_start[i] != '\0') )
		{
			length[j] = buf_start[i];
			i ++;j++;
		}

		len = simple_strtoul((const char *)length, NULL, 10);
		printf("name in map %s, len 0x%x\n", name,len);
		memset(buffer, 0, 4096);

		if( !strncmp("key_burned_flag", name, strlen("key_burned_flag") ))
			ret = sunxi_secure_storage_read(name, (void *)buffer,4096, &retLen);
		else
			ret = sunxi_secure_object_read(name, (void *)buffer, 4096, &retLen);
		if(ret < 0)
		{
			printf("get secure storage index %d err\n", index);

			return -1;
		}
		else if(ret > 0)
		{
			printf("the secure storage index %d is empty\n", index);

			return -1;
		}
		else
		{
			printf("%d data:\n", index);
			sunxi_dump(buffer, retLen);
		}
		index ++;
		buf_start += strlen((const char *)buf_start) + 1;
	}

	return 0;
}
static int dump_secure_store(char *type)
{
	if( !memcmp(type, "pure",4 ) ){
		return sunxi_secure_object_list();	
	}else
		return sunxi_secure_storage_list();
}

static int cmd_secure_object(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = -1;
	
	if(argc >3 || argc <1){
		printf("wrong argc\n");
		return -1 ;
	}

	if( sunxi_secure_storage_init() < 0  ){
		printf("secure storage init fail\n");
		return -1 ;
	}

	if ( (strncmp("clean", argv[1],strlen("clean")) == 0)){
		if( strncmp("all", argv[2], strlen("all") ) ==0  ){
			ret = clear_secure_store(0xffff);
		}else{
			unsigned int index = simple_strtoul( (const char *)argv[2], NULL, 10 ) ;
			ret = clear_secure_store(index) ; 
		}
	}else if(strncmp("dump", argv[1], strlen("dump"))== 0){
		ret = dump_secure_store(argv[2]);
	}else if( (strncmp("test", argv[1],strlen("test")) == 0) ) {
		#ifdef _SO_TEST_
		ret = secure_object_op_test();
		#else
		ret = cmd_usage(cmdtp);
		#endif
	}else if(strncmp("crypt", argv[1], strlen("crypt"))== 0){
#ifdef CONFIG_SUNXI_SECURE_SYSTEM
		extern int smc_load_sst_test(void);
		smc_load_sst_test();
#endif
		ret = 0 ;
	}else
		ret = cmd_usage(cmdtp);

	if( sunxi_secure_storage_exit() < 0  ){
		printf("secure storage exit fail\n");
		return -1 ;
	}
	return ret;
}



int sunxi_secure_object_down( const char *name , char *buf, int len, int encrypt, int write_protect)
{
	int  ret, i, align_len = 0;
	char lower_name[64];
	sunxi_secure_storage_info_t secdata;

	if (len > 4096) {
		printf("the input key is too long!\n");

		return -1;
	}
	memset(&secdata, 0, sizeof(secdata));
	i = 0;
	memset(lower_name, 0, 64);
	while(name[i] != '\0')
	{
		lower_name[i] = tolower(name[i]);
		i++;
	}
	strcpy(secdata.name, lower_name);
	secdata.encrypted = encrypt;
	secdata.write_protect = write_protect;
	secdata.len = len;
	if(gd->securemode == SUNXI_SECURE_MODE_WITH_SECUREOS)
	{
		if(!strcmp("hdcpkey", secdata.name))
			ret = smc_tee_rssk_encrypt(secdata.key_data, buf, len,&align_len);
		else
			ret = smc_tee_ssk_encrypt(secdata.key_data, buf, len,&align_len);
		if (ret)
		{
			printf("ssk encrypt failed\n");
			return -1;
		}
		secdata.len = align_len;
		sunxi_dump(secdata.key_data, secdata.len);
	}
	else
	{
		debug("no secure os, data can't be encrypted\n");
		memcpy(secdata.key_data,buf,len);
	}

	ret = sunxi_secure_object_write(lower_name, (void*)&secdata,
		SUNXI_SECURE_STORTAGE_INFO_HEAD_LEN + secdata.len);
	if (ret) {
		printf("secure storage write fail\n");

		return -1;
	}

	return 0;
}

int sunxi_secure_object_up(const char *name,char *buf,int len)
{
	sunxi_secure_storage_info_t secdata;
	int data_len;
	int ret;

	memset(&secdata, 0, sizeof(secdata));
	ret = sunxi_secure_object_read(name, (char *)&secdata, sizeof(secdata), &data_len);
	if (ret)
	{
		printf("secure storage read fail\n");
		return -1;
	}
	if(buf)
	{
		memcpy(buf, &secdata, len < sizeof(secdata) ? len:sizeof(secdata));
	}
	return 0;
}

U_BOOT_CMD(
	sunxi_so, 3, 1,	cmd_secure_object,
	"sunxi_so sub-system",
	"sunxi_so <cmd> \n"
	"\t Allwinner secure object storage \n"
	);

//extern int smc_tee_ssk_decrypt(char *out_buf, char *in_buf, int len);

#if defined(CONFIG_SUNXI_SECURE_STORAGE)
int sunxi_widevine_keybox_install(void)
{
	int ret = -1;
	char buffer[4096];
	//char en_buf[4096];
	//char de_buf[4096];
	int  data_len;
    sunxi_secure_storage_info_t *secdata;
	int workmode = uboot_spare_head.boot_data.work_mode;

	if(workmode != WORK_MODE_BOOT)
		return 0;

	if(gd->securemode != SUNXI_SECURE_MODE_WITH_SECUREOS)
		return 0;
		
	if( sunxi_secure_storage_init() < 0  ) {
		printf("secure storage init fail\n");
		ret = -1;
		goto out ;
	}

	memset(buffer, 0, 4096);
	ret = sunxi_secure_object_up("widevine", buffer, 4096);
	if (ret) {
		printf("secure storage read fail\n");
		ret = -1;
		goto out ;
	}

    secdata = (sunxi_secure_storage_info_t *)buffer;
    data_len  = secdata->len;
//	printf("total data len=%d\n", data_len);
//	printf("source data_len=%d\n", secdata->len);
//	sunxi_dump(secdata->key_data, secdata->len);

//	ret = smc_tee_ssk_decrypt(en_buf, secdata->key_data, secdata->len);
//	if (ret) {
//		printf("secure storage decrypt fail\n");
//		return -1;
//	}
//
//	printf("en data data_len=%d\n", secdata->len);
//	sunxi_dump(en_buf, secdata->len);
//
//	printf("secure os en\n");
//	memset(de_buf, 0, 4096);
//	ret = smc_tee_ssk_encrypt(de_buf, en_buf, secdata->len);
//	if (ret) {
//		printf("secure storage encrypt fail\n");
//		return -1;
//	}
//	sunxi_dump(de_buf, secdata->len);

//	printf("test\n");
//	sunxi_dump(buffer, data_len);
	ret = smc_tee_keybox_store("widevine", buffer, data_len);
	if (!ret)
		printf("key install finish\n");
	else
		printf("key install fail\n");

out:
	if( ret <0 ) 
		printf("Warnning:Widevine key install fail !!!\n");

	return 0;
}

#endif
