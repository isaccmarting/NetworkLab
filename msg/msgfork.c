#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

struct msgmbuf {
	long msg_type; 
	char msg_text[512]; 
};

int main()
{
    int qid, len; 
    int pid; 
    key_t key; 
    struct msgmbuf msg;

    if((key = ftok(".", 'a')) == -1) {
        perror("Key error!\n"); 
        exit(1); 
    }
    
    if((qid = msgget(key, IPC_CREAT | 0666)) == -1) {
        perror("Creating message queue error!\n"); 
        exit(1); 
    }
    printf("The queue number is %d\n", qid); 

    pid = fork(); 
    if(pid > 0) {
        printf("I'm Father Process. \n"); 
        puts("Please input the message to put in the queue: "); 
        /**/
        if((fgets((&msg) -> msg_text, 512, stdin)) == NULL) {
            puts("No messages!\n"); 
            exit(1); 
        }
        msg.msg_type = getpid(); 
        len = strlen(msg.msg_text); 
        /**/
        if((msgsnd(qid, &msg, len, 0)) < 0) {
            perror("Adding message error!\n"); 
            exit(1); 
        }
    }
    else {
        printf("I'm Child Process!\n"); 
        /**/
        if((msgrcv(qid, &msg, 512, 0, 0)) < 0) {
            perror("Reading message error!\n"); 
            exit(1); 
        }
        printf("The message is %s\n", (&msg)->msg_text); 
        /**/
        if((msgctl(qid, IPC_RMID, NULL)) < 0) {
            perror("Deleting message queue error!\n"); 
            exit(1); 
        }
    }

    exit(0); 
}


