#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/msg.h>
#include <sys/ipc.h>

#include "FrameRule.h"

#define BUF_SIZE 4096

// save server IP and port 
struct ConnectRecord {
    char IP[30]; 
    unsigned int port; 
}; 
typedef struct ConnectRecord *PtrToConnectRecord; 
typedef PtrToConnectRecord ConnectLog; 

// the message structure 
struct msgmbuf {
	long msg_type; 
	char msg_text[BUF_SIZE]; 
};

// enum type of the menu 
enum choice {CONNECT, DISCONNECT, GET_TIME, GET_NAME, GET_LIST, SEND_INFO, EXIT}; 
const char choices[][20] = {
    "connect", "disconnect", "get_time", "get_name", "get_list", "send_info", "exit"
}; 

key_t key_p2c, key_c2p; 
int qid_p2c, qid_c2p; 
volatile int snd_c2p, snd_p2c; 

// the thread receives packets from the server and send message to the main thread 
void *thread_func(int BaseSocket)
{
    struct msgmbuf msg, msg_p2c; 
	pFrameHead myHead; 
	byte type; 
	word num, length; 
	char *mybuf; 
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消 

	myHead = (pFrameHead) malloc(sizeof(struct FrameHead)); 
	if(myHead == NULL) fatal("No memory for myHead!\n"); 
	while(1) {
		mybuf = readPacket(BaseSocket, myHead); 
		// test if a valid packet 
		type = myHead -> type; 
		// read the contents of the packet 
		memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
		// the format of sending answer: ErrorCode + information 
		if(type == ANS_INFO) 
			strcpy((&msg) -> msg_text, mybuf + 1); 
		else 
			strcpy((&msg) -> msg_text, mybuf); 
		msg.msg_type = getpid(); 
		if((msgsnd(qid_c2p, &msg, strlen(msg.msg_text), 0) < 0))
			fatal("No message!\n"); 
		if(myHead -> length > 0) free(mybuf); 
		snd_c2p++; 
	}
	free(myHead); 
    return NULL; 
}

int main(int argc, char **argv)
{
    struct hostent *host; 
    struct sockaddr_in channel; 
    ConnectLog ConnectEntry; 
    int err; 
    pthread_t tid; 
    struct msgmbuf msg, msg_c2p; 

    int BaseSocket, ClientConnect; 
    int ConnectState = 0; 
    int client_choice = -1; 
	int old_snd_c2p = 0; 
    int i, quit = 0; 
	int len; 
	word ClientNum; 
	char mybuf[BUF_SIZE], sndbuf[BUF_SIZE]; 
    
	// msg queue: parent to child 
	if((key_p2c = ftok("p2c", 'a')) == -1) {
        fatal("Key error!\n"); 
    }
	if((qid_p2c = msgget(key_p2c, IPC_CREAT | 0666)) == -1) {
        fatal("Creating message queue error!\n"); 
    }
    // msg queue: child to parent 
	if((key_c2p = ftok("c2p", 'a')) == -1) {
		fatal("Key error!\n"); 
	}
	qid_c2p = msgget(key_c2p, IPC_CREAT | 0666); 
	if(qid_c2p == -1)
		fatal("Creating message queue error!\n"); 
	
	snd_c2p = 0; snd_p2c = 0; 
	memset((&msg_c2p) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
    
    while(quit == 0) 
    {
		// list the menu 
		printf("Please choose a following number:\n"); 
        for(i = 1; i <= 7; i++)
            if(ConnectState == 0 && i > 1 && i < 7)
                continue; 
            else
                printf("%d-%s\t", i, choices[i-1]); 
        printf("\n"); 
        scanf("%d", &client_choice); getchar();
        client_choice--;
        switch(client_choice)
        {
			case CONNECT: // connect 
                if(ConnectState != 0) {
                    printf("Already connected!\n"); 
                    break; 
                }
				// set socket 
				BaseSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
				if(BaseSocket < 0) fatal("Socket failed!"); 
				
				// get server IP and port 
				ConnectEntry = (ConnectLog) malloc(sizeof(struct ConnectRecord)); 
				if(ConnectEntry == NULL)
				    fatal("No memory to connect!\n"); 
				printf("Please input server IP: "); 
				gets(ConnectEntry -> IP); 
				printf("Please input server port: "); 
				scanf("%d", &(ConnectEntry -> port)); 
				host = gethostbyname(ConnectEntry -> IP); 
				if(host < 0)
				    fatal("gethostbyname failed!\n"); 
				
				// set channel 
				memset(&channel, 0, sizeof(channel)); 
				channel.sin_family = AF_INET; 
				memcpy(&channel.sin_addr.s_addr, host -> h_addr, host -> h_length); 
				channel.sin_port = htons(ConnectEntry -> port); 

				// connect socket and channel 
				ClientConnect = connect(BaseSocket, (struct sockaddr*) &channel, sizeof(channel)); 
	            if(ClientConnect < 0) fatal("connect failed"); 
	            
				old_snd_c2p = snd_c2p; 
				ConnectState = 1; 
	            err = pthread_create(&tid, NULL, thread_func, BaseSocket); 
	            if(err != 0)
	                fatal("Creating thread failed!\n"); 
				
				// wait the thread to send message to the main thread 
				while(snd_c2p == old_snd_c2p); 
                break; 
			case GET_TIME: // get the server time 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writePacket(BaseSocket, REQ_TIME, sndbuf, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
			case GET_NAME: // get the server name 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writePacket(BaseSocket, REQ_NAME, sndbuf, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
			case GET_LIST: // get the connection list 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writePacket(BaseSocket, REQ_LIST, sndbuf, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
			case SEND_INFO: // send information to other clients 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				// get client number and information to send 
				printf("Please input the ClientNum: "); 
				scanf("%ld", &ClientNum); getchar(); 
				memset(mybuf, 0, BUF_SIZE * sizeof(char)); 
				printf("Please input the information: \n"); 
				if((fgets(mybuf, BUF_SIZE, stdin)) == NULL)
                    fatal("No message!\n"); 
				// format: ClientNum + information 
				ClientNum = htonl(ClientNum); 
				memset(sndbuf, 0, sizeof(sndbuf)); 
				memcpy(sndbuf, &ClientNum, sizeof(ClientNum)); 
				memcpy(sndbuf+sizeof(ClientNum), mybuf, strlen(mybuf)); 
				writePacket(BaseSocket, REQ_INFO, sndbuf, sizeof(ClientNum) + strlen(sndbuf+sizeof(ClientNum))); 
				
				while(snd_c2p == old_snd_c2p); 
                break; 
			case EXIT: // exit 
                quit = 1; 
			case DISCONNECT: // disconnect 
				if(ConnectState == 0) break; 
				// cancel the thread 
				if(pthread_cancel(tid) != 0) fatal("Canceling thread failed!\n"); 
				pthread_join(tid, NULL); 
				
				// close socket 
				writePacket(BaseSocket, REQ_DISC, sndbuf, 0); 
                close(BaseSocket); 
                ConnectState = 0; 
                break; 
            default:
                printf("Invalid choice!\n"); 
                break; 
        }
		
		// receive message from the thread 
		while(snd_c2p != 0 && (msgrcv(qid_c2p, &msg_c2p, BUF_SIZE, 0, 0)) > 0) {
			printf("The message is \n%s\n\n", msg_c2p.msg_text); 
			snd_c2p--; 
			memset((&msg_c2p) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
		}
    }
	
	// exit msg queue
	if((msgctl(qid_p2c, IPC_RMID, NULL)) < 0)
		fatal("Deleting message queue error!\n"); 
	if((msgctl(qid_c2p, IPC_RMID, NULL)) < 0)
		fatal("Deleting message queue error!\n"); 

	return 0; 
}

