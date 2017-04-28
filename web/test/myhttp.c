#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myhttp.h"

#define PRE_FILE_PATH "."
#define MAX_FILE_TEMP_LEN 512

struct file_type
{
	char *suffix; 
	char *mytype; 
}; 
struct file_type file_types[] = {
	{"txt", "text/plain"}, 
	{"html", "text/html"}
}; 

void WaitForNext(int cSocket)
{
	char c, state = 0; 
	int bytes; 
	while(state != 4) {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) break; 
		if(state == 0 && c == '\r') state = 1; 
		else if(state == 1 && c == '\n') state = 2; 
		else if(state == 2 && c == '\r') state = 3; 
		else if(state == 3 && c == '\n') state = 4; 
		else state = 0; 
	}
	return ; 
}

http_head getHead(int cSocket)
{
	http_head pHttpHead; 
	int i; 
	char c; 
	int bytes; 
	pHttpHead = (http_head) malloc(sizeof(struct http_head_)); 
	if(pHttpHead == NULL) {printf("No memory for pHttpHead!\n"); exit(1); }
	memset(pHttpHead, 0, sizeof(struct http_head_)); 
	// bytes = read(cSocket, &c, 1); printf("%d\n", bytes); 
	for(i = 0; i < MAX_METHOD_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++) {
		if(c != '\r')
			pHttpHead -> method[i] = c; 
		// bytes = read(cSocket, &c, 1); 
		// printf("%d\n", bytes); 
	}
	pHttpHead -> method[i] = 0; 
	for(i = 0; i < MAX_FILENAME_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++)
		if(c != '\r')
			pHttpHead -> filename[i] = c; 
	pHttpHead -> filename[i] = 0; 
	for(i = 0; i < MAX_VERSION_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++)
		if(c != '\r')
			pHttpHead -> version[i] = c; 
	pHttpHead -> version[i] = 0; 
	return pHttpHead; 
}

void freeHead(http_head pHttpHead)
{
	free(pHttpHead); 
	return ; 
}

void notFound(int cSocket)
{
	char buf[] = "HTTP/1.0 404 File Not Found\r\nContent-type:text/html\r\nContent-length:%d\r\n\r\n%s"; 
	char text[] = "<html><title>File Not Found</title><body><p>The server could not find the resource.</p></body></html>"; 
	char sendbuf[360]; 
	sprintf(sendbuf, buf, strlen(text), text); 
	write(cSocket, sendbuf, strlen(sendbuf)); 
	return ; 
}

/*
void fileFound(int cSocket, char *fType, FILE *pFile)
{
	char buf[] = "HTTP/1.0 200 OK\r\nContent-type:%s\r\nContent-length:%d\r\n\r\n"; 
	char fText[MAX_FILE_TEMP_LEN], headBuf[160]; 
	unsigned long fLength; 
	int bytes; 
	fseek(pFile, 0, SEEK_END); 
	fLength = ftell(pFile); 
	fseek(pFile, 0, SEEK_SET); 
	
	sprintf(headBuf, buf, fType, fLength); 
	write(cSocket, headBuf, strlen(headBuf)); 
	
	while(1) {
		bytes = read(pFile, fText, MAX_FILE_TEMP_LEN); 
		if(bytes <= 0) break; 
		write(cSocket, fText, bytes); 
}
	return ; 
}
*/

char* ftype(char *filename)
{
	int i, len; 
	char *suffix; 
	suffix = strrchr(filename, '.'); 
	suffix += 1; 
	len = sizeof(file_types) / sizeof(file_types[0]); 
	for(i = 0; i < len; i++)
		if(strcmp(suffix, file_types[i].suffix) == 0)
			return file_types[i].mytype; 
	return NULL; 
}

void sendFile(int cSocket, char *filename)
{
	char buf[] = "HTTP/1.0 200 OK\r\nContent-type:%s\r\nContent-length:%d\r\n\r\n"; 
	char fText[MAX_FILE_TEMP_LEN], headBuf[300]; 
	FILE *pFile; 
	char fPath[50], *fType; 
	unsigned long fLength; 
	int bytes; 
	strcpy(fPath, PRE_FILE_PATH); 
	strcat(fPath, filename); 
	pFile = fopen(fPath, "rb"); 
	if(pFile == NULL) {
		printf("No file %s!\n", fPath); 
		notFound(cSocket); 
		return ; 
	}
	
	fType = ftype(filename); 
	fseek(pFile, 0, SEEK_END); 
	fLength = ftell(pFile); 
	fseek(pFile, 0, SEEK_SET); 
	
	sprintf(headBuf, buf, fType, fLength); 
	write(cSocket, headBuf, strlen(headBuf)); 
	
	while(1) {
		bytes = fread(fText, sizeof(fText[0]), MAX_FILE_TEMP_LEN, pFile); 
		if(bytes <= 0) break; 
		write(cSocket, fText, bytes); 
	}
	fclose(pFile); 
	return ; 
}
