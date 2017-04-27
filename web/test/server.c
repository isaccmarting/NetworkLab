#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>
#include <signal.h>

#define SERVER_PORT 2442
#define LISTEN_QUEUE_SIZE 10

// server records the live connections 
struct S_ConnectRecord {
    int mySocket; 						// socket 
	char* IP; 							// client IP address 
	unsigned int port; 					// client port 
	pthread_t tid; 						// thread id 
	struct S_ConnectRecord *Last; 		// last connection record 
	struct S_ConnectRecord *Next; 		// next connection record 
}; 
typedef struct S_ConnectRecord *PtrToS_ConnectRecord; 
typedef PtrToS_ConnectRecord S_ConnectLog; 

S_ConnectLog ConnectList = NULL; 

int fatal(chr *string)
{
	printf("%s\n", string); 
	exit(1); 
}

int CtrlC(int signalno)
{
	S_ConnectLog ConnectEntry, ConnectTemp; 
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
	free(ConnectList); 
	printf("Exiting OK!\n"); 
	exit(0); 
}

void thread_func(ConnectLog ConnectEntry)
{
	pthread_setcancelstate(PTTHREAD_CANCEL_ENABLE, NULL); 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); 
	
	if(ConnectEntry == NULL) pthread_exit(0); 
	
	pthread_exit(0); 
}

int main()
{
	int sSocket. myBind, myListen; 
	int aSocket; 
	struct sockaddr_in channel; 
	ConnectLog ConnectTemp; 
	int err; 
	struct sockaddr_in aChannel; 
	pthread_t tid; 
	
	signal(SIGINT, CtrlC); 
	
	sSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(sSocket < 0) fatal("Error socket!\n"); 
	
	memset(&channel, 0, sizeof(channel)); 
	channel.sin_family = AF_INET; 
	channel.sin_addr.s_addr = htonl(INADDR_ANY); 
	channel.sin_port = htons(SERVER_PORT); 
	
	myBind = bind(sSocket, (sockaddr*)&channel, sizeof(channel)); 
	if(myBind < 0) fatal("Error bind!\n"); 
	
	myListen = listen(sSocket, LISTEN_QUEUE_SIZE); 
	if(myListen < 0) fatal("Error listen!\n"); 
	
	ConnectList = (struct S_ConnectRecord *) malloc(sizeof(struct S_ConnectRecord)); 
	if(ConnectList == NULL) fatal("No memory for ConnectList!\n"); 
	ConnectList -> Last = NULL; 
	ConnectList -> Next = NULL; 
	
	while(1) {
		aSocket = accept(sSocket, 0, 0); 
		if(aSocket < 0) fatal("Error accept!\n"); 
		
		ConnectTemp = (struct S_ConnectRecord *) malloc(sizeof(struct S_ConnectRecord)); 
		if(ConnectTemp == NULL) fatal("No memory for ConnectTemp!\n"); 
		err = getpeername(aSocket, (sockaddr*)&aChannel, sizeof(aChannel)); 
		if(err < 0) fatal("Error getpeername!\n"); 
		
		ConnectTemp -> mySocket = aSocket; 
		ConnectTemp -> IP = (char *) ntoa(aChannel.sin_addr); 
		ConnectTemp -> port = ntohs(aChannel.sin_port); 
		ConnectTemp -> Last = ConnectList; 
		ConnectTemp -> Next = ConnectList -> Next; 
		ConnectList -> Next = ConnectTemp; 
		if(ConnectTemp -> Next != NULL) 
			ConnectTemp -> Next -> Last = ConnectTemp; 
		
		err = pthread_create(&tid, NULL, &thread_func, &ConnectTemp); 
		if(err < 0) fatal("Error pthread_create!\n"); 
		ConnectTemp -> tid = tid; 
		
		close(aSocket); 
	}
	return 0; 
}
