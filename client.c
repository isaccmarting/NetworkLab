#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 12345
#define BUF_SIZE 4096

struct ConnectRecord {
    char IP[30]; 
    unsigned int port; 
}; 
typedef struct ConnectRecord *PtrToConnectRecord; 
typedef PtrToConnectRecord ConnectLog; 

enum choice {CONNECT, DISCONNECT, GET_TIME, GET_NAME, GET_LIST, SEND_INFO, EXIT}; 
const char choices[][20] = {
    "connect", "disconnect", "get_time", "get_name", "get_list", "send_info", "exit"
}; 

void *thread_func(int BaseSocket)
{
    char bytes; 
    char buf[BUF_SIZE]; 
    
    while(1) {
		bytes = read(BaseSocket, buf, BUF_SIZE); 
		if(bytes <= 0) exit(0); 
		write(1, buf, bytes); 
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
    key_t key; 
    int qid; 

    int BaseSocket, ClientConnect; 
    int ConnectState = 0; 
    int client_choice = -1; 
    int i, quit = 0; 
    
    if((key = ftok(".", 'a')) == -1) {
        fatal("Key error!\n"); 
    }
    if((qid = msgget(key, IPC_CREAT | 0666)) == -1) {
        fatal("Creating message queue error!\n"); 
    }
    printf("The queue number is %d\n", qid); 
    
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
                break; 
            case DISCONNECT: 
                close(BaseSocket); 
                ConnectState = 0; 
                break; 
            case GET_TIME: 
                break; 
            case GET_NAME: 
                break; 
            case GET_LIST: 
                break; 
            case SEND_INFO: 
                break; 
            case EXIT: 
                if(ConnectState != 0)
                    close(BaseSocket); 
                exit(0); 
                break; 
            default:
                printf("Invalid choice!\n"); 
                break; 
        }
    }

	return 0; 
}

