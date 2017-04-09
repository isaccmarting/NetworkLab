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
	// char mybuf[BUF_SIZE]; 
	int bytes, myQuit = 0; 
	S_ConnectLog ConnectTemp; 
	
	pFrameHead myHead; 
	byte type; 
	word num, length; 
	char *mybuf; 
	
	if(ConnectEntry == NULL) pthread_exit(0); 
	mySocket = ConnectEntry -> mySocket; 
	mybuf = "hello\n"; puts(mybuf); 
	// head 
	writeHead(mySocket, ANS_CONN, strlen(mybuf)); 
	puts(mybuf); 
	// body 
	write(mySocket, mybuf, strlen(mybuf)); 
	puts(mybuf); 
	
	while(myQuit == 0) {
		// 
// 		bytes = read(mySocket, mybuf, BUF_SIZE); 
// 		puts(mybuf); 
// 		if(bytes > 0) {
// 			if(strcmp(mybuf, "1\n") == 0) {
// 				memset(mybuf, 0, sizeof(mybuf)); 
// 				strcpy(mybuf, "hi"); 
// 				bytes = strlen(mybuf) + 1; 
// 				write(mySocket, mybuf, bytes); 
// 			}
// 			else if(strcmp(mybuf, "disconnect") == 0) {
// 				printf("Disconnect!\n"); 
// 				if(ConnectEntry != NULL) {
// 					ConnectTemp = ConnectEntry -> Last; 
// 					if(ConnectTemp == NULL) fatal("wrong list!\n"); 
// 					ConnectTemp -> Next = ConnectEntry -> Next; 
// 					if(ConnectTemp -> Next != NULL)
// 						ConnectTemp -> Next -> Last = ConnectTemp; 
// 					free(ConnectEntry -> IP); 
// 					free(ConnectEntry); 
// 				}
// 				close(mySocket); 
// 				myQuit = 1; 
// 			}
// 			else if(strcmp(mybuf, "time") == 0) {
// 				time_t timer; 
// 				struct tm *tblock; 
// 				timer = time(NULL); 
// 				tblock = localtime(&timer); 
// 				memset(mybuf, 0, sizeof(mybuf)); 
// 				strcpy(mybuf, asctime(tblock)); 
// 				bytes = strlen(mybuf) + 1; 
// 				write(mySocket, mybuf, bytes); 
// 			}
// 			else if(strcmp(mybuf, "name") == 0) {
// 				char hostname[100]; 
// 				int err; 
// 				err = gethostname(hostname, sizeof(hostname)); 
// 				if(err < 0) {printf("hostname failed!\n"); continue; }
// 				memset(mybuf, 0, sizeof(mybuf)); 
// 				strcpy(mybuf, hostname); 
// 				bytes = strlen(mybuf) + 1; 
// 				write(mySocket, mybuf, bytes); 
// 			}
// 			else if(strcmp(mybuf, "list") == 0) {
// 				int len; 
// 				memset(mybuf, 0, sizeof(mybuf)); 
// 				if(ConnectList == NULL) fatal("ConnectList is NULL!\n"); 
// 				for(ConnectTemp = ConnectList -> Next; ConnectTemp!= NULL; ConnectTemp = ConnectTemp -> Next) {
// 					printf("%d\t%s\t%06x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
// 					len = strlen(mybuf); 
// 					sprintf(mybuf + len, "%d\t%s\t%06x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
// 				}
// 				bytes = strlen(mybuf) + 1; 
// 				write(mySocket, mybuf, bytes); 
// 			}
// 			else if(strcmp(mybuf, "send") == 0) {
// 				int len; 
// 				memset(mybuf, 0, sizeof(mybuf)); 
// 				
// 				bytes = strlen(mybuf) + 1; 
// 				write(mySocket, mybuf, bytes); 
// 			}
// 		}
		
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
		}
		else if(type == REQ_DISC) {
			printf("Disconnect!\n"); 
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
	int s, b, l, fd, sa, bytes, on = 1; 
	char buf[BUF_SIZE]; 
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
