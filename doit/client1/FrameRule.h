#define REQ_DISC 0x00
#define REQ_TIME 0x01
#define REQ_NAME 0x02
#define REQ_LIST 0x03
#define REQ_INFO 0x04

#define ANS_TIME 0x81
#define ANS_NAME 0x82
#define ANS_LIST 0x83
#define ANS_INFO 0x84
#define ANS_CONN 0x05

#define INS_INFO 0x44

#define BEGIN0 0xAA
#define BEGIN1 0x33
#define BEGIN2 0x55

typedef unsigned char byte; 
typedef unsigned short half; 
typedef unsigned long word; 

struct FrameHead {
	byte begin[3]; 
	byte type; 
	word num; 
	word length; 
}; 
typedef struct FrameHead *PtrToFrameHead; 
typedef PtrToFrameHead pFrameHead; 

extern int fatal(char *string); 
extern int writeHead(int mySocket, byte type, word length); 
extern pFrameHead readHead(int mySocket); 
extern int readFrame(int mySocket, char* buf, int size_frame); 
