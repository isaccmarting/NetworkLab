#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_t ntid1, ntid2; 

void printids(const char *s)
{
    pid_t pid; 
    pthread_t tid; 
    pid = getpid(); 
    tid = pthread_self(); 
    printf("%s pid = %u tid = %u (0x%x) \n", s, (unsigned int)pid, 
        (unsigned int)tid, (unsigned int)tid); 
}

void *thread_fun(void *arg)
{
    printids(arg); 
    return NULL; 
}

int main(void)
{
    int err; 
    /**/
    err = pthread_create(&ntid1, NULL, thread_fun, "I'm the new thread1: "); 
    if(err != 0) {
        fprintf(stderr, "Creating thread failed: %s\n", strerror(err)); 
        exit(1); 
    }
    err = pthread_create(&ntid2, NULL, thread_fun, "I'm the new thread2: "); 
    if(err != 0) {
        fprintf(stderr, "Creating thread failed: %s\n", strerror(err)); 
        exit(1); 
    }
    printids("I'm Father Process: "); 
    pthread_join(ntid1, NULL); 
    pthread_join(ntid2, NULL); 
    
    return 0; 
}
