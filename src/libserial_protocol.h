/**
******************************************************************************
* @文件		libserial_protocol.h
* @版本		V1.0.0
* @日期
* @概要		串口通信协议
* @作者		lovemengx	email:lovemengx@qq.com
******************************************************************************
* @注意  	All rights reserved
******************************************************************************
*/
#ifndef __LIB_SERIAL_PROTOCOL_H_
#define __LIB_SERIAL_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

// 缓存数据结构(编解码不能同时使用同一块缓存)
typedef struct{
	unsigned char *buf;			// 缓存位置, 由用户指向一块可用的内存空间
	unsigned int total;			// 缓存大小, 标明该内存空间的总长度
}libserial_protocol_buf_t;

/*---------------------------------------------------------------------
*	函数:	libserial_protocol_create
*	功能:	使用接口内部申请指定可用大小的空间
*	参数:	size: 申请可用缓冲区大小 
*	返回:	NULL: 申请内存空间失败		>0: 申请成功
*	备注:	接口内部会多申请内部数据结构所需的空间大小
*---------------------------------------------------------------------*/
libserial_protocol_buf_t *libserial_protocol_create(unsigned int size);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_release
*	功能:	释放接口内部申请的内存空间
*	参数:	splbuf: 由 libserial_protocol_create() 创建的内存空间
*	返回:	0: 不满足最小长度要求  >0: 可供用户使用的大小 
*---------------------------------------------------------------------*/
void libserial_protocol_release(libserial_protocol_buf_t *splbuf);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_internal_size
*	功能:	返回内部数据结构占用字节数
*	参数:	
*	返回:	返回内部数据结构占用字节数 
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_internal_size();

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_init
*	功能:	使用用户提供的或创建接口的缓冲区, 初始化内部数据结构
*	参数:	splbuf: 缓冲区  size: 缓冲区大小 
*	返回:	0: 不满足最小长度要求  >0: 可供用户使用的大小 
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_init(libserial_protocol_buf_t *splbuf);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_reset
*	功能:	重置解码器
*	参数:	splbuf: 缓冲区
*	返回:	无返回值
*---------------------------------------------------------------------*/
void libserial_protocol_decode_reset(libserial_protocol_buf_t *splbuf);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode
*	功能:	解码数据
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 解成功的数据长度
*	返回:	0: 正在解码  1:解码成功  -1: 校验失败	
*---------------------------------------------------------------------*/
int libserial_protocol_decode(libserial_protocol_buf_t *splbuf, unsigned char indata, unsigned int *dalen);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode_find
*	功能:	寻找数据区域(适合较大数据量)
*	参数:	splbuf: 缓冲区  indata: 输入数据
*	返回:	0: 正在寻找  >0: 数据区域长度
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_decode_find(libserial_protocol_buf_t *splbuf, unsigned char indata);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_decode_copy
*	功能:	拷贝数据区域(适合较大数据量)
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 输入的数据长度
*	返回:	0: 完成拷贝  -1: 校验失败  -2: 数据长度或解码状态错误
*---------------------------------------------------------------------*/
int libserial_protocol_decode_copy(libserial_protocol_buf_t *splbuf, unsigned char *indata, unsigned int len);

/*---------------------------------------------------------------------
*	函数: 	libserial_protocol_encode
*	功能:	数据编码
*	参数:	splbuf: 缓冲区  indata: 输入数据  dalen: 输入的数据长度
*	返回:	0: 数据长度不合法  >0: 编码后的数据长度
*---------------------------------------------------------------------*/
unsigned int libserial_protocol_encode(libserial_protocol_buf_t *splbuf, const void *indata, unsigned int dalen);

#ifdef __cplusplus
}
#endif

#endif
