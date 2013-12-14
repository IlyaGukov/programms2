#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#define path_to_first "./first"
#define path_to_second "./second"
int my_error(char* s);

int main(int argc, char** argv)
{
	if (argc < 2) my_error ("invalid input of directory \n");
	int fr = 2;
	char for_first[] = path_to_first;
	char for_second[] = path_to_second;
	fr = fork();
	if (fr < 0) my_error( "fork error \n" );
	if (fr == 0) 
	{
		if ( execl(for_first,for_first,argv[1],NULL) < 0) my_error("execl error on first \n");
	}
	if (fr > 0)
	{
		if ( execl(for_second,for_second,NULL) < 0) my_error("execl error on second \n");
	}
	return 0;	
}
int my_error(char* s)
{ 
    	int n;
    	n = strlen(s);
    	write(2,s,n+1);
    	exit(-1);
}	
