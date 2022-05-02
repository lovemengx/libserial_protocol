/**
******************************************************************************
* @文件		libserial_protocol.c
* @版本		V1.0.0
* @日期
* @概要		串口通信协议
* @作者		lovemengx	email:lovemengx@qq.com
******************************************************************************
* @注意  	All rights reserved
******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libserial_protocol.h"

#define eprintf(format,...)		printf("[err]%s():%05d " format , __func__, __LINE__,##__VA_ARGS__)

// 数据包头和数据缓冲区长度定义
#define SERIAL_PROTOCOL_HEAD0				'L'
#define SERIAL_PROTOCOL_HEAD1				'M'
#define SERIAL_PROTOCOL_HEAD2				'X'

// 用于解码的状态定义, 使用枚举类型不能确定长度, 因此使用宏定义
#define PROTOCOL_STATE_HEAD0				1
#define PROTOCOL_STATE_HEAD1				2
#define PROTOCOL_STATE_VERIFY				3
#define PROTOCOL_STATE_LENGTH_0				4
#define PROTOCOL_STATE_LENGTH_1				5
#define PROTOCOL_STATE_LENGTH_2				6
#define PROTOCOL_STATE_LENGTH_3				7
#define PROTOCOL_STATE_DATA					8

// 缓冲区描述信息
#pragma pack(1)
typedef struct {
	unsigned char *buf;				// 存储数据的位置
	unsigned int space;				// 减去编解码所需的大小
}protocol_buffer_t;	
#pragma pack()	
	
// 解码相关	
#pragma pack(1)	
typedef struct{	
	unsigned char 	sta;			// 记录当前解码的状态
	unsigned int  	len;			// 记录拷贝数据区域的数据长度
}protocol_status_t;	
#pragma pack()	
	
// 头部信息
#pragma pack(1)	
typedef struct {	
	unsigned char mark[3];			// 帧头标志, 用于数据封包解包
	unsigned char verify;			// 数据校验, 用于校验数据完整
	unsigned int  length;			// 数据长度, 用于确定数据长度
}protocol_pkthead_t;
#pragma pack()

// 编解码所需的数据结构
#pragma pack(1)
typedef struct {
	protocol_pkthead_t	head;		// 头部信息
	protocol_buffer_t 	buff;		// 缓存信息
	protocol_status_t	state;		// 解码状态机
}protocol_packate_t;
#pragma pack()

/*---------------------------------------------------------------------
*	函数: 	data_verify_check
*	功能:	校验数据并输出校验码
*	参数:	data: 输入数据  len: 输入数据长度 
*	返回:	校验码
*---------------------------------------------------------------------*/
static inline unsigned char data_verify_check(const void *data, unsigned int len)
{
	unsigned char check = 0x00;
	for(unsigned int i = 0; i < len; i++){
		check = check + ((unsigned char*)data)[i];
	}
	check = 0x01 + (check) + 'X';
	return check;
}

/*---------------------------------------------------------------------
*	函数: 	get_protocol_packate
*	功能:	从用户提供的内存中计算数据结构的位置
*	参数:	splbuf: 用户提供的内存 
*	返回:	数据结构的位置
*---------------------------------------------------------------------*/
static inline protocol_packate_t *get_protocol_packate(libserial_protocol_buf_t *splbuf)
{
	return (protocol_packate_t *)(splbuf->buf + splbuf->total - sizeof(protocol_packate_t));
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_internal_size
*	功能:	返回内部数据结构占用字节数
*	参数:	
*	返回:	返回内部数据结构占用字节数 
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_internal_size()
{
	return sizeof(protocol_packate_t);
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_create
*	功能:	使用接口内部申请指定可用大小的空间
*	参数:	size: 申请可用缓冲区大小 
*	返回:	NULL: 申请内存空间失败		>0: 申请成功
*---------------------------------------------------------------------*/
libserial_protocol_buf_t *libserial_protocol_create(unsigned int size)
{
	libserial_protocol_buf_t *splbuf = NULL;
	
	if((splbuf = (libserial_protocol_buf_t *)malloc(sizeof(libserial_protocol_buf_t))) == NULL){
		return NULL;
	}
	
	splbuf->total = size + libserial_protocol_internal_size();
	if((splbuf->buf = (unsigned char *)malloc(splbuf->total)) == NULL){
		eprintf("malloc failed %d byte\n", splbuf->total);
		free(splbuf);
		return NULL;
	}

	return splbuf;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_release
*	功能:	释放接口内部申请的内存空间
*	参数:	splbuf: 由 libserial_protocol_create() 创建的内存空间
*	返回:	0: 不满足最小长度要求  >0: 可供用户使用的大小 
*---------------------------------------------------------------------*/
void libserial_protocol_release(libserial_protocol_buf_t *splbuf)
{
	if (splbuf) {
		splbuf->total = 0;
		free(splbuf->buf);
		free(splbuf);
	}
	return ;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_init
*	功能:	使用用户提供的缓冲区, 初始化内部数据结构
*	参数:	splbuf: 缓冲区  size: 缓冲区大小 
*	返回:	0: 不满足最小长度要求  >0: 可供用户使用的大小 
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_init(libserial_protocol_buf_t *splbuf)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	
	// 判断缓存大小是否满足最小要求
	memset(splbuf->buf, 0x00, splbuf->total);
	if(splbuf->total < sizeof(protocol_packate_t) + 1){
		return 0;
	}
	
	// 复位解码器状态
	pkt->state.len = 0x00;
	pkt->state.sta = PROTOCOL_STATE_HEAD0;
	
	// 设置缓存信息和计算可使用的空间大小
	pkt->buff.buf	= splbuf->buf;
	pkt->buff.space = splbuf->total - sizeof(protocol_packate_t);

	return pkt->buff.space;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_reset
*	功能:	重置解码器
*	参数:	splbuf: 缓冲区
*	返回:	无返回值
*---------------------------------------------------------------------*/
void libserial_protocol_decode_reset(libserial_protocol_buf_t *splbuf)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	pkt->state.len = 0x00;
	pkt->state.sta = PROTOCOL_STATE_HEAD0;
	return ;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode_find
*	功能:	寻找数据区域(适合较大数据量)
*	参数:	splbuf: 缓冲区  indata: 输入数据
*	返回:	0: 正在寻找  >0: 数据区域长度
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_decode_find(libserial_protocol_buf_t *splbuf, unsigned char indata)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	
	// 采用状态机方式进行解码
	switch(pkt->state.sta){
		case PROTOCOL_STATE_HEAD0:		
			pkt->state.len = 0x00;
			pkt->head.length = 0x00;
			pkt->head.mark[0] = indata;
			pkt->state.sta = (SERIAL_PROTOCOL_HEAD0 != indata) ? PROTOCOL_STATE_HEAD0 : PROTOCOL_STATE_HEAD1;			
			break;
		case PROTOCOL_STATE_HEAD1:		
			pkt->head.mark[1] = indata;
			pkt->state.sta = (SERIAL_PROTOCOL_HEAD1 != indata) ? PROTOCOL_STATE_HEAD0 : SERIAL_PROTOCOL_HEAD2;
			break;
		case SERIAL_PROTOCOL_HEAD2:		
			pkt->head.mark[2] = indata;
			pkt->state.sta = (SERIAL_PROTOCOL_HEAD2 != indata) ? PROTOCOL_STATE_HEAD0 : PROTOCOL_STATE_VERIFY;
			break;
		case PROTOCOL_STATE_VERIFY:		
			pkt->head.verify = indata;
			pkt->state.sta = PROTOCOL_STATE_LENGTH_0;
			break;
		case PROTOCOL_STATE_LENGTH_0:	
			pkt->head.length |= indata << 0;
			pkt->state.sta = PROTOCOL_STATE_LENGTH_1;
			break;
		case PROTOCOL_STATE_LENGTH_1:	
			pkt->head.length |= indata << 8;
			pkt->state.sta = PROTOCOL_STATE_LENGTH_2;
			break;	
		case PROTOCOL_STATE_LENGTH_2:	
			pkt->head.length |= indata << 16;
			pkt->state.sta = PROTOCOL_STATE_LENGTH_3;
			break;
		case PROTOCOL_STATE_LENGTH_3:	
			pkt->head.length |= indata << 24;
			pkt->state.sta = (pkt->head.length > pkt->buff.space) ? PROTOCOL_STATE_HEAD0 : PROTOCOL_STATE_DATA;
			break;
		default: pkt->state.sta = PROTOCOL_STATE_HEAD0;
	}
	
	if(PROTOCOL_STATE_DATA == pkt->state.sta){
		return pkt->head.length;
	}
	return 0;	
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode
*	功能:	解码数据
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 解成功的数据长度
*	返回:	0: 正在解码  1:解码成功  -1: 校验失败	
*---------------------------------------------------------------------*/
int libserial_protocol_decode(libserial_protocol_buf_t *splbuf, unsigned char indata, unsigned int *dalen)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	
	// 如果还没有找到数据区域就继续寻找
	if(PROTOCOL_STATE_DATA != pkt->state.sta){
		libserial_protocol_decode_find(splbuf, indata);
		return 0;
	}
	
	// 找到数据区域后开始逐个字节拷贝数据
	if(pkt->state.len < pkt->head.length){
		pkt->buff.buf[pkt->state.len++] = indata;
		if(pkt->state.len < pkt->head.length){
			return 0;
		}
	}
	
	// 重置解码器后对数据进行校验
	libserial_protocol_decode_reset(splbuf);
	if(pkt->head.verify == data_verify_check(pkt->buff.buf, pkt->head.length)){
		*dalen = pkt->head.length;
		return 1;
	}
	return -1;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode_copy
*	功能:	拷贝数据区域(适合较大数据量)
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 输入的数据长度
*	返回:	0: 完成拷贝  -1: 校验失败  -2: 数据长度或解码状态错误
*---------------------------------------------------------------------*/
int libserial_protocol_decode_copy(libserial_protocol_buf_t *splbuf, unsigned char *indata, unsigned int dalen)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	
	// 检查当前解码状态和数据长度
	if(pkt->state.sta != PROTOCOL_STATE_DATA || dalen != pkt->head.length){
		eprintf("pkt->state.sta(%d) != PROTOCOL_STATE_DATA || dalen(%d) != pkt->head.length(%d)\n",pkt->state.sta, dalen, pkt->head.length);
		return -2;
	}
	
	// 重置解码器后拷贝数据并校验数据
	libserial_protocol_decode_reset(splbuf);
	memcpy(pkt->buff.buf, indata, pkt->head.length);
	if(data_verify_check(pkt->buff.buf, pkt->head.length) != pkt->head.verify){
		eprintf("verify failed\n");
		return -1;
	}
	
	return  0;
}

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_encode
*	功能:	数据编码
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 输入的数据长度
*	返回:	0: 数据长度不合法  >0: 编码后的数据长度
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_encode(libserial_protocol_buf_t *splbuf, const void *indata, unsigned int dalen)
{
	protocol_packate_t *pkt = get_protocol_packate(splbuf);
	protocol_pkthead_t *hd  = (protocol_pkthead_t *)pkt->buff.buf;
	
	if(dalen > pkt->buff.space){
		eprintf("dalen(%d) > pkt->buff.space(%d)\n", dalen, pkt->buff.space);
		return 0;
	}
	
	hd->mark[0] = SERIAL_PROTOCOL_HEAD0;
	hd->mark[1] = SERIAL_PROTOCOL_HEAD1;
	hd->mark[2] = SERIAL_PROTOCOL_HEAD2;
	hd->length  = dalen;
	hd->verify  = data_verify_check(indata, dalen);
	memcpy(pkt->buff.buf + sizeof(protocol_pkthead_t), indata, dalen);

	return hd->length + sizeof(protocol_pkthead_t);
}

