#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <string.h>
#define path_to_server "./server"
#define path_to_client "./client"
int number_length(int i);

char* st_num(int i);

int my_error(char* s);

int get_client_number(char* st);

int deg(int k);

int main (int argc, char** argv)
{
	int client_number, fd;
	if (argc < 2) my_error("not enough arguments \n");
	client_number = get_client_number(argv[1]);
	int fr;
	char for_server[] = path_to_server;
	char for_client[] = path_to_client;
	fr = fork();
	if (fr < 0) my_error( "fork error \n" );
	if (fr == 0) 
	{
		if ( execl(for_server,for_server,argv[1],NULL) < 0) my_error("execl error on server /n");
	}
	int i = 0;
	char* client_order;
	for (;i < client_number; i++ )
	{
		fr = fork();
		if (fr < 0) my_error( "fork error \n" );
		if (fr == 0) 
		{
			client_order = st_num(i + 1);  
			if ( execl(for_client,for_client,client_order,NULL) < 0) my_error("execl error on client /n");
		}	
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

int get_client_number(char* st)
{
    	int i=0, size=0;	
	for(;i<strlen(st);i++)
    	{
		if ((st[i]>='0') && (st[i] <='9')) size=size*10+st[i]-'0';
		if ((st[i] <'0' || st[i]>'9') && st[i]!=' ') error("invalid size"); 
	
    	}
    	return size;
}

int deg(int k)    
{
    int a=1,i=0;
    for (;i<k;i++) a*=10;
    return a;
}

int number_length(int i)
{
	int l = 1;
	while ((i/deg(l)) !=0) l++;
	return l;
}

char* st_num(int i)
{
	char* st;
	int m;
	m = number_length(i+1);
	st = (char*)malloc(sizeof(char)*m);
	int k = 0;
	for (; k < m; k++)
	{
		st[k] = (char)(i/deg(m-k-1)) + '0';
		i%=deg(m-1-k);
	}
	return(st);
}

