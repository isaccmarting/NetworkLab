#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>

#define SERVER_PORT 2451
#define MAX_TEMP_LEN 1000

int mysocket; 

void CtrlC(int signalno)
{
	printf("\nOK!\n"); 
	close(mysocket); 
	exit(0); 
}

int fatal(char *string)
{
	printf("%s\n", string); 
	exit(1); 
}

int main()
{
	int cSocket, myConnect; 
	struct hostent *host; 
	struct sockaddr_in channel; 
	/*char buf[] = "GET / HTTP/1.1\r\nHost: www.zju.edu.cn\r\nConnection: keep-alive\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,* /*;q=0.8\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.108 Safari/537.36 2345Explorer/8.4.1.14855\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.8\r\n\r\n"; */
	char buf[] = "GET /txt/test.txt HTTP/1.1\r\nHost: 192.168.184.129\r\nConnection: keep-alive\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUpgrade-Insecure-Requests: 1\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.108 Safari/537.36 2345Explorer/8.4.1.14855\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.8\r\n\r\n"; 
	int bytes; 
	char temp[MAX_TEMP_LEN]; 
	
	signal(SIGINT, CtrlC); 
	
	cSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if(cSocket < 0) fatal("Error socket!\n"); 
	
	host = gethostbyname("192.168.184.129"); 
	
	memset(&channel, 0, sizeof(channel)); 
	channel.sin_family = AF_INET; 
	memcpy(&channel.sin_addr.s_addr, host -> h_addr, host -> h_length); 
	channel.sin_port = htons(SERVER_PORT); 
	
	myConnect = connect(cSocket, (struct sockaddr*)&channel, sizeof(channel)); 
	if(myConnect < 0) fatal("Error connection!\n"); 
	
	// printf("%s", buf); 
	// write(cSocket, "hello", strlen("hello")); 
	write(cSocket, buf, strlen(buf)); mysocket = cSocket; 
	while(1) {
		bytes = read(cSocket, temp, MAX_TEMP_LEN); 
		// printf("bytes: %d\n", bytes); 
		if(bytes <= 0) break; 
		write(1, temp, bytes); 
	}
	// printf("\nOK!\n"); 
	// close(cSocket); 
	return 0; 
}
