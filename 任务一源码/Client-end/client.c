#include "define.h"
#include <stdio.h>   // 用于标准输入输出功能
#include <winsock2.h> // Windows Socket API
#include <stdint.h>   // 标准整数类型
#include <time.h>
#define SERVER_PORT  6666 // 接收数据的端口号
#define SERVER_IP    "127.0.0.1" // 服务器的IP地址
#define BUFFER sizeof(struct packet)

SOCKET socketClient; // 客户端套接字
SOCKADDR_IN addrServer; // 服务器地址

char filename[50];
int waitseq = 1; // 等待的数据包
int totalpacket; // 数据包总数
int len = sizeof(SOCKADDR);
int totalrecv = 0;
int seqnumber=1030;

// 初始化
void init() {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		// 找不到winsock.dll
		printf("WSAStartup failed with error: %d\n", err);
		return;
	}
	
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
	} else {
		printf("套接字创建成功\n");
	}
	
	socketClient = socket(AF_INET, SOCK_DGRAM, 0);
	addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);
}
int main(){
	init();
	FILE *out_result=NULL;
	struct packet *pkt = malloc(sizeof(struct packet));
	struct packet *pkt1 = malloc(sizeof(struct packet));
	init_packet(pkt1);
	init_packet(pkt);
	int stage = 0;
	while(1) {
		init_packet(pkt);
		pkt->signal = 0;
		Sleep(3000);
		sendto(socketClient, (char*)pkt, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
		printf("发送三次握手中第一个数据包\n");
		while(1){
			switch (stage) {
			case 0:
				recvfrom(socketClient, (char*)pkt1, sizeof(*pkt1), 0, (SOCKADDR*)&addrServer, &len);
				printf("接收到来自服务器的第一次ACK应答，第二次握手完成\n");
				if(pkt1->signal!=1){
					printf("连接错误，程序即将退出\n");
					free(pkt);
					free(pkt1);
					closesocket(socketClient);
					WSACleanup();
					Sleep(5000);
					exit(1); 
				}
				else{
				totalpacket = pkt1->len;
				printf("预备建立连接，此次传输总共有%d个数据包\n", totalpacket);
				init_packet(pkt);
				init_packet(pkt1);
				pkt = connecthandler(pkt,2);
				sendto(socketClient, (char*)pkt, sizeof(*pkt), 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				printf("已发送第三次握手信息\n");
				stage = 1;
				break;
					}
			case 1:
				recvfrom(socketClient, (char*)pkt1, sizeof(*pkt1), 0, (SOCKADDR*)&addrServer, &len);
				memcpy(filename, pkt1->data, pkt1->len);
				printf("已收到来自服务器端传来的文件目录：%s\n", filename);
				char filename1[30]="";
				printf("请输入你要保存文件的路径。\n");
				fgets(filename1, sizeof(filename1), stdin);	
				// 删除可能存在的换行符
				filename1[strcspn(filename1, "\n")] = '\0';
				memset(pkt, 0, sizeof(struct packet));
				memset(pkt1, 0, sizeof(struct packet));
				pkt->ack=1;
				pkt->signal=300;
				sendto(socketClient, (char*)pkt, sizeof(*pkt), 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
				printf("已发送对于文件目录数据包的确认ACK\n");
				Sleep(5000);
				out_result = fopen(filename1, "ab");
				if (!out_result) {
					printf("文件打开失败！！！\n");
					exit(1);
				}
				stage = 2;
				break;
			case 2:
				init_packet(pkt);
				init_packet(pkt1);
				  while(recvfrom(socketClient, (char *)pkt1,sizeof(*pkt1), 0, (SOCKADDR*)&addrServer, &len)<0){
					  continue;
				  }	
				
				if (pkt1->signal == 444) {
					printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<收到第 %u 号数据包pkt1->len=%d\n\n", pkt1->seq,pkt1->len);
					fwrite(pkt1->data, 1, pkt1->len, out_result);
					fflush(out_result);
					memset(pkt1, 0, sizeof(struct packet));
					memset(pkt, 0, sizeof(struct packet));
					waitseq++;
					make_mypkt(pkt, waitseq, 1);       
					printf("发送对第 %u 号数据包的确认\n", waitseq-1);
					sendto(socketClient, (char*)pkt, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
					printf("**************************************\n");
					printf("文件传输成功\n");
					goto success;
				}
				// 停止等待实现
				if (pkt1->seq == waitseq ) {
					printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<收到第 %u 号数据包\n\n", pkt1->seq);
					// 写入文件
					printf("文件seq %d\n",pkt1->seq);
					fwrite(pkt1->data, 1, pkt1->len, out_result);
					fflush(out_result);
					memset(pkt1, 0, sizeof(struct packet));
					memset(pkt, 0, sizeof(struct packet));
					waitseq++;
					make_mypkt(pkt, waitseq, 1);       
					printf("发送对第 %u 号数据包的确认\n", waitseq-1);
					sendto(socketClient, (char*)pkt, BUFFER, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
					waitseq %= seqnumber;
					totalrecv++;
				} else {
					// 如果收到的不是期待的数据包或数据包损坏，则忽略或进行相应的处理
					printf("**********不是期待的数据包或数据包损坏\n");
				}	
				break;
			}
		}
	}
	success:
	{
		fclose(out_result); // 关闭文件
		exit(0); // 退出程序
	}
	free(pkt);
	free(pkt1);
	closesocket(socketClient);
	WSACleanup();
	return 0;
}
