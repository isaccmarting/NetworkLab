// request sign 
#define REQ_DISC 0x00
#define REQ_TIME 0x01
#define REQ_NAME 0x02
#define REQ_LIST 0x03
#define REQ_INFO 0x04

// answer sign 
#define ANS_TIME 0x81
#define ANS_NAME 0x82
#define ANS_LIST 0x83
#define ANS_INFO 0x84
#define ANS_CONN 0x05

// instruction sign 
#define INS_INFO 0x44

// the symbol to identify the boundary 
#define BEGIN0 0xAA
#define BEGIN1 0x33
#define BEGIN2 0x55

typedef unsigned char byte; 
typedef unsigned short half; 
typedef unsigned long word; 

// the head of the frame 
struct FrameHead {
	byte begin[3]; 		// symbols to identify the boundary 
	byte type; 			// type of the packet 
	word num; 			// the sequence number of the packet 
	word length; 		// the length of contents 
}; 
typedef struct FrameHead *PtrToFrameHead; 
typedef PtrToFrameHead pFrameHead; 

extern int fatal(char *string); 
extern int writePacket(int mySocket, byte type, char* buf, word length); 
extern char* readPacket(int mySocket, pFrameHead myHead); 
