#include <stdio.h>
#include <time.h>
 
int main()
{
	time_t timer; 
	struct tm *tblock; 
	timer = time(NULL); 
	tblock = localtime(&timer); 
	printf("Time is %s\n", asctime(tblock)); 
	return 0; 
}

