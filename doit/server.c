#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <time.h>

#include "FrameRule.h"

#define SERVER_PORT 2442
#define BUF_SIZE 4096
#define QUEUE_SIZE 10

struct S_ConnectRecord {
    int mySocket; 
	int ConnectState; 
	word num; 
    char* IP; 
	unsigned int port; 
	struct S_ConnectRecord *Last; 
	struct S_ConnectRecord *Next; 
}; 
typedef struct S_ConnectRecord *PtrToS_ConnectRecord; 
typedef PtrToS_ConnectRecord S_ConnectLog; 

S_ConnectLog ConnectList; 

void S_thread_func(S_ConnectLog ConnectEntry)
{
	int mySocket; 
	int myQuit = 0; 
	S_ConnectLog ConnectTemp; 
	
	pFrameHead myHead; 
	byte type; 
	word num, length; 
	char *mybuf; 
	
	if(ConnectEntry == NULL) pthread_exit(0); 
	mySocket = ConnectEntry -> mySocket; 
	mybuf = "hello"; 
	// head 
	writeHead(mySocket, ANS_CONN, strlen(mybuf)); 
	// body 
	write(mySocket, mybuf, strlen(mybuf)); 
	
	while(myQuit == 0) {
		myHead = readHead(mySocket); 
		if(myHead == NULL) {printf("Invalid myHead\n"); continue; }
		type = myHead -> type; 
		num = ntohl(myHead -> num); 
		length = ntohl(myHead -> length); 
		if(type == REQ_TIME) {
			time_t timer; 
			struct tm *tblock; 
			timer = time(NULL); 
			tblock = localtime(&timer); 
			mybuf = asctime(tblock); 
			// head 
			writeHead(mySocket, ANS_TIME, strlen(mybuf)); 
			// body 
			write(mySocket, mybuf, strlen(mybuf)); 
			// free(mybuf); 
		}
		else if(type == REQ_NAME) {
			int err; 
			mybuf = (char*) malloc(sizeof(char) * 100); 
			memset(mybuf, 0, sizeof(char) * 100); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n"); 
			err = gethostname(mybuf, sizeof(char) * 100); 
			if(err < 0) {printf("hostname failed!\n"); continue; }
			// head 
			writeHead(mySocket, ANS_NAME, strlen(mybuf)); 
			// body 
			write(mySocket, mybuf, strlen(mybuf)); 
			free(mybuf); 
		}
		else if(type == REQ_LIST) {
			int len; 
			if(ConnectList == NULL) fatal("ConnectList is NULL!\n"); 
			mybuf = (char*) malloc(sizeof(char) * BUF_SIZE); 
			memset(mybuf, 0, sizeof(char) * BUF_SIZE); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n"); 
			for(ConnectTemp = ConnectList -> Next; ConnectTemp!= NULL; ConnectTemp = ConnectTemp -> Next) {
				printf("%d\t%s\t%06x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
				len = strlen(mybuf); 
				sprintf(mybuf + len, "%d\t%s\t%06x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
			}
			// head 
			writeHead(mySocket, ANS_LIST, strlen(mybuf)); 
			// body 
			write(mySocket, mybuf, strlen(mybuf)); 
			free(mybuf); 
		}
		else if(type == REQ_INFO) {
			word ClientNum; 
			char *rcvbuf; 
			if(ConnectList == NULL) fatal("Invalid ConnectList!\n"); 
			mybuf = (char*) malloc(sizeof(char) * length + 1); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n"); 
			memset(mybuf, 0, sizeof(char) * length + 1); 
			readFrame(mySocket, mybuf, sizeof(char) * length); 
			memcpy(&ClientNum, mybuf, sizeof(ClientNum)); 
			ClientNum = ntohl(ClientNum); 
			for(ConnectTemp = ConnectList -> Next; ConnectTemp != NULL; ConnectTemp = ConnectTemp -> Next) {
				if(ConnectTemp -> num == ClientNum) break; 
			}
			if(ConnectTemp == NULL) {
				rcvbuf = (char*) malloc(strlen("The client does not exist!\n") + 2); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				memset(rcvbuf, 0, strlen("The client does not exist!\n") + 2); 
				rcvbuf[0] = 0xFF; // error code 
				strcpy(rcvbuf+sizeof(char), "The client does not exist!\n"); 
				writeHead(mySocket, ANS_INFO, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				write(mySocket, rcvbuf, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				free(rcvbuf); 
			}
			else {
				int sndSocket; 
				rcvbuf = (char*) malloc(length - sizeof(ClientNum) + 1); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				memset(rcvbuf, 0, length - sizeof(ClientNum) + 1); 
				memcpy(rcvbuf, mybuf + sizeof(ClientNum), length - sizeof(ClientNum)); 
				sndSocket = ConnectTemp -> mySocket; 
				writeHead(sndSocket, INS_INFO, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				write(sndSocket, rcvbuf, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				free(rcvbuf); 
				
				rcvbuf = (char*) malloc(strlen("Sending is OK!\n") + 2); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				memset(rcvbuf, 0, strlen("Sending is OK!\n") + 2); 
				rcvbuf[0] = 0x00; 
				strcpy(rcvbuf+sizeof(char), "Sending is OK!\n"); puts(rcvbuf+1); 
				writeHead(mySocket, ANS_INFO, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				write(mySocket, rcvbuf, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				free(rcvbuf); 
			}
			free(mybuf); 
		}
		else if(type == REQ_DISC) {
			printf("Disconnect: %d\t%s\t%04x\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			if(ConnectEntry != NULL) {
				ConnectTemp = ConnectEntry -> Last; 
				if(ConnectTemp == NULL) fatal("wrong list!\n"); 
				ConnectTemp -> Next = ConnectEntry -> Next; 
				if(ConnectTemp -> Next != NULL)
					ConnectTemp -> Next -> Last = ConnectTemp; 
				// free(ConnectEntry -> IP); 
				free(ConnectEntry); 
			}
			close(mySocket); 
			myQuit = 1; 
		}
		else if(length != 0) {
			mybuf = (char*) malloc(sizeof(char) * length); 
			memset(mybuf, 0, sizeof(char) * length); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n");
			readFrame(mySocket, mybuf, length); 
			free(mybuf); 
		}
	}
}

int main(int argc, char *argv)
{
	int bytes, on = 1; 
	struct sockaddr_in channel, peeraddr; 
	S_ConnectLog ConnectEntry; 
	
	int BaseSocket, BaseBind, BaseListen; 
	int NewSocket; 
	int err; 
	int num = 0; 
	socklen_t lenofsock; 
	pthread_t tid; 
    
    /**/
	memset(&channel, 0, sizeof(channel)); 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_addr = htonl(INADDR_ANY); 
	channel.sin_port = htons(SERVER_PORT); 

	/**/
	BaseSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(BaseSocket < 0) fatal("socket failed"); 
	setsockopt(BaseSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)); 

	BaseBind = bind(BaseSocket, (struct sockaddr*)&channel, sizeof(channel)); 
	if(BaseBind < 0) fatal("bind failed"); 

	BaseListen = listen(BaseSocket, QUEUE_SIZE); 
	if(BaseListen < 0) fatal("listen failed"); 

	ConnectList = (S_ConnectLog) malloc(sizeof(struct S_ConnectRecord)); 
	if(ConnectList == NULL) fatal("No memory for ConnectList!\n"); 
	ConnectList -> Next = NULL; 
	
	/**/
	while(1) {
		NewSocket = accept(BaseSocket, 0, 0);
		if(sa < 0) fatal("accept failed"); 
		
		ConnectEntry = (S_ConnectLog) malloc(sizeof(struct S_ConnectRecord)); 
		if(ConnectEntry == NULL) fatal("No memory for ConnectEntry!\n"); 
		ConnectEntry ->  mySocket = NewSocket; 
		ConnectEntry -> ConnectState = 1; 
		ConnectEntry -> num = ++num; 
		memset(&peeraddr, 0, sizeof(peeraddr)); 
		lenofsock = sizeof(peeraddr); 
		err = getpeername(NewSocket, (struct sockaddr*)&peeraddr, &lenofsock); 
		if(err != 0) {printf("getpeername failed!\n"); continue; }
		// memset(ConnectEntry -> IP, 0, sizeof(ConnectEntry -> IP)); 
		// strcpy(ConnectEntry -> IP, inet_ntoa(peeraddr.sin_addr)); 
		ConnectEntry -> IP = (char*) inet_ntoa(peeraddr.sin_addr); 
		ConnectEntry -> port = ntohs(peeraddr.sin_port); 
		ConnectEntry -> Next = ConnectList -> Next; 
		ConnectList -> Next  = ConnectEntry; 
		ConnectEntry -> Last = ConnectList; 
		if(ConnectEntry -> Next != NULL)
			ConnectEntry -> Next -> Last = ConnectEntry; 
		
		err = pthread_create(&tid, NULL, S_thread_func, ConnectEntry); 
		if(err != 0) fatal("Creating thread failed!\n"); 
	}

	return 0; 
}
