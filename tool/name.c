#include <stdio.h>
#include <unistd.h>

#include <sys/utsname.h>

int main()
{
	char hostname[100]; 
	int err; 
	
	struct utsname *myname; 
	
	err = gethostname(hostname, sizeof(hostname)); 
	if(err != 0) {printf("gethostname failed!\n"); return 1; }
	printf("My hostname is %s\n", hostname); 
	
	err = uname(myname); 
	if(err != 0) {printf("uname failed!\n"); return 1; }
	puts(myname -> sysname); 
	puts(myname -> nodename); 
	puts(myname -> release); 
	puts(myname -> version); 
	puts(myname -> machine); 
#ifdef _GNU_SOURCE
	puts(myname -> domainname); 
#endif
	
	return 0; 
}

