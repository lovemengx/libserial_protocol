## libserial_protocol

适用于单片机的串口通信协议基础库

具有以下几种特点：

* 内存空间占用可控，libserial_protocol 可使用静态内存，也可以使用动态内存，内存空间可控。
* 接口简单容易复用，libserial_protocol 采用面向对象方式实现，提供适用于小数据量和大数据量解码方式。


## 接口介绍

```C
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
```

## Sample

```C
#include <stdio.h>
#include "libserial_protocol.h"

#define iprintf(format,...) 	printf("[inf]%s():%05d " format , __func__, __LINE__,##__VA_ARGS__)

/*---------------------------------------------------------------------
*	函数: 	static_libserial_protocol
*	功能:	演示采用静态内存方式进行两种方法解码
*---------------------------------------------------------------------*/
void static_libserial_protocol(const char *data, unsigned int length)
{
	int result = 0;
	unsigned int i = 0;
	unsigned char buf1[512], buf2[512];
	libserial_protocol_buf_t splbuf1, splbuf2;

	// 使用静态内存
	splbuf1.buf = buf1;
	splbuf2.buf = buf2;
	splbuf1.total = sizeof(buf1);
	splbuf2.total = sizeof(buf2);

	// 初始化内部数据结构
	unsigned int avail1 = libserial_protocol_init(&splbuf1);
	unsigned int avail2 = libserial_protocol_init(&splbuf2);
	iprintf("avail1:%d  avail2:%d\n", avail1, avail2);

	// 对数据进行编码
	unsigned int enbyte = libserial_protocol_encode(&splbuf1, data, length);
	iprintf("enbyte:%d  datalen:%d\n", enbyte, length);

	// 使用最简单的方式解码, 适合小数据量
	unsigned int debyte = 0x00;
	for (i = 0; i < enbyte; i++) {
		if ((result = libserial_protocol_decode(&splbuf2, splbuf1.buf[i], &debyte)) == 1) {
			iprintf("simple mode: decode success debyte:%d data:[%s]\n", debyte, splbuf2.buf);
			break;
		}
	}

	// 使用较高性能方式解码, 适合大数据量
	for (i = 0; i < enbyte; i++){
		if ((debyte = libserial_protocol_decode_find(&splbuf2, splbuf1.buf[i])) > 0) {
			if (libserial_protocol_decode_copy(&splbuf2, splbuf1.buf + i + 1, debyte) == 0) {
				iprintf("comple mode: decode success debyte:%d data:[%s]\n", debyte, splbuf2.buf);
			}
			break;
		}
	}

	iprintf("static run done...\n\n");
	return;
}

/*---------------------------------------------------------------------
*	函数: 	static_libserial_protocol
*	功能:	演示采用动态内存方式进行两种方法解码
*---------------------------------------------------------------------*/
void dynamic_libserial_protocol(const char* data, unsigned int length)
{
	int result = 0;
	unsigned int i = 0;
	libserial_protocol_buf_t* splbuf1, * splbuf2;

	// 使用动态内存
	splbuf1 = libserial_protocol_create(512);
	splbuf2 = libserial_protocol_create(512);
	if (!splbuf1 || !splbuf2) {
		iprintf("create dynamic failed.\n");
		libserial_protocol_release(splbuf1);
		libserial_protocol_release(splbuf2);
		return ;
	}

	// 初始化内部数据结构
	unsigned int avail1 = libserial_protocol_init(splbuf1);
	unsigned int avail2 = libserial_protocol_init(splbuf2);
	iprintf("avail1:%d  avail2:%d\n", avail1, avail2);

	// 对数据进行编码
	unsigned int enbyte = libserial_protocol_encode(splbuf1, data, length);
	iprintf("enbyte:%d  datalen:%d\n", enbyte, length);

	// 使用最简单的方式解码, 适合小数据量
	unsigned int debyte = 0x00;
	for (i = 0; i < enbyte; i++) {
		if ((result = libserial_protocol_decode(splbuf2, splbuf1->buf[i], &debyte)) == 1) {
			iprintf("simple mode: decode success debyte:%d data:[%s]\n", debyte, splbuf2->buf);
			break;
		}
	}

	// 使用较高性能方式解码, 适合大数据量
	for (i = 0; i < enbyte; i++) {
		if ((debyte = libserial_protocol_decode_find(splbuf2, splbuf1->buf[i])) > 0) {
			if (libserial_protocol_decode_copy(splbuf2, splbuf1->buf + i + 1, debyte) == 0) {
				iprintf("comple mode: decode success debyte:%d data:[%s]\n", debyte, splbuf2->buf);
			}
			break;
		}
	}

	// 释放内存空间
	libserial_protocol_release(splbuf1);
	libserial_protocol_release(splbuf2);

	iprintf("dynamic run done...\n\n");
	return;
}

int main(int argc, char* argv[])
{
	char data[256] = { 0 };

	// 填充数据, 最后一字节存储 '\0'
	for (unsigned int i = 0, j = 0; i < sizeof(data) - 1; i++) {
		data[i] = '0' + j;
		j = '9' == data[i] ? 0 : j + 1;
	}

	static_libserial_protocol(data, sizeof(data));
	dynamic_libserial_protocol(data, sizeof(data));
	
	return 0;
}

```
