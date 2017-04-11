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

// write a complete packet 
// mySocket: socket; type: the type of the socket; buf: buf to store the body; length: the length of the body 
int writePacket(int mySocket, byte type, char* buf, word length)
{
	pFrameHead myHead; 
	char* sndbuf; 
	static word num = 0; 
	int size_head, i; 
	// construct the head of the packet 
	myHead = (pFrameHead) malloc(sizeof(struct FrameHead));  
	if(myHead == NULL) {
		fatal("No memory for myHead!\n"); 
	}
	myHead -> begin[0] = BEGIN0; myHead -> begin[1] = BEGIN1; myHead -> begin[2] = BEGIN2; 
	myHead -> type = type; 
	myHead -> num = htonl(++num); // translate to the network type 
	myHead -> length = htonl(length); 
	
	// head: BEGIN0(1) BEGIN1(1) BEGIN2(1) TYPE(1) NUM(4) LENGTH(4)
	size_head = sizeof(byte) * 3 + sizeof(byte) + sizeof(word) + sizeof(word); 
	sndbuf = (char*) malloc(size_head + sizeof(char) * length + 1); 
	if(sndbuf == NULL) fatal("No memory for sndbuf!\n"); 
	memset(sndbuf, 0, size_head + sizeof(char) * length + 1); 
	// write the content 
	for(i = 0; i < 3; i++) sndbuf[i] = myHead -> begin[i]; 
	sndbuf[i++] = myHead -> type; 
	memcpy(sndbuf+i, &(myHead -> num), sizeof(myHead -> num)); 
	memcpy(sndbuf+i+sizeof(myHead -> length), &(myHead -> length), sizeof(myHead -> length)); 
	if(length > 0) memcpy(sndbuf+size_head, buf, sizeof(char) * length); 
	write(mySocket, sndbuf, size_head + sizeof(char) * length); 
	free(myHead); 
	free(sndbuf); 
	
	return size_head + sizeof(char) * length; 
}

// read the packet  
// mySocket: socket; myHead: the head of the socket 
// return the body of the socket 
char* readPacket(int mySocket, pFrameHead myHead)
{
	int size_head, bytes, total = 0; 
	char *buf, temp, state = 0; 
	size_head = sizeof(byte) * 3 + sizeof(byte) + sizeof(word) + sizeof(word); 
	if(myHead == NULL) {
		fatal("Invalid myHead!\n"); 
	}
	memset(myHead, 0, sizeof(struct FrameHead)); 
	// to get the start of the head 
	while(state != 3) {
		read(mySocket, &temp, sizeof(temp)); 
		if(state == 0 && temp == (char) BEGIN0) {
			myHead -> begin[0] = temp; 
			state = 1; 
		}
		else if(state == 1 && temp == (char) BEGIN1) {
			myHead -> begin[1] = temp; 
			state = 2; 
		}
		else if(state == 2 && temp == (char) BEGIN2) {
			myHead -> begin[2] = temp; 
			state = 3; 
		}
		else
			state = 0; 
	}
	// get the head of the packet 
	read(mySocket, &(myHead -> type), sizeof(myHead -> type)); 
	read(mySocket, &(myHead -> num), sizeof(myHead -> num)); 
	read(mySocket, &(myHead -> length), sizeof(myHead -> length)); 
	
	myHead -> num = htonl(myHead -> num); // translate to the network type 
	myHead -> length = htonl(myHead -> length); 
	
	// get the body of the packet 
	if(myHead -> length > 0) {
		buf = (char*) malloc(sizeof(char) * myHead -> length + 1); 
		if(buf == NULL) fatal("No memory for buf!\n"); 
		memset(buf, 0, sizeof(char) * myHead -> length + 1); 
		total = 0; 
		while(total < myHead -> length) {
			bytes = read(mySocket, buf + total, myHead -> length - total); 
			total += bytes; 
		}
	}
	
	return buf; 
}

