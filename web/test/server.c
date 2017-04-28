#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <pthread.h>
#include <signal.h>

#include "myhttp.h"

#define SERVER_PORT 2442
#define LISTEN_QUEUE_SIZE 10

// server records the live connections 
struct ConnectRecord {
    int mySocket; 						// socket 
	char* IP; 							// client IP address 
	unsigned int port; 					// client port 
	pthread_t tid; 						// thread id 
	struct ConnectRecord *Last; 		// last connection record 
	struct ConnectRecord *Next; 		// next connection record 
}; 
typedef struct ConnectRecord *PtrToConnectRecord; 
typedef PtrToConnectRecord ConnectLog; 

// the list of Connection log 
ConnectLog ConnectList = NULL; 

// deal with errors 
int fatal(char *string)
{
	printf("%s\n", string); 
	exit(1); 
}

// deal with Ctrl+C INT 
int CtrlC(int signalno)
{
	ConnectLog ConnectEntry, ConnectTemp; 
	pthread_t tid; 
	if(ConnectList == NULL) fatal("Invalid ConnectList!\n"); 
	// free ConnectList 
	for(ConnectEntry = ConnectList -> Next; ConnectEntry != NULL; ConnectEntry = ConnectTemp) {
		// cancel threads 
		tid = ConnectEntry -> tid; 
		if(pthread_cancel(tid) != 0) fatal("Error pthread_cancel!\n\n"); 
		pthread_join(tid, NULL); 
		close(ConnectEntry -> mySocket); 
		ConnectTemp = ConnectEntry -> Next; 
		free(ConnectEntry); 
	}
	// close the server socket 
	close(ConnectList -> mySocket); 
	free(ConnectList); 
	printf("Exiting OK!\n"); 
	exit(0); 
}

// the child thread 
// ConnectEntry contains the connection information 
int thread_func(ConnectLog ConnectEntry)
{
	int cSocket; 
	http_head pHttpHead; 
	char buf[512]; 
	int bytes; 
	
	// make the child thread asynchronously cancalable 
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); 
	
	if(ConnectEntry == NULL) pthread_exit(0); 
	cSocket = ConnectEntry -> mySocket; 
	
	// print the client IP and port 
	printf("Connected: %s:%04x\n", ConnectEntry -> IP, ConnectEntry -> port); 
// 	printf("%d\n", errno); 
// 	printf("%d\n", EAGAIN); 
// 	printf("%d\n", EBADF); 
// 	printf("%d\n", EFAULT); 
// 	printf("%d\n", EINTR); 
// 	printf("%d\n", EINVAL); 
// 	printf("%d\n", EIO); 
// 	printf("%d\n", EISDIR); 
	// while(1) {
	// deal with the head of http packets 
	pHttpHead = getHead(cSocket); 
	printf("Method: %s\n", pHttpHead -> method); 
	printf("Filename: %s\n", pHttpHead -> filename); 
	printf("Version: %s\n", pHttpHead -> version); 
// 		if(strlen(pHttpHead -> method) == 0 || strlen(pHttpHead -> filename) == 0 || strlen(pHttpHead -> version) == 0) {
// 			ConnectEntry -> Last -> Next = ConnectEntry -> Next; 
// 			if(ConnectEntry -> Next != NULL)
// 				ConnectEntry -> Next -> Last = ConnectEntry -> Last; 
// 			free(ConnectEntry); 
// 			break; 
// 		}
	// method 
	if(strcmp(pHttpHead -> method, "GET") == 0) {
		// send the requested file or notFound 
		sendFile(cSocket, pHttpHead -> filename); 
		// pass the rest information of the head 
		WaitForNext(cSocket, 0); 
	}
	// deal with post 
	else if(strcmp(pHttpHead -> method, "POST") == 0) doPost(cSocket, pHttpHead -> filename); 
	else { // else send Not Implemented 
		notImplemented(cSocket); 
		// pass the rest information of the head 
		WaitForNext(cSocket, 0); 
	}
	freeHead(pHttpHead); 
	// }
	
	// finish the connection and free the ConnectionEntry 
	close(cSocket); 
	ConnectEntry -> Last -> Next = ConnectEntry -> Next; 
	if(ConnectEntry -> Next != NULL) ConnectEntry -> Next -> Last = ConnectEntry -> Last; 
	free(ConnectEntry); 
	pthread_exit(0); 
}

int main()
{
	int sSocket, myBind, myListen; 
	int on = 1; 
	int aSocket; 
	struct sockaddr_in channel; 
	ConnectLog ConnectTemp; 
	int err; 
	struct sockaddr_in aChannel; 
	socklen_t SizeChannel; 
	pthread_t tid; 
	
	// replace with CtrlC to deal with Ctrl+C INT 
	signal(SIGINT, CtrlC); 
	
	// the server socket 
	sSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(sSocket < 0) fatal("Error socket!\n"); 
	setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)); 
	
	memset(&channel, 0, sizeof(channel)); 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_addr = htonl(INADDR_ANY); 
	channel.sin_port = htons(SERVER_PORT); 
	
	// bind 
	myBind = bind(sSocket, (struct sockaddr*)&channel, sizeof(channel)); 
	if(myBind < 0) fatal("Error bind!\n"); 
	
	// listen 
	myListen = listen(sSocket, LISTEN_QUEUE_SIZE); 
	if(myListen < 0) fatal("Error listen!\n"); 
	
	// create the connection list head 
	ConnectList = (struct ConnectRecord *) malloc(sizeof(struct ConnectRecord)); 
	if(ConnectList == NULL) fatal("No memory for ConnectList!\n"); 
	ConnectList -> mySocket = sSocket; 
	ConnectList -> Last = NULL; 
	ConnectList -> Next = NULL; 
	
	while(1) {
		// accept 
		aSocket = accept(sSocket, 0, 0); 
		if(aSocket < 0) fatal("Error accept!\n"); 
		
		// create a new connection log 
		ConnectTemp = (struct ConnectRecord *) malloc(sizeof(struct ConnectRecord)); 
		if(ConnectTemp == NULL) fatal("No memory for ConnectTemp!\n"); 
		// get the client information 
		SizeChannel = sizeof(aChannel); 
		err = getpeername(aSocket, (struct sockaddr*)&aChannel, &SizeChannel); 
		if(err < 0) fatal("Error getpeername!\n"); 
		
		// the accepted socket 
		ConnectTemp -> mySocket = aSocket; 
		// the client IP 
		ConnectTemp -> IP = (char *) inet_ntoa(aChannel.sin_addr); 
		// the client port 
		ConnectTemp -> port = ntohs(aChannel.sin_port); 
		// join in the connection list 
		ConnectTemp -> Last = ConnectList; 
		ConnectTemp -> Next = ConnectList -> Next; 
		ConnectList -> Next = ConnectTemp; 
		if(ConnectTemp -> Next != NULL) 
			ConnectTemp -> Next -> Last = ConnectTemp; 
		
		// create a new thread to deal with it 
		err = pthread_create(&tid, NULL, thread_func, ConnectTemp); 
		if(err < 0) fatal("Error pthread_create!\n"); 
		// record the thread id to cancel it later 
		ConnectTemp -> tid = tid; 
	}
	return 0; 
}
