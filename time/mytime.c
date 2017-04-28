#include <stdio.h>
#include <time.h>
#include <string.h>

int main()
{
	time_t t; 
	struct tm *gmt; 
	char *temp; 
	
	t = time(NULL); 
	gmt = gmtime(&t); 
	printf("GMT: %s\nLocal: %s\n", asctime(gmt), asctime(localtime(&t))); 
	temp = strchr(asctime(gmt), '\n'); 
	printf("Loc: %d %d\n", strlen(asctime(gmt)), strlen(temp)); 

	return 0; 
}

