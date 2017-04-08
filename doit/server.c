#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 12345
#define BUF_SIZE 4096
#define QUEUE_SIZE 10

struct S_ConnectRecord {
    int mySocket; 
	int ConnectState; 
    char IP[30]; 
	unsigned int port; 
	struct S_ConnectRecord *Next; 
}; 
typedef struct S_ConnectRecord *PtrToS_ConnectRecord; 
typedef PtrToS_ConnectRecord S_ConnectLog; 

int fatal(char *string)
{
	printf("%s\n", string); 
	exit(1); 
}

void S_thread_func(S_ConnectLog ConnectEntry)
{
	int mySocket; 
	char mybuf[BUF_SIZE]; 
	int bytes, myQuit = 0; 
	if(ConnectEntry == NULL) pthread_exit(0); 
	mySocket = ConnectEntry -> mySocket; 
	memset(mybuf, 0, sizeof(mybuf)); 
	strcpy(mybuf, "wello\n\t"); 
	bytes = strlen(mybuf) + 1; 
	write(mySocket, mybuf, bytes); 
	
	while(myQuit == 0) {
		memset(mybuf, 0, sizeof(mybuf)); 
		// 
		bytes = read(mySocket, mybuf, BUF_SIZE); 
		if(bytes > 0) {
			if(strcmp(mybuf, "1") == 0) {
				memset(mybuf, 0, sizeof(mybuf)); 
				strcpy(mybuf, "hi\t"); 
				bytes = strlen(mybuf) + 1; 
				write(mySocket, mybuf, bytes); 
			}
			else if(strcmp(mybuf, "disconnect") == 0) {
			}
		}
	}
}

int main(int argc, char *argv)
{
	int s, b, l, fd, sa, bytes, on = 1; 
	char buf[BUF_SIZE]; 
    struct sockaddr_in channel; 
	S_ConnectLog ConnectList, ConnectEntry; 
	
	int BaseSocket, BaseBind, BaseListen; 
	int NewSocket; 
	int err; 
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
		ConnectEntry -> port = SERVER_PORT; 
		ConnectEntry -> Next = ConnectList -> Next; 
		ConnectList -> Next  = ConnectEntry; 
		
		err = pthread_create(&tid, NULL, S_thread_func, ConnectEntry); 
		if(err != 0) fatal("Creating thread failed!\n"); 

// 		read(sa, buf, BUF_SIZE); 
// 
// 		/**/
// 		fd = open(buf, O_RDONLY); 
// 		if(fd < 0) fatal("open failed"); 
// 
// 		while(1) {
// 			bytes = read(fd, buf, BUF_SIZE); 
// 			if(bytes <= 0) break; 
// 			write(sa, buf, bytes); 
// 		}
// 		close(fd); 
// 		close(sa); 
	}

	return 0; 
}
