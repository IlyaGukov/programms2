#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define BUF_SIZE 400
#define PATHFILE "./client.c"
#define FOR_EVERYONE 1
#define FOR_EVERYBODY 2
#define HELLO 100
char* log_sign = NULL;

struct for_receiving
{
	long mtype;
	int buffer[BUF_SIZE]; 
};

void send_result(int* result, long type, int msg_id, int size);

void write_to_log_file(char* st, int fd, int sem_id);

int* receive_vector(int size, int msg_id, long type);

void do_A(int sem_id);

void do_D(int sem_id);

int my_error(char* s);

int get_client_id(char* st);

int main (int argc, char** argv)
{
	if (argc < 2) my_error("no client id \n");
	int cid, fd_log;
	
	cid = get_client_id(argv[1]);
	struct hellomsg 
	{
                long mtype;
                char mtext[1];
        } hello_msg;
	hello_msg.mtype = (long)(cid*HELLO);
	umask (0);
	log_sign = (char*)malloc(sizeof(char)*(strlen("client \0") + strlen(argv[1]) + 2));
	strcpy (log_sign,"client \0");
	strcat (log_sign,argv[1]);
	strcat (log_sign," \0");
	key_t msg_key, sem_key;
	int sem_id, msg_id = 0;;
	if (((msg_key = ftok(PATHFILE,(char)FOR_EVERYBODY) ) < 0)) my_error("error while generating key for messages \n");
        if ((sem_key = ftok(PATHFILE, FOR_EVERYONE)) < 0) my_error ("error while generating key for semaphor\n");
        if ((sem_id = semget (sem_key, 1 , IPC_CREAT | 0666)) < 0) my_error ("error on getting semaphor\n");
	if (cid == 1) 
	{
		do_A(sem_id);
		if ((fd_log = open ("log.txt" , O_WRONLY | O_CREAT | O_TRUNC | O_APPEND , 0666)) < 0) my_error ("error on opening file log\n");
	}
	else if ((fd_log = open ("log.txt" , O_WRONLY | O_CREAT | O_APPEND , 0666)) < 0) my_error ("error on opening file log \n");
	write_to_log_file("is ready!\n", fd_log ,sem_id);
	if ((msg_id = msgget(msg_key, IPC_CREAT | 0666)) < 0 ) my_error("error on getting messages\n");
	if (msgsnd (msg_id, &hello_msg , 0, 0) < 0) 
	{
                msgctl (msg_id , IPC_RMID , NULL );
                my_error ("can not send a hello message\n");
        }
	int* vector;
	int* matrix;
	struct from_sv 
	{
                long mtype;
                struct data_sv 
		{
                        int size;
                        int num_of_iter;
                        int start_sum;
			int start_mat;
                } data;
        } do_work_msg;
	do_work_msg.mtype = cid;
	if ( msgrcv (msg_id , &do_work_msg, sizeof(struct data_sv), cid, 0) < 0) my_error ("can not recieve a message\n"); 
	int num_of_iter, size, start_sum, start_mat;
	size = do_work_msg.data.size;
	num_of_iter = do_work_msg.data.num_of_iter;
	start_sum = do_work_msg.data.start_sum;
	start_mat = do_work_msg.data.start_mat;
	matrix = receive_vector(size*size, msg_id, cid );
	vector = receive_vector(size, msg_id, cid );
	write_to_log_file ("message with data for calculation recieved\n" , fd_log, sem_id);
	int* result_vector;
	result_vector = (int*) malloc (num_of_iter*sizeof(int));
	int i = 0, j = 0, k = 0;
	for (;i < num_of_iter; i++)
	{
		for (; j < size; j++) result_vector[k] += (matrix[start_mat + j + (i * size)] * vector[j]);
		k++;
		j = 0;
	}
	write_to_log_file ( "calculated!\n" , fd_log, sem_id);
	send_result (result_vector, cid * HELLO, msg_id, num_of_iter);
	free (vector);
	free (matrix);
	free (log_sign);
	if ( msgrcv ( msg_id , &hello_msg, 0 , cid , 0 ) < 0) my_error ("can not recieve a message\n");  //BYE
	free (result_vector);  		
	return 0;
}

int my_error(char* s)
{ 
    	int n;
    	n = strlen(s);
    	write(2,s,n+1);
    	exit(-1);
}

int get_client_id(char* st)
{
    	int i=0, size=0;	
	for(;i<strlen(st);i++)
    	{
		if ((st[i]>='0') && (st[i] <='9')) size=size*10+st[i]-'0';
		if ((st[i] <'0' || st[i]>'9') && st[i]!=' ') error("invalid size"); 
	
    	}
    	return size;
}
void write_to_log_file(char* st, int fd, int sem_id)
{
	do_D (sem_id);
	if ((write(fd,log_sign,strlen(log_sign))) < 0) my_error("error while writing to log.txt \n");
	if ((write(fd,st,strlen(st))) < 0) my_error("error while writing to log.txt \n");
	do_A (sem_id);
	return;
}
void do_A(int sem_id)
{
        struct sembuf sem;        
        sem.sem_op = 1;
        sem.sem_flg = 0;
        sem.sem_num = 0;
        if (semop (sem_id, &sem, 1) < 0) my_error ("error while doing A\n");
        return;
}	
void do_D(int sem_id)
{
        struct sembuf sem;        
        sem.sem_op = -1;
        sem.sem_flg = 0;
        sem.sem_num = 0;
        if (semop (sem_id, &sem, 1) < 0) my_error ("error while doing D\n");
        return;
}	
int* receive_vector(int size, int msg_id, long type)
{
	int* vector;
	vector = (int*)malloc(sizeof(int)*size);
	int i = 0, result = 0, j = 0, k = 0;
	struct for_receiving rvector;
	for (;i < size*sizeof(int); i += result)
	{
		if ((result = msgrcv(msg_id, &rvector ,BUF_SIZE * sizeof(int), type, 0)) < 0) my_error("error while receiving vector \n");
		k = result/sizeof(int);
		for (; j < k; j++) vector[(i / sizeof(int)) + j] = rvector.buffer[j];
		j = 0;
	}
	return vector;
}
void send_result(int* result, long type, int msg_id, int size)
{
	struct for_receiving rvector;
	int i = 0, j = 0, k = 0;
	rvector.mtype = type;
	for (; i < size; i++)
	{
		rvector.buffer[j] = result[i];
		if ((j == (BUF_SIZE - 1)) || (i == (size - 1)))
		{
			if (msgsnd (msg_id , &rvector , (j + 1) * sizeof(int), 0) < 0) 
			{
                        	msgctl (msg_id , IPC_RMID , NULL );
                        	my_error ("can not send a message\n");
                	}
			j = (-1);
		}
		j++;
	}
	return;
}
			 
		

