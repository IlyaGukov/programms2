#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#define BUF_SIZE 400
#define PATHFILE "./client.c"
#define FOR_EVERYBODY 2
#define FOR_EVERYONE 1
#define HELLO 100
int* matrix;
int* vector;
int msg_id;
int* result_vector;
struct data_sv 
{
        int size;
        int num_of_iter;
	int start_sum;
	int start_mat;
}; 
struct from_sv 
{
       long mtype;
       struct data_sv data;
};
struct hellomsg 
{
        long mtype;
        char mtext[1];
};
struct for_receiving
{
	long mtype;
	int buffer[BUF_SIZE]; 
};
char* normal_strcat(char* s1, char* s2, int size, int rr);

int* result_vector = NULL;

int deg(int k);

int get_number(char* st);

int number_size = 0;

int my_error(char* s);

int get_client_number(char* st);

void send_data(int* data, long type, int msg_id, int size);

int* receive_vector(int size, int msg_id, long type);

void* for_thread(void* arg);

void write_result_to_file(int *result,int size, int fd);

int main(int argc, char** argv)
{
	if (argc < 2) my_error("no number of clients \n");
	int client_number;
	client_number = get_client_number(argv[1]);
	int fd;
	if ((fd = open("problem.txt",O_RDONLY)) < 0) my_error ("can not open a file \n");
	int result, i = 0, n = 0, size = 0, pre_matrix = 0;
	char* workstr = NULL;
	workstr = (char*)malloc(sizeof(char)*BUF_SIZE);
	char* restring = NULL;
	restring = (char*)malloc(sizeof(char));
	int strsize = 0;
	while((result = read(fd,workstr,BUF_SIZE)) > 0)
	{	
		restring = realloc(restring,sizeof(char)*(strsize + result));
		normal_strcat(restring,workstr,strsize,result);
		strsize+=result;
	}
	if (result < 0) my_error(" reading error \n");
	size = get_number(restring);
	pre_matrix = number_size + 1;
	matrix = (int*)malloc(sizeof(int)*(size*size));
	vector = (int*)malloc(sizeof(int)*size);
	i = 0;
	for (;i < (size*size); i++)	
	{	
		matrix[i] = get_number(restring + n + pre_matrix);
		n += (number_size + 1);	
	} 
	n++;
	i = 0;
	for (;i < size; i++)
	{	
		vector[i] = get_number(restring + n + pre_matrix);
		n += (number_size+1);
	}
	result_vector = (int*)calloc(size,sizeof(int));
	free(restring);
	free(workstr);
	key_t msg_key, sem_key;
	if (((msg_key = ftok(PATHFILE,(char)FOR_EVERYBODY)) < 0)) my_error("error while generating key for messages \n");
	umask(0);
	if ((msg_id = msgget(msg_key, IPC_CREAT | 0666)) < 0 ) my_error("error on getting messages\n");
	struct from_sv* ftr;
	ftr = (struct from_sv*)malloc(sizeof(struct from_sv)*client_number);
	int k = 0;
	for (; k < (client_number - 1); k++)
	{
		(ftr[k]).mtype = (long)(k + 1);
		(ftr[k]).data.start_mat = k*size*(size/client_number);
		(ftr[k]).data.size = size;
		(ftr[k]).data.num_of_iter = (size/client_number);
		(ftr[k]).data.start_sum = k*(ftr[k]).data.num_of_iter;
	}		
	pthread_t *thid;
	thid = (pthread_t*)malloc(sizeof(pthread_t)*client_number);
	k = 0;
	for (; k < (client_number - 1); k++) if (pthread_create(thid + k,NULL,&for_thread,ftr + k) != 0) my_error ("thread creating fault\n");
	(ftr[client_number - 1]).mtype = (long)client_number;
	(ftr[client_number - 1]).data.size = size;
	(ftr[client_number - 1]).data.num_of_iter = (size/client_number + size%client_number);
	(ftr[client_number - 1]).data.start_mat = (size*size - (size * (ftr[client_number - 1]).data.num_of_iter));
	(ftr[client_number - 1]).data.start_sum = (size - (ftr[client_number - 1]).data.num_of_iter );
	for_thread(ftr + client_number - 1);
	int j = 0;
	for (; j < (client_number - 1); j++ ) pthread_join(thid[j], NULL);
	free(thid);
	free(ftr);
	int fd1;
	if ((fd1 = open("result.txt", O_WRONLY | O_TRUNC | O_CREAT , 0666)) < 0) my_error("can not open result file\n");
	write_result_to_file(result_vector, size, fd1);
	if (close(fd) < 0) my_error("cannot close file \n");
	if (close(fd1) < 0) my_error("cannot close file \n");
	if ( msgctl ( msg_id , IPC_RMID , NULL ) < 0 ) my_error ("Can not delete message!\n");
	int sem_id;
	if ((sem_key = ftok(PATHFILE, FOR_EVERYONE)) < 0) my_error ("error while generating key for semaphor\n");
	if ((sem_id = semget (sem_key, 1 , 0666)) < 0) my_error ("error on getting semaphor\n");
        if (semctl (sem_id , 0 , IPC_RMID , 0) < 0) my_error (" can not delete semaphor\n");
	free(matrix);
	free(vector);
	free(result_vector);
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
		if ((st[i] <'0' || st[i]>'9') && st[i]!=' ') my_error("invalid size"); 
	
    	}
    	return size;
}
int deg(int k)    
{
    	int a=1,i=0;
    	for (;i<k;i++) a*=10;
    	return a;
}
int get_number(char* st)
{
    	int i=0, number=0;	
	while ((st[i] != ' ') && (st[i] != '\n'))
    	{
		if ((st[i]>='0') && (st[i] <='9')) number = number*10+st[i]-'0';
		if (((st[i] <'0') || (st[i]>'9')) && (st[i]!=' ')) my_error("invalid number \n");
		if ((st[i+1] == ' ') && (st[i+2] == '\n')) i++;
	i++; 
    	}
	number_size = i;
    	return number;
}
void send_data(int* data, long type, int msg_id, int size)
{
	struct for_receiving rvector;
	int i = 0, j = 0, k = 0;
	rvector.mtype = type;
	for (; i < size; i++)
	{
		rvector.buffer[j] = data[i];
		if ((j == (BUF_SIZE - 1)) || (i == (size - 1)))
		{
			if (msgsnd (msg_id , &rvector , (j + 1) * sizeof(int), 0) < 0) 
			{
                        	msgctl (msg_id , IPC_RMID , NULL );
                        	my_error ("can not send a message in send_data\n");
                	}
			j = (-1);
		}
		j++;
		
	}
	return;
}
void* for_thread(void* arg)
{
	struct from_sv* data;
	data = (struct from_sv*)arg;
	struct hellomsg hello_msg;
	int* rvector;
	if ( msgrcv (msg_id , &hello_msg, 0 , data->mtype * HELLO , 0) < 0 ) my_error (" can not recieve a hello message\n");
	if (msgsnd (msg_id ,data ,sizeof(struct data_sv), 0) < 0)
	{
		msgctl (msg_id , IPC_RMID , NULL );
		my_error ("can not send a message!\n");
	}
	send_data (matrix, data->mtype , msg_id, (data->data.size)*(data->data.size));
	send_data (vector ,data->mtype , msg_id, data->data.size);
	rvector = receive_vector (data->data.num_of_iter, msg_id, (data->mtype) * HELLO);
	hello_msg.mtype = data->mtype;
	if (msgsnd (msg_id ,&hello_msg ,0 , 0) < 0) //BYE
	{
		msgctl (msg_id , IPC_RMID , NULL );
		my_error ("can not send a message\n");
	}
	int i = 0;
	for (; i < data->data.num_of_iter; i++) result_vector[i + data->data.start_sum] = rvector[i];
	free (rvector);
	return;
}
int* receive_vector(int size, int msg_id, long type)
{
	int* vector;
	vector = (int*)malloc(sizeof(int)*size);
	int i = 0, result = 0, j = 0, k = 0;
	struct for_receiving rvector;
	for (;i < size * sizeof(int); i += result)
	{
		if ((result = msgrcv(msg_id, &rvector, BUF_SIZE * sizeof(int), type, 0)) < 0) my_error("error while receiving result vector \n");
		k = result/sizeof(int);
		for (; j < k; j++) vector[i + j] = rvector.buffer[j];
		j = 0;
	}
	return vector;
}
void write_result_to_file(int *result, int size, int fd)
{ 
	int k = 0, l = 1, j = 0, element;
	char* forfile;
	forfile = (char*)malloc(sizeof(char));
	for (j=0;j<size;j++)
	{
		element = result[j];
		while ((element/deg(l)) !=0) l++; 
		for (; k < l; k ++)
		{
		 	forfile[0] = (char)(element/deg(l-k-1)) + '0';
			element %= deg(l-1-k); 
			if (write( fd, forfile, 1) < 0) my_error("cannot write to file \n");
		}
		if (write( fd," ", 1) < 0) my_error("cannot write to file \n");
		k = 0;
		l = 1;
	}
	free (forfile);
	return;
}
char* normal_strcat(char* s1, char* s2, int size, int rr)
{
    	int i = 0;
	for(;i<rr;i++) s1[i + size] = s2[i];
    	return s1;
}
