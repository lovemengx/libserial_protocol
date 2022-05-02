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
