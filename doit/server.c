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
#include <signal.h>
#include <pthread.h>

#include "FrameRule.h"

#define SERVER_PORT 2442
#define BUF_SIZE 4096
#define QUEUE_SIZE 10

// server records the live connections 
struct S_ConnectRecord {
    int mySocket; 						// socket 
	int ConnectState; 					// connection state 
	word num; 							// the sequence number of the packet 
	char* IP; 							// client IP address 
	unsigned int port; 					// client port 
	pthread_t tid; 						// thread id 
	struct S_ConnectRecord *Last; 		// last connection record 
	struct S_ConnectRecord *Next; 		// next connection record 
}; 
typedef struct S_ConnectRecord *PtrToS_ConnectRecord; 
typedef PtrToS_ConnectRecord S_ConnectLog; 

// connection record list 
S_ConnectLog ConnectList; 

// interrupt signal to end the server 
void CtrlC(int signalno)
{
	S_ConnectLog ConnectEntry, ConnectTemp; 
	pthread_t tid; 
	if(ConnectList == NULL) fatal("Invalid ConnectList!\n"); 
	// free ConnectList 
	for(ConnectEntry = ConnectList -> Next; ConnectEntry != NULL; ConnectEntry = ConnectTemp) {
		// cancel threads 
		tid = ConnectEntry -> tid; 
		if(pthread_cancel(tid) != 0) fatal("Canceling thread failed!\n"); 
		pthread_join(tid, NULL); 
		close(ConnectEntry -> mySocket); 
		ConnectTemp = ConnectEntry -> Next; 
		free(ConnectEntry); 
    }
	close(ConnectList -> mySocket); 
	free(ConnectList); 
	printf("Exiting OK!\n"); 
	exit(0); 
}

// the thread to receive or send information from or to clients 
void S_thread_func(S_ConnectLog ConnectEntry)
{
	int mySocket; 
	int myQuit = 0; 
	S_ConnectLog ConnectTemp; 
	
	pFrameHead myHead; 
	byte type; 
	word num, length; 
	char *mybuf, *frmbuf; 
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消 
	
	// send "hello" to client 
	if(ConnectEntry == NULL) pthread_exit(0); 
	mySocket = ConnectEntry -> mySocket; 
	mybuf = "hello"; 
	// head + body 
	writePacket(mySocket, ANS_CONN, mybuf, strlen(mybuf)); 
	
	printf("%8d %8s %04x: Connected\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
	
	myHead = (pFrameHead) malloc(sizeof(struct FrameHead)); 
	if(myHead == NULL) fatal("No memory for myHead!\n"); 
	while(myQuit == 0) {
		// receive packets 
		frmbuf = readPacket(mySocket, myHead); 
		type = myHead -> type; 
		// get local data type 
		num = myHead -> num; 
		length = myHead -> length; 
		// request of time 
		if(type == REQ_TIME) {
			time_t timer; 
			struct tm *tblock; 
			printf("%8d %8s %04x: Time\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			timer = time(NULL); 
			tblock = localtime(&timer); 
			mybuf = asctime(tblock); 
			// head + body 
			writePacket(mySocket, ANS_TIME, mybuf, strlen(mybuf)); 
			// free(mybuf); 
		}
		// request of name 
		else if(type == REQ_NAME) {
			int err; 
			printf("%8d %8s %04x: Name\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			mybuf = (char*) malloc(sizeof(char) * 100); 
			memset(mybuf, 0, sizeof(char) * 100); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n"); 
			err = gethostname(mybuf, sizeof(char) * 100); 
			if(err < 0) {printf("hostname failed!\n"); continue; }
			// head + body 
			writePacket(mySocket, ANS_NAME, mybuf, strlen(mybuf)); 
			free(mybuf); 
		}
		// request of connection lists 
		else if(type == REQ_LIST) {
			int len; 
			printf("%8d %8s %04x: List\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			if(ConnectList == NULL) fatal("ConnectList is NULL!\n"); 
			mybuf = (char*) malloc(sizeof(char) * BUF_SIZE); 
			memset(mybuf, 0, sizeof(char) * BUF_SIZE); 
			for(ConnectTemp = ConnectList -> Next; ConnectTemp!= NULL; ConnectTemp = ConnectTemp -> Next) {
				printf("\t\t%d\t%s\t%04x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
				len = strlen(mybuf); 
				// add records 
				sprintf(mybuf + len, "%d\t%s\t%04x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
			}
			// head + body 
			writePacket(mySocket, ANS_LIST, mybuf, strlen(mybuf)); 
			free(mybuf); 
		}
		// request of sending information 
		else if(type == REQ_INFO) {
			word ClientNum; 
			char *rcvbuf; 
			printf("%8d %8s %04x: Send\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			if(ConnectList == NULL) fatal("Invalid ConnectList!\n"); 
			// get the client number 
			memcpy(&ClientNum, frmbuf, sizeof(ClientNum)); 
			ClientNum = ntohl(ClientNum); 
			// look for the client number 
			for(ConnectTemp = ConnectList -> Next; ConnectTemp != NULL; ConnectTemp = ConnectTemp -> Next) {
				if(ConnectTemp -> num == ClientNum) break; 
			}
			// not found 
			if(ConnectTemp == NULL) {
				rcvbuf = (char*) malloc(strlen("The client does not exist!\n") + 2); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				memset(rcvbuf, 0, strlen("The client does not exist!\n") + 2); 
				// format: ErrorCode + information 
				rcvbuf[0] = 0xFF; // error code 
				strcpy(rcvbuf+sizeof(char), "The client does not exist!\n"); 
				writePacket(mySocket, ANS_INFO, rcvbuf, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				free(rcvbuf); 
			}
			// found 
			else {
				int sndSocket; 
				char sbuf[50]; 
				// send the given information to the goal client 
				sprintf(sbuf, "%-8d %-8s %04x\n", ConnectTemp -> num, ConnectTemp -> IP, ConnectTemp -> port); 
				rcvbuf = (char*) malloc(length - sizeof(ClientNum) + 1 + strlen(sbuf)); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				// the format of received contents is ClientNum + information 
				memset(rcvbuf, 0, length - sizeof(ClientNum) + 1 + strlen(sbuf)); 
				// add number, IP and port of the sending client 
				memcpy(rcvbuf, sbuf, strlen(sbuf)); 
				memcpy(rcvbuf+strlen(sbuf), frmbuf + sizeof(ClientNum), length - sizeof(ClientNum)); 
				sndSocket = ConnectTemp -> mySocket; 
				writePacket(sndSocket, INS_INFO, rcvbuf, strlen(rcvbuf)); 
				free(rcvbuf); 
				
				// send back "Sending is OK!\n" to the sending client 
				rcvbuf = (char*) malloc(strlen("Sending is OK!\n") + 2); 
				if(rcvbuf == NULL) fatal("No memory for rcvbuf!\n"); 
				memset(rcvbuf, 0, strlen("Sending is OK!\n") + 2); 
				// format: ErrorCode + information 
				rcvbuf[0] = 0x00; // means sending is OK 
				strcpy(rcvbuf+sizeof(char), "Sending is OK!\n"); 
				writePacket(mySocket, ANS_INFO, rcvbuf, sizeof(char) + strlen(rcvbuf+sizeof(char))); 
				free(rcvbuf); 
			}
		}
		// request to disconnect 
		else if(type == REQ_DISC) {
			printf("%8d %8s %04x: Disconnect\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
			// free the connecting entry 
			if(ConnectEntry != NULL) {
				ConnectTemp = ConnectEntry -> Last; 
				if(ConnectTemp == NULL) fatal("wrong list!\n"); 
				ConnectTemp -> Next = ConnectEntry -> Next; 
				if(ConnectTemp -> Next != NULL)
					ConnectTemp -> Next -> Last = ConnectTemp; 
				free(ConnectEntry); 
			}
			close(mySocket); 
			myQuit = 1; 
		}
		// discard invalid packets 
		else if(length != 0) {
			printf("%8d %8s %04x: Invalid request\n", ConnectEntry -> num, ConnectEntry -> IP, ConnectEntry -> port); 
		}
		if(myHead -> length > 0)
			free(frmbuf); 
	}
	free(myHead); 
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
    
	signal(SIGINT, CtrlC); 
	
	/* set channel */
	memset(&channel, 0, sizeof(channel)); 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_addr = htonl(INADDR_ANY); 
	channel.sin_port = htons(SERVER_PORT); 

	/* set socket */
	BaseSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(BaseSocket < 0) fatal("socket failed"); 
	setsockopt(BaseSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)); 

	/* bind socket with channel */
	BaseBind = bind(BaseSocket, (struct sockaddr*)&channel, sizeof(channel)); 
	if(BaseBind < 0) fatal("bind failed"); 

	/* listen for requests */ 
	BaseListen = listen(BaseSocket, QUEUE_SIZE); 
	if(BaseListen < 0) fatal("listen failed"); 

	ConnectList = (S_ConnectLog) malloc(sizeof(struct S_ConnectRecord)); 
	if(ConnectList == NULL) fatal("No memory for ConnectList!\n"); 
	ConnectList -> mySocket = BaseSocket; 
	ConnectList -> Next = NULL; 
	ConnectList -> Last = NULL; 
	
	printf("Server is started!\n"); 
	
	/**/
	while(1) {
		// wait for connection requests from clients
		NewSocket = accept(BaseSocket, 0, 0);
		if(BaseSocket < 0) fatal("accept failed"); 
		
		// build a new connection entry 
		ConnectEntry = (S_ConnectLog) malloc(sizeof(struct S_ConnectRecord)); 
		if(ConnectEntry == NULL) fatal("No memory for ConnectEntry!\n"); 
		ConnectEntry ->  mySocket = NewSocket; 
		ConnectEntry -> ConnectState = 1; 
		ConnectEntry -> num = ++num; 
		memset(&peeraddr, 0, sizeof(peeraddr)); 
		lenofsock = sizeof(peeraddr); 
		// get client IP and port 
		err = getpeername(NewSocket, (struct sockaddr*)&peeraddr, &lenofsock); 
		if(err != 0) {printf("getpeername failed!\n"); continue; }
		ConnectEntry -> IP = (char*) inet_ntoa(peeraddr.sin_addr); 
		ConnectEntry -> port = ntohs(peeraddr.sin_port); 
		ConnectEntry -> Next = ConnectList -> Next; 
		ConnectList -> Next  = ConnectEntry; 
		ConnectEntry -> Last = ConnectList; 
		if(ConnectEntry -> Next != NULL)
			ConnectEntry -> Next -> Last = ConnectEntry; 
		
		// create a thread 
		err = pthread_create(&tid, NULL, S_thread_func, ConnectEntry); 
		if(err != 0) fatal("Creating thread failed!\n"); 
		
		ConnectEntry -> tid = tid; 
	}

	return 0; 
}
