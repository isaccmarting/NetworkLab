#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include "myhttp.h"

#define PRE_FILE_PATH "."
#define MAX_FILE_TEMP_LEN 512
#define MAX_LOGIN_PASS_LEN 50
#define LOGIN_ID "3140102442"
#define LOGIN_PASS "2442"

struct file_type
{
	char *suffix; 
	char *mytype; 
}; 
struct file_type file_types[] = {
	{"txt", "text/plain"}, 
	{"html", "text/html"}, 
	{"jpg", "image/jpg"}
}; 

void WaitForNext(int cSocket, char init_state)
{
	char c, state = init_state; 
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

int WaitForContentLen(int cSocket)
{
	int content_len = 0; 
	char str_content_len[] = "Content-Length: "; 
	int i, bytes; 
	char c , state = 2, temp[20]; 
	while(state != 4) {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) break; 
		if(state == 0 && c == '\r') state = 1; 
		else if(state == 1 && c == '\n') {state = 2; i = 0; }
		else if(state == 2) {
			if(c != str_content_len[i]) 
				if(c == '\r') state = 1; 
			    else if(c == '\n') {state = 2; i = 0; }
				else state = 0; 
			else {
				i++; 
				if(i == strlen(str_content_len)) 
					state = 3; 
			}
		}
		else if(state == 3) {
			if(c == '\r') {state = 4; WaitForNext(cSocket, 1); }
			else content_len = content_len * 10 + c - '0'; 
		}
		else state = 0; 
	}
	return content_len; 
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

void notImplemented(int cSocket)
{
	char buf[] = "HTTP/1.1 501 Not Implemented\r\nDate:  %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char text[] = "<html><title>Not Implemented</title><body><p>The server does not recognize the request method. </p></body></html>"; 
	char sendbuf[500]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	sprintf(sendbuf, buf, mytime,  hostname, strlen(text), mytime, text); 
	write(cSocket, sendbuf, strlen(sendbuf)); 
	return ; 
}

void notFound(int cSocket)
{
	char buf[] = "HTTP/1.1 404 File Not Found\r\nDate:  %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char text[] = "<html><title>File Not Found</title><body><p>The server could not find the resource.</p></body></html>"; 
	char sendbuf[500]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	sprintf(sendbuf, buf, mytime,  hostname, strlen(text), mytime, text); 
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
	char buf[] = "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n"; 
	char fText[MAX_FILE_TEMP_LEN], headBuf[300]; 
	FILE *pFile; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
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
	
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	
	fType = ftype(filename); 
	fseek(pFile, 0, SEEK_END); 
	fLength = ftell(pFile); 
	fseek(pFile, 0, SEEK_SET); 
	
	sprintf(headBuf, buf, mytime, hostname, fType, fLength, mytime); 
	write(cSocket, headBuf, strlen(headBuf)); 
	
	while(1) {
		bytes = fread(fText, sizeof(fText[0]), MAX_FILE_TEMP_LEN, pFile); 
		if(bytes <= 0) break; 
		write(cSocket, fText, bytes); 
	}
	fclose(pFile); 
	return ; 
}

void doPost(int cSocket, char *filename)
{
	char *post, c; 
	int bytes, i, text_len, j; 
	char state; 
	char login[MAX_LOGIN_PASS_LEN], pass[MAX_LOGIN_PASS_LEN]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	char buf[] = "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char error_id[] = "<html><title>Login Error!</title><body><p>Login id error!</p></body></html>"; 
	char error_pass[] = "<html><title>Login Error!</title><body><p>Login password error!</p></body></html>"; 
	char success[] = "<html><title>Login Success!</title><body><p>Login success, welcome!</p></body></html>"; 
	char sndBuf[300]; 
	if(filename == NULL) notFound(cSocket); 
	post = strrchr(filename, '/'); 
	post += 1; 
	if(strcmp(post, "dopost") != 0) 
		notFound(cSocket); 
	// WaitForNext(); 
	text_len = WaitForContentLen(cSocket); 
	state = 0; 
	for(i = 0; i < text_len; i++) {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) return ; 
		if(state == 0 && c == '=') {state = 1; j = 0; }
		else if(state == 1) 
			if(c == '&') {
			    login[j] = 0; 
			    state = 2; 
			}
			else login[j++] = c; 
		else if(state == 2 && c == '=') {state = 3; j = 0; }
		else if(state == 3) pass[j++] = c; 
	}
	pass[j] = 0; 
	
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }

	if(strcmp(login, LOGIN_ID) == 0) 
		if(strcmp(pass, LOGIN_PASS) == 0) sprintf(sndBuf, buf, mytime, hostname, strlen(success), mytime, success); 
	else sprintf(sndBuf, buf, mytime, hostname, strlen(error_pass), mytime, error_pass); 
	else sprintf(sndBuf, buf, mytime, hostname, strlen(error_id), mytime, error_id); 
	write(cSocket, sndBuf, strlen(sndBuf)); 
	return ; 
}

/*
int WaitForContentLen(int cSocket)
{
	int content_len = 0; 
	char str_content_len[] = "Content-Length: "; 
	int i, bytes; 
	char c , state = 2, temp[20]; 
	while(state != 3) {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) break; 
		if(state == 0 && c == '\r') state = 1; 
		else if(state == 1 && c == '\n') state = 2; 
		else if(state == 2) {
			for(i = 0; i < strlen(str_content_len); i++) {
				bytes = read(cSocket, &c, 1); 
				if(bytes <= 0) break; 
				if(c == '\r') {state = 1; break; }
				else if(c == '\n') {state = 2; break; }
				temp[i] = c; 
}
			temp[i] = 0;
			if(bytes <= 0) break; 
			if(strcmp(temp, str_content_len) == 0) {
				while(c != '\r') {
					bytes = read(cSocket, &c, 1); 
					if(bytes <= 0) break; 
					content_len = content_len * 10 + c - '0'; 
}
				WaitForNext(cSocket, 1); 
				state = 3; 
}
}
		else state = 0; 
}
	return content_len; 
}
*/

/*
	do {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) return ; 
}while(c != '='); 
	i = 0; 
	while(c != '&') {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) return ; 
		login[i++] = c; 
}
	login[i] = 0; 
	do {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) return ; 
}while(c != '='); 
*/
