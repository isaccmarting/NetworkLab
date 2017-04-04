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

typedef PtrToConnectRecord ConnectLog; 
typedef struct ConnectRecord *PtrToConnectRecord; 
struct ConnectRecord {
    char IP[30]; 
    unsigned int port; 
}; 

enum choice {CONNECT, DISCONNECT, GET_TIME, GET_NAME, GET_LIST, SEND_INFO, EXIT}; 
const char choices[] = {
    "connect", "disconnect", "get_time", "get_name", "get_list", "send_info", "exit"
}; 

int fatal(char *string)
{
    printf("%s\n", string); 
    exit(1); 
}

int main(int argc, char **argv)
{
    struct hostent *host; 
    struct sockaddr_in channel; 
    ConnectLog ConnectList[100]; 

    int BaseSocket, ClientConnect; 
    int connect_cnt = 0, byte_cnt = 0; 
    int client_choice = -1; 
    int i, quit = 0; 
    char buf[BUF_SIZE]; 
    
    BaseSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(BaseSocket < 0) fatal("Socket failed!"); 
	
    memset(&channel, 0, sizeof(channel)); 
    channel.sin_family = AF_INET; 
    memcpy(&channel.sin_addr.s_addr, host -> h_addr, host -> h_length); 
    channel.sin_port = htons(SERVER_PORT); 
    
    while(quit == 0) 
    {
        printf("Please choose a following number:\n"); 
        for(i = 1; i <= 7; i++)
            if(connect_cnt == 0 && i > 1 && i < 7)
                continue; 
            else
                printf("%d-%s\t", i, choices[i]); 
        scanf("%d", &client_choice); 
        switch(client_choice)
        {
            case CONNECT: 
	            ClientConnect = connect(BaseSocket, (struct sockaddr*) &channel, sizeof(channel)); 
	            if(ClientConnect < 0) fatal("connect failed"); 
                break; 
            case DISCONNECT: 
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
                break; 
            default:
                printf("Invalid choice!\n"); 
                break; 
        }
    }


	/**/
	write(s, argv[2], strlen(argv[2]) + 1); 

	while(1) {
		bytes = read(s, buf, BUF_SIZE); 
		if(bytes <= 0) exit(0); 
		write(1, buf, bytes); 
	}

	return 0; 
}

