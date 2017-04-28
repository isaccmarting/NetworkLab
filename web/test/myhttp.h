#ifndef MYHTTP_H
#define MYHTTP_H

#define MAX_METHOD_LEN 10
#define MAX_FILENAME_LEN 50 
#define MAX_VERSION_LEN 15

struct http_head_ {
    char method[MAX_METHOD_LEN]; 
	char filename[MAX_FILENAME_LEN]; 
	char version[MAX_VERSION_LEN]; 
}; 
typedef struct http_head_* PtrTohttp_head_; 
typedef PtrTohttp_head_ http_head; 

extern http_head getHead(int cSocket); 
extern void sendFile(int cSocket, char *filename); 

#endif
