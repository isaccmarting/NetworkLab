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

#define SERVER_PORT 12345
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
volatile int qid_p2c, qid_c2p; 
volatile int snd_c2p, snd_p2c; 

void *thread_func(int BaseSocket)
{
    char rcvChar, bytes; 
    char buf[BUF_SIZE]; 
	int i = 0, state = 0, len; 
    struct msgmbuf msg, msg_p2c; 
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程 
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消 
    
	if((key_c2p = ftok("c2p", 'a')) == -1) {
		fatal("Key error!\n"); 
	}
	
	qid_c2p = msgget(key_c2p, IPC_CREAT | 0666); 
    if(qid_c2p == -1)
        fatal("Creating message queue error!\n"); 
    // printf("The the queue number is %d\n", qid_c2p); 
    
    while(1) {
// 		if(snd_p2c != 0 && (msgrcv(qid_p2c, &msg_p2c, BUF_SIZE, 0, 0)) >= 0) {
// 			snd_p2c = 0; 
// 			puts("Child: "); 
// 			puts((&msg_p2c) -> msg_text); 
// 			if(strcmp((&msg_p2c) -> msg_text, "exit") == 0) {
// 				memset((&msg_p2c) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
// 			    if((msgctl(qid_c2p, IPC_RMID, NULL)) < 0)
// 		            fatal("Deleting message queue error!\n"); 
// 		        pthread_exit(0); 
// 		    }
// // 		    else
// // 		    {
// // 		        int len; 
// // 				memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
// // 				strcpy(msg.msg_text, msg_p2c.msg_text); 
// // 		        len = strlen((&msg) -> msg_text); 
// // 				msg.msg_type = getpid(); 
// // 		        if((msgsnd(qid_c2p, &msg, len, 0) < 0))
// // 		            fatal("No message!\n"); 
// // 				snd_c2p = 1; 
// // 				memset((&msg_p2c) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
// // 		    }
// 		}
		
// 		bytes = read(BaseSocket, buf, BUF_SIZE); 
// 		if(bytes <= 0) continue; // exit(0); 
// 		write(1, buf, bytes); 
		bytes = read(BaseSocket, &rcvChar, sizeof(rcvChar)); 
		if(bytes <= 0) continue; 
		if(bytes > 0) {
			state = 1; 
			buf[i++] = rcvChar; 
			if(rcvChar == '\t') {
				buf[i] = 0; 
				memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
				strcpy((&msg) -> msg_text, buf); 
				msg.msg_type = getpid(); 
				snd_c2p = 1; 
				puts((&msg) -> msg_text); 
				len = strlen((&msg) -> msg_text); 
				if((msgsnd(qid_c2p, &msg, len, 0) < 0))
					fatal("No message!\n"); 
				puts((&msg) -> msg_text); 
				i = 0; 
				state = 0; 
			}
			write(1, &rcvChar, sizeof(rcvChar)); 

		}
// 		else if(state != 0) {
// 
// 			continue; 
// 		}
	}
    return NULL; 
}

int fatal(char *string)
{
    printf("%s\n", string); 
    exit(1); 
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
    int i, quit = 0; 
	int len; 
    
	if((key_p2c = ftok("p2c", 'a')) == -1) {
        fatal("Key error!\n"); 
    }
	if((qid_p2c = msgget(key_p2c, IPC_CREAT | 0666)) == -1) {
        fatal("Creating message queue error!\n"); 
    }
    printf("The queue number is %d\n", qid_p2c); 
	snd_c2p = 0; snd_p2c = 0; 
    
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
				fflush(stdin); 
				printf("Please input server IP: "); 
				gets(ConnectEntry -> IP); 
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
	            
	            ConnectState = 1; 
	            err = pthread_create(&tid, NULL, thread_func, BaseSocket); 
	            if(err != 0)
	                fatal("Creating thread failed!\n"); 
				
				snd_c2p = 0; 
				while(snd_c2p == 0); 
                break; 
            case GET_TIME: 
                break; 
            case GET_NAME: 
                break; 
            case GET_LIST: 
                break; 
            case SEND_INFO: 
				memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
                printf("Please input the information: \n"); 
				if((fgets((&msg) -> msg_text, BUF_SIZE, stdin)) == NULL)
                    fatal("No message!\n"); 
//                 len = strlen((&msg) -> msg_text); 
// 				msg.msg_type = getpid(); 
//                 if((msgsnd(qid_p2c, &msg, len, 0)) < 0)
//                     fatal("Adding message error!\n"); 
// 				snd_p2c = 1; 
				puts("Father: "); 
				puts((&msg) -> msg_text); 
				write(BaseSocket, (&msg) -> msg_text, strlen((&msg) -> msg_text)); 
// 				while(snd_c2p == 0); 
                break; 
            case EXIT: 
                quit = 1; 
            case DISCONNECT: 
				if(ConnectState == 0) break; 
				msg.msg_type = getpid(); 
				memset((&msg) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
                strcpy((&msg) -> msg_text, "exit"); 
				puts("Father: "); 
				puts((&msg) -> msg_text); 
                len = strlen((&msg) -> msg_text); 
                if((msgsnd(qid_p2c, &msg, len, 0)) < 0)
                    fatal("Adding message error!\n"); 
				snd_p2c = 1; 
				if(pthread_cancel(tid) != 0) fatal("Canceling thread failed!\n"); 
				pthread_join(tid, NULL); 
                
                if((msgctl(qid_p2c, IPC_RMID, NULL)) < 0)
                    fatal("Deleting message queue error!\n"); 
                close(BaseSocket); 
                ConnectState = 0; 
                break; 
            default:
                printf("Invalid choice!\n"); 
                break; 
        }
		
		sleep(10); 
		if(snd_c2p != 0 && (msgrcv(qid_c2p, &msg_c2p, BUF_SIZE, 0, 0)) >= 0) {
			// msg_c2p.msg_text[strlen(msg_c2p.msg_text)] = 0; 
// 			printf("%d\n", strlen(msg_c2p.msg_text)); 
// 			printf("The message is %s\n\n", msg_c2p.msg_text); 
			int i; 
			for(i = 0; i < BUF_SIZE; i++)
				putchar((&msg_c2p) -> msg_text[i]); 
			snd_c2p = 0; 
			memset((&msg_c2p) -> msg_text, 0, BUF_SIZE * sizeof(char)); 
		}
    }

	return 0; 
}

