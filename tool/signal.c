#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void CtrlC(int signalno)
{
	printf("Hello, world!\n"); 
	_exit(0); 
}

int main()
{
	signal(SIGINT, CtrlC); 
	while(1) {
	}
	return 0; 
}

