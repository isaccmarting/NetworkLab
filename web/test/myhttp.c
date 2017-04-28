#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include "myhttp.h"

// used to adjust the http path to the server file path 
#define PRE_FILE_PATH "."
// the length of char temp[] to store file contents 
#define MAX_FILE_TEMP_LEN 512
// the length of char login[] and pass[] to store login id and password 
#define MAX_LOGIN_PASS_LEN 50
// the correct login id 
#define LOGIN_ID "3140102442"
// the correct login password 
#define LOGIN_PASS "2442"

// transform the file suffix to the http file type 
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

// go through the rest http head 
void WaitForNext(int cSocket, char init_state)
{
	char c, state = init_state; 
	int bytes; 
	// go out until meeting "\r\n\r\n" 
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

// go through the rest http head and get the Content-Length 
int WaitForContentLen(int cSocket)
{
	int content_len = 0; 
	char str_content_len[] = "Content-Length: "; 
	int i, bytes; 
	char c , state = 2, temp[20]; 
	// record the string after "\r\n"
	while(state != 4) {
		bytes = read(cSocket, &c, 1); 
		if(bytes <= 0) break; 
		if(state == 0 && c == '\r') state = 1; 
		else if(state == 1 && c == '\n') {state = 2; i = 0; }
		else if(state == 2) {
			// store fixed-length string after get "\r\n"
			if(c != str_content_len[i]) 
				// if meet '\r' or '\n' then restart 
				if(c == '\r') state = 1; 
			    else if(c == '\n') {state = 2; i = 0; }
				else state = 0; 
			else {
				i++; 
				// after get string "Content-Length: "
				if(i == strlen(str_content_len)) 
					state = 3; 
			}
		}
		else if(state == 3) {
			// get the Content-Length 
			if(c == '\r') {state = 4; WaitForNext(cSocket, 1); }
			else content_len = content_len * 10 + c - '0'; 
		}
		else state = 0; 
	}
	return content_len; 
}

// go through the http head to get method, file name and version 
http_head getHead(int cSocket)
{
	http_head pHttpHead; 
	int i; 
	char c; 
	int bytes; 
	pHttpHead = (http_head) malloc(sizeof(struct http_head_)); 
	if(pHttpHead == NULL) {printf("No memory for pHttpHead!\n"); exit(1); }
	memset(pHttpHead, 0, sizeof(struct http_head_)); 
	// get method 
	for(i = 0; i < MAX_METHOD_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++) {
		if(c != '\r')
			pHttpHead -> method[i] = c; 
	}
	pHttpHead -> method[i] = 0; 
	// get file name 
	for(i = 0; i < MAX_FILENAME_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++)
		if(c != '\r')
			pHttpHead -> filename[i] = c; 
	pHttpHead -> filename[i] = 0; 
	// get version 
	for(i = 0; i < MAX_VERSION_LEN && read(cSocket, &c, 1) > 0 && c != ' ' && c != '\n'; i++)
		if(c != '\r')
			pHttpHead -> version[i] = c; 
	pHttpHead -> version[i] = 0; 
	return pHttpHead; 
}

// free the http_head 
void freeHead(http_head pHttpHead)
{
	free(pHttpHead); 
	return ; 
}

// deal with not implemented methods 
void notImplemented(int cSocket)
{
	// the temporary to transfer 
	char buf[] = "HTTP/1.1 501 Not Implemented\r\nDate:  %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char text[] = "<html><title>Not Implemented</title><body><p>The server does not recognize the request method. </p></body></html>"; 
	char sendbuf[500]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	// get GMT time 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	// get host name 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	// get all information to transfer 
	sprintf(sendbuf, buf, mytime,  hostname, strlen(text), mytime, text); 
	// tranfer to the client 
	write(cSocket, sendbuf, strlen(sendbuf)); 
	return ; 
}

// deal with not found files and invalid dopost 
void notFound(int cSocket)
{
	// the temporary to transfer 
	char buf[] = "HTTP/1.1 404 File Not Found\r\nDate:  %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char text[] = "<html><title>File Not Found</title><body><p>The server could not find the resource.</p></body></html>"; 
	char sendbuf[500]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	// get GMT time 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	// get host name 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	// get all information to transfer 
	sprintf(sendbuf, buf, mytime,  hostname, strlen(text), mytime, text); 
	// tranfer to the client 
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

// tansfer file suffix to http file type 
char* ftype(char *filename)
{
	int i, len; 
	char *suffix; 
	// get file suffix 
	suffix = strrchr(filename, '.'); 
	suffix += 1; 
	// compare and get the respected http file type 
	len = sizeof(file_types) / sizeof(file_types[0]); 
	for(i = 0; i < len; i++)
		if(strcmp(suffix, file_types[i].suffix) == 0)
			return file_types[i].mytype; 
	return NULL; 
}

// deal with GET method 
// send file contents to clients or notFound 
void sendFile(int cSocket, char *filename)
{
	// the temporary to transfer 
	char buf[] = "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n"; 
	char fText[MAX_FILE_TEMP_LEN], headBuf[300]; 
	FILE *pFile; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	char fPath[50], *fType; 
	unsigned long fLength; 
	int bytes; 
	// get the server file path 
	strcpy(fPath, PRE_FILE_PATH); 
	strcat(fPath, filename); 
	// whether the file exists 
	pFile = fopen(fPath, "rb"); 
	if(pFile == NULL) {
		printf("No file %s!\n", fPath); 
		notFound(cSocket); 
		return ; 
	}
	
	// get GMT time 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	// get host name 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }
	
	// get http file type 
	fType = ftype(filename); 
	fseek(pFile, 0, SEEK_END); 
	fLength = ftell(pFile); 
	fseek(pFile, 0, SEEK_SET); 
	
	// get all information to transfer 
	sprintf(headBuf, buf, mytime, hostname, fType, fLength, mytime); 
	// tranfer to the client 
	write(cSocket, headBuf, strlen(headBuf)); 
	
	// read and transfer file contents 
	while(1) {
		bytes = fread(fText, sizeof(fText[0]), MAX_FILE_TEMP_LEN, pFile); 
		if(bytes <= 0) break; 
		write(cSocket, fText, bytes); 
	}
	fclose(pFile); 
	return ; 
}

// deal with POST method 
void doPost(int cSocket, char *filename)
{
	char *post, c; 
	int bytes, i, text_len, j; 
	char state; 
	char login[MAX_LOGIN_PASS_LEN], pass[MAX_LOGIN_PASS_LEN]; 
	char *mytime, hostname[30]; 
	int err; 
	time_t t; 
	// the temporary to transfer 
	char buf[] = "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-type: text/html\r\nContent-length: %d\r\nCache-Control: max-age=0\r\nExpires: %s\r\nVary: Accept-Encoding\r\n\r\n%s"; 
	char error_id[] = "<html><title>Login Error!</title><body><p>Login id error!</p></body></html>"; 
	char error_pass[] = "<html><title>Login Error!</title><body><p>Login password error!</p></body></html>"; 
	char success[] = "<html><title>Login Success!</title><body><p>Login success, welcome!</p></body></html>"; 
	char sndBuf[300]; 
	if(filename == NULL) notFound(cSocket); 
	// get the exact name 
	post = strrchr(filename, '/'); 
	post += 1; 
	if(strcmp(post, "dopost") != 0) 
		notFound(cSocket); 
	// get Content-Length 
	text_len = WaitForContentLen(cSocket); 
	// get login id and password 
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
	
	// get GMT time 
	t = time(NULL); 
	mytime = asctime(gmtime(&t)); 
	mytime[strlen(mytime)-1] = ' '; 
	strcat(mytime, "GMT"); 
	// get host name 
	err = gethostname(hostname, sizeof(hostname)); 
	if(err < 0) {printf("Error gethostname!\n"); exit(1); }

	// get all information to transfer 
	if(strcmp(login, LOGIN_ID) == 0) 
		// correct id and password 
		if(strcmp(pass, LOGIN_PASS) == 0) sprintf(sndBuf, buf, mytime, hostname, strlen(success), mytime, success); 
	    // correct id but wrong password 
	    else sprintf(sndBuf, buf, mytime, hostname, strlen(error_pass), mytime, error_pass); 
	// wrong id 
	else sprintf(sndBuf, buf, mytime, hostname, strlen(error_id), mytime, error_id); 
	// tranfer to the client 
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
