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

struct ConnectRecord {
    char IP[30]; 
    unsigned int port; 
}; 
typedef struct ConnectRecord *PtrToConnectRecord; 
typedef PtrToConnectRecord ConnectLog; 

struct msgmbuf {
	long msg_type; 
	char msg_text[BUF_SIZE]; 
};

enum choice {CONNECT, DISCONNECT, GET_TIME, GET_NAME, GET_LIST, SEND_INFO, EXIT}; 
const char choices[][20] = {
    "connect", "disconnect", "get_time", "get_name", "get_list", "send_info", "exit"
}; 

key_t key_p2c, key_c2p; 
int qid_p2c, qid_c2p; 
volatile int snd_c2p, snd_p2c; 

void *thread_func(int BaseSocket)
{
    struct msgmbuf msg, msg_p2c; 
	pFrameHead myHead; 
	byte type; 
	word num, length; 
	char *mybuf; 
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消 

	while(1) {
		myHead = readHead(BaseSocket); 
		if(myHead == NULL) fatal("Invalid head!\n"); 
		if(myHead -> begin[0] == BEGIN0 && myHead -> begin[1] == BEGIN1 && myHead -> begin[2] == BEGIN2) {
			type = myHead -> type; 
			num = ntohl(myHead -> num); 
			length = ntohl(myHead -> length); 
			mybuf = (char*) malloc(sizeof(char) * length + 1); 
			if(mybuf == NULL) fatal("No memory for mybuf!\n"); 
			memset(mybuf, 0, sizeof(char) * length + 1); 
			readFrame(BaseSocket, mybuf, length); 
			memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
			if(type == ANS_INFO) 
				strcpy((&msg) -> msg_text, mybuf + 1); 
			else 
			    strcpy((&msg) -> msg_text, mybuf); 
			// msg.msg_text = mybuf; 
			// puts((&msg) -> msg_text); 
			msg.msg_type = getpid(); 
			if((msgsnd(qid_c2p, &msg, strlen(msg.msg_text), 0) < 0))
				fatal("No message!\n"); 
			free(mybuf); 
			snd_c2p++; 
		}
	    free(myHead); 
	}
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
            case CONNECT: 
                if(ConnectState != 0) {
                    printf("Already connected!\n"); 
                    break; 
                }
				BaseSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
				if(BaseSocket < 0) fatal("Socket failed!"); 
				
				ConnectEntry = (ConnectLog) malloc(sizeof(struct ConnectRecord)); 
				if(ConnectEntry == NULL)
				    fatal("No memory to connect!\n"); 
				printf("Please input server IP: "); 
				scanf("%s", &(ConnectEntry -> IP)); 
				printf("Please input server port: "); 
				scanf("%d", &(ConnectEntry -> port)); 
				host = gethostbyname(ConnectEntry -> IP); 
				if(host < 0)
				    fatal("gethostbyname failed!\n"); 
				
				memset(&channel, 0, sizeof(channel)); 
				channel.sin_family = AF_INET; 
				memcpy(&channel.sin_addr.s_addr, host -> h_addr, host -> h_length); 
				channel.sin_port = htons(ConnectEntry -> port); 

	            ClientConnect = connect(BaseSocket, (struct sockaddr*) &channel, sizeof(channel)); 
	            if(ClientConnect < 0) fatal("connect failed"); 
	            
				old_snd_c2p = snd_c2p; 
				ConnectState = 1; 
	            err = pthread_create(&tid, NULL, thread_func, BaseSocket); 
	            if(err != 0)
	                fatal("Creating thread failed!\n"); 
				
				while(snd_c2p == old_snd_c2p); 
                break; 
            case GET_TIME: 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writeHead(BaseSocket, REQ_TIME, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
            case GET_NAME: 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writeHead(BaseSocket, REQ_NAME, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
            case GET_LIST: 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				writeHead(BaseSocket, REQ_LIST, 0); 
				while(snd_c2p == old_snd_c2p); 
                break; 
            case SEND_INFO: 
				if(ConnectState == 0) {printf("Not connected!\n"); break; } 
				old_snd_c2p = snd_c2p; 
				printf("Please input the ClientNum: "); 
				scanf("%ld", &ClientNum); getchar(); 
				memset(mybuf, 0, BUF_SIZE * sizeof(char)); 
				printf("Please input the information: \n"); 
				if((fgets(mybuf, BUF_SIZE, stdin)) == NULL)
                    fatal("No message!\n"); 
				memset(sndbuf, 0, BUF_SIZE * sizeof(char)); 
				ClientNum = htonl(ClientNum); 
				memset(sndbuf, 0, sizeof(sndbuf)); 
				memcpy(sndbuf, &ClientNum, sizeof(ClientNum)); 
				memcpy(sndbuf+sizeof(ClientNum), mybuf, strlen(mybuf)); 
				writeHead(BaseSocket, REQ_INFO, sizeof(ClientNum) + strlen(sndbuf+sizeof(ClientNum))); 
				write(BaseSocket, sndbuf, sizeof(ClientNum) + strlen(sndbuf+sizeof(ClientNum))); 
				
				while(snd_c2p == old_snd_c2p); 
                break; 
            case EXIT: 
                quit = 1; 
            case DISCONNECT: 
				if(ConnectState == 0) break; 
				if(pthread_cancel(tid) != 0) fatal("Canceling thread failed!\n"); 
				pthread_join(tid, NULL); 
				
				// close socket 
				writeHead(BaseSocket, REQ_DISC, 0); 
                close(BaseSocket); 
                ConnectState = 0; 
                break; 
            default:
                printf("Invalid choice!\n"); 
                break; 
        }
		
		while(snd_c2p != 0 && (msgrcv(qid_c2p, &msg_c2p, BUF_SIZE, 0, 0)) > 0) {
			printf("The message is %s\n\n", msg_c2p.msg_text); 
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

