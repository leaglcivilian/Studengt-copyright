#include "define.h"
#include <stdio.h>
#include <winsock2.h>
#include <stdint.h>
#include <stdlib.h>

#define SERVER_IP    "127.0.0.1"  // 服务器IP地址 
#define SERVER_PORT  6666         // 服务器端口号
#define BUFFER sizeof(struct packet)  // 缓冲区大小
#define TIMEOUT 8000   // 超时，单位s
#define WINDOWSIZE 20 // 滑动窗口大小

SOCKADDR_IN addrServer;   // 服务器地址
SOCKADDR_IN addrClient;   // 客户端地址

SOCKET sockServer; // 服务器套接字
SOCKET sockClient; // 客户端套接字

char filepath[50]; // 文件路径
int totalpacket; // 数据包个数
int totalack = 0; // 正确确认的数据包个数
int waitack=1;//ack值
int seqnumber =1040; // 序列号个数
int recvSize;
int length = sizeof(SOCKADDR);
int STATE = 0;

// 初始化工作
void inithandler() {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	
	// 版本 2.2
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		printf("WSAStartup failed with error: %d\n", err);
		return;
	}
	
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return;
	}
	
	sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	// 设置套接字为非阻塞模式
	int iMode = 1; // 1：非阻塞，0：阻塞
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) &iMode); // 非阻塞设置
	
	addrServer.sin_addr.S_un.S_addr =inet_addr(SERVER_IP);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);
	err = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err) {
		err = GetLastError();
		printf("Could not bind the port %d for socket. Error code is %d\n", SERVER_PORT, err);
		WSACleanup();
		return;
	} else {
		printf("服务器创建成功\n");
	}
}

int TimeoutRepeat(struct packet *pkt,struct packet *pkt1,FILE*is ){
	clock_t start, current;
	int waitCount = 0;
	start = clock();
	//进入计时阶段
	while(1){	
		recvSize = recvfrom(sockServer, (char *)pkt1, BUFFER, 0, (SOCKADDR *)&addrClient, &length);
		if(pkt1->ack==1){
			printf("%d",pkt1->signal);
			printf("\n--------携带文件目录的数据包发送无误,对方已经接收--------\n");
			return 1;
		}
		if (pkt1->ack == (pkt->seq)+1) {
			printf("\n--------数据包发送无误,对方已经接收--------\n");
			return 1;
		}
		current = clock();
		if ((double)(current - start) * 1000 / CLOCKS_PER_SEC > TIMEOUT) {
			printf("超时，服务器重新发送数据包\n");
			++waitCount;//重传10次则认为客户端主机异常，会释放连接。
			sendto(sockServer, (char *)pkt, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
			start = clock();//再次启动计时器。
			if (waitCount > 10) {
				printf("重传次数超过上限！程序异常即将退出\n");
				fclose(is);
				closesocket(sockServer);
				WSACleanup();
				free(pkt);
				free(pkt1);
				exit(1);
			}
			
		}
		continue;
	}
}
int main() {
	// 初始化工作
	inithandler();
	//计时器
	clock_t start, current;
	
	// 读文件
	printf("请输入要发送的文件名：");
	fgets(filepath, sizeof(filepath), stdin);
	filepath[strcspn(filepath, "\n")] = '\0';

	FILE *is = fopen(filepath, "rb");  // 以二进制方式打开文件
	if (!is) {
		printf("文件无法打开!\n");
		exit(1);
	}
	fseek(is, 0, SEEK_END);  // 将文件流指针定位到流的末尾
	int length1 = ftell(is);
	totalpacket = length1 / 1024 + 1;
	printf("文件大小为%d Bytes, 总共有%d个数据包\n", length1, totalpacket);
	fseek(is, 0, SEEK_SET);  // 将文件流指针重新定位到流的开始
	
struct packet *pkt = malloc(sizeof(struct packet));
struct packet *pkt1 = malloc(sizeof(struct packet));//当作副本用来重传
	init_packet(pkt);
	// 建立连接
	while (1) {
		 recvSize = recvfrom(sockServer, (char *)pkt, BUFFER, 0, (SOCKADDR *)&addrClient, &length);
		int count = 0;
	if(pkt->signal!= 0) {
			count++;
			Sleep(1000);
			continue;
			if (count > 20) {
				printf("当前没有客户端请求连接！\n");
				count = 0;
				fclose(is);
				closesocket(sockServer);
				WSACleanup();
				free(pkt);
				free(pkt1);
				exit(1);
			}
		}
		
		if (pkt->signal== 0) {
			clock_t st = clock();//开始记录时间
			printf("开始建立连接...\n");
			int stage = 0;
			int runFlag = 1;
			int waitCount = 0;
			
			while (runFlag) {
				switch (stage) {
				case 0:
					// 发送1阶段
					memset(pkt, 0, sizeof(struct packet));
					memset(pkt1, 0, sizeof(struct packet));
					// 移除多余的参数
				   connecthandler(pkt,1);
					pkt->len=totalpacket;
					sendto(sockServer, (char *)pkt, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
					start = clock();
					//进入计时阶段
					while(1){				
						recvSize = recvfrom(sockServer, (char *)pkt1, BUFFER, 0, (SOCKADDR *)&addrClient, &length);
						if (pkt1->signal == 2) {
							printf("三次握手连接已经建立完成\n");
							memset(pkt, 0, sizeof(struct packet));
							memset(pkt1, 0, sizeof(struct packet));
							printf("首先发送携带文件目录的数据包...\n");
							memcpy(pkt->data, filepath, strlen(filepath));
							pkt->len = strlen(filepath);//发送一个数据包，该数据包内容是文件目录。
							pkt->seq=0;//代表是文件目录数据包。
							sendto(sockServer, (char *)pkt, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
							TimeoutRepeat(pkt,pkt1,is);
							memset(pkt, 0, sizeof(struct packet));
							stage = 2;
							printf("文件传输正式开始\n");
							break;
						}
						current = clock();
						if ((double)(current - start) * 1000 / CLOCKS_PER_SEC > TIMEOUT) {
							printf("超时，服务器重新发送三次握手中的第二个数据包\n");
							++waitCount;//重传10次则认为客户端主机异常，会释放连接。
							sendto(sockServer, (char *)pkt, BUFFER, 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
							start = clock();//再次启动计时器。
							if (waitCount > 10) {
								printf("第二次握手连接建立失败！程序异常即将退出\n");
								fclose(is);
								closesocket(sockServer);
								WSACleanup();
								free(pkt);
								free(pkt1);
								exit(1);
							}
							continue;
						}
					}
				case 2:
				
					if (totalpacket == 1) {
						memset(pkt, 0, sizeof(struct packet));
						memset(pkt1, 0, sizeof(struct packet));
						pkt->signal = 444;
						pkt->len= length1;
						pkt->seq=1;
						fread(pkt->data, 1, pkt->len,is);
						printf(" 读取到的数据为 %s  ",pkt->data);
						sendto(sockServer, (char *)pkt, sizeof(*pkt), 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
						TimeoutRepeat(pkt,pkt1,is);
						printf("*************************************\n");
						printf("数据传输成功！\n");
						printf("文件大小为%d Bytes, 总共有%d个数据包\n", length1, totalpacket);
						printf("传输用时: %.2f ms\n", (clock() - st) * 1000.0 / CLOCKS_PER_SEC);
						runFlag = 0;
						fclose(is);
						closesocket(sockServer);
						WSACleanup();
						free(pkt);
						free(pkt1);
						exit(0);
						break;
					}
					if (totalpacket-1== totalack) {
						memset(pkt, 0, sizeof(struct packet));
						memset(pkt1, 0, sizeof(struct packet));
						pkt->signal = 444;
						pkt->len= length1-ftell(is);
						pkt->seq=totalpacket;
						fread(pkt->data, 1, pkt->len,is);
						printf("正在发送第%d个数据包\n",pkt->seq);
						sendto(sockServer, (char *)pkt, sizeof(*pkt), 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
						TimeoutRepeat(pkt,pkt1,is);
						printf("*************************************\n");
						printf("数据传输成功！\n");
						printf("传输用时: %.2f ms\n", (clock() - st) * 1000.0 / CLOCKS_PER_SEC);
						printf("文件大小为%d Bytes, 总共有%d个数据包\n", length1, totalpacket);
						runFlag = 0;
						fclose(is);
						closesocket(sockServer);
						WSACleanup();
						free(pkt);
						free(pkt1);
						exit(0);
						break;
					}
					if(totalack<totalpacket-1)
					{
							memset(pkt, 0, sizeof(struct packet));
							memset(pkt1, 0, sizeof(struct packet));
							pkt->signal =300;//区别连接和断开tag标志
							pkt->seq=waitack;
							pkt->len=1024;
							fread(pkt->data, 1, pkt->len,is);
							printf("正在发送第%d个数据包\n",pkt->seq);
							sendto(sockServer, (char *)pkt, sizeof(*pkt), 0, (SOCKADDR *)&addrClient, sizeof(SOCKADDR));
							TimeoutRepeat(pkt,pkt1,is);
							waitack=pkt1->ack;
							totalack++;
							break;
					}	
					
				}
			}
		}
	}
	
	// 关闭文件和套接字
	fclose(is);
	closesocket(sockServer);
	WSACleanup();
	free(pkt);
	free(pkt1);
	return 0;
}


