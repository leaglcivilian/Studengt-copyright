#ifndef __DEFINE_H__
#define __DEFINE_H__

#ifndef __PKT_H__
#define __PKT_H__

/* 使用C标准库而非C++ */
#include <stdlib.h>
#include <time.h>
#include<stdbool.h>
/* 用于Windows网络编程的头文件 */
#include <WinSock2.h>

/* 标准输入输出库 */
#include <stdio.h>

/* 定义了一些整数类型 */
#include <stdint.h>

/* 声明宏以禁用某些警告和安全功能 */
#define _CRT_SECURE_NO_WARNINGS



#endif /* __PKT_H__ */

#include <string.h>

/* 定义packet结构体 */
struct packet {
	unsigned int signal;         /* 连接建立、断开标识 */
	unsigned int seq;          /* 序列号 */
	unsigned int ack;          /* 确认号 */
	unsigned short len;        /* 数据部分长度 */
	unsigned short window;     /* 窗口 */
	char data[1024];           /* 数据长度 */
};

/* 初始化packet */
void init_packet(struct packet *pkt) {
	pkt->signal = -1;
	pkt->seq = -1;
	pkt->ack = -1;
	pkt->len = -1;
	pkt->window = -1;
	memset(pkt->data, 0, 1024);
}
/* 构造packet */
void make_mypkt(struct packet *pkt, unsigned int ack, unsigned short window) {
	pkt->ack = ack;
	pkt->window = window;
}

/* 处理连接 */
struct packet* connecthandler(struct packet*pkt,int signal) {
	if (!pkt) return NULL;
	init_packet(pkt);
	pkt->signal = signal;
	return pkt;
}

#endif /* __DEFINE_H__ */

