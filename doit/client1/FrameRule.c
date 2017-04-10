#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
 
#include "FrameRule.h"

// deal with the errors 
int fatal(char *string)
{
	printf("%s\n", string); 
	exit(1); 
}

// send the head of he frame 
// mySocket: socket; type: type of the packet; length: the length of contents 
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
	myHead -> num = htonl(++num); // translate to the network type 
	myHead -> length = htonl(length); 
	write(mySocket, (byte*) myHead, sizeof(struct FrameHead)); 
	free(myHead); 
	return 0; 
}

// read the head of the packet  
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
	// to keep a complete head 
	while(total < size_head) {
		bytes = read(mySocket, (char*)myHead + total, size_head - total); 
		total += bytes; 
	}
	return myHead; 
}

// read a complete packet 
// mySocket: socket; buf: buf to store the packet; size_frame: the length of the frame 
int readFrame(int mySocket, char* buf, int size_frame)
{
	int bytes, total = 0; 
	// to keep a complete packet 
	while(total < size_frame) {
		bytes = read(mySocket, buf + total, size_frame - total); 
		total += bytes; 
	}
	return total; 
}

