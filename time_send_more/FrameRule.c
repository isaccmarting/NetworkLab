#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
 
#include "FrameRule.h"
 
int fatal(char *string)
{
	printf("%s\n", string); 
	exit(1); 
}

int writeHead(int mySocket, byte type, word length)
{
	pFrameHead myHead; 
	static word num = 0; 
	myHead = (pFrameHead) malloc(sizeof(struct FrameHead));  
	if(myHead == NULL) {
		printf("No memory for myHead!\n"); 
		return -1; 
	}
	myHead -> begin[0] = BEGIN0; myHead -> begin[1] = BEGIN1; myHead -> begin[2] = BEGIN2; 
	myHead -> type = type; 
	myHead -> num = htonl(++num); 
	myHead -> length = htonl(length); 
	write(mySocket, (byte*) myHead, sizeof(struct FrameHead)); 
	free(myHead); 
	return 0; 
}

pFrameHead readHead(int mySocket)
{
	pFrameHead myHead; 
	int size_head, bytes, total = 0; 
	size_head = sizeof(struct FrameHead); 
	myHead = (pFrameHead) malloc(size_head); 
	if(myHead == NULL) {
		fatal("No memory for myHead!\n"); 
	}
	memset(myHead, 0, size_head); 
	while(total < size_head) {
		bytes = read(mySocket, (char*)myHead + total, size_head - total); 
		total += bytes; 
	}
	return myHead; 
}

int readFrame(int mySocket, char* buf, int size_frame)
{
	int bytes, total = 0; 
	while(total < size_frame) {
		bytes = read(mySocket, buf + total, size_frame - total); 
		total += bytes; 
	}
	return total; 
}

