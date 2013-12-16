#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <signal.h>
#define FOR_EVERYBODY 2
#define DATE_SIZE 26
#define SIZE_OF_PERMISSIONS 10
#define TYPE_SIZE 20
int k = 0;
struct about_file
{
	size_t string_length;
	char creation_date[DATE_SIZE];
	char last_changes_date[DATE_SIZE];
	off_t file_size;
	char permissions[SIZE_OF_PERMISSIONS];
	size_t file_offset;
	char file_type[TYPE_SIZE];
};

void my_handler(int nsig);

int my_error(char* s);

int main()
{
	while(k != 1) (void)signal(SIGUSR1, my_handler);
	int fd, i = 0, file_count = 0; 
	size_t size = 0;
	if ((fd = open("result.txt", O_RDWR | O_CREAT , 0666)) < 0) my_error("can not open result file\n");
	struct about_file *ptr1, *ptr2;
	if ((ptr1 = (struct about_file* )mmap(NULL, sizeof(size_t), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) my_error("mapping failed \n");
	ptr2 = ptr1;
	size = *((size_t*)ptr2);
	ptr2 ++;
	if (munmap ((void*)ptr1, size) < 0 ) my_error ("error with munmap \n"); 
	if ((ptr1 = (struct about_file* )mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) my_error("mapping failed \n");
	if (close(fd) < 0) my_error("cannot close file \n");
	ptr2 = (ptr1 + 1);
	file_count = *((int*)ptr2);
	ptr2 ++; 
	printf("number of all files is: %i \n",file_count);
	for (; i < file_count; i++)
	{
		printf ("\nfile name\towner\tgroup\n"); // у меня есть две области - структур и строк. структуры лежат в начале и легко 
		printf("%s\n\n",(char*)((char*)ptr1 + ptr2->file_offset)); //находятся, строки вытаскиваютя благодаря соответсвующему смещению
		printf("file permissions is: %s \n",ptr2->permissions);
		printf("creation date is: %s",ptr2 -> creation_date);
                printf("last changes date is: %s",ptr2->last_changes_date );
		printf("file type is: %s", ptr2->file_type);
		printf("file size is: %i \n", (int)ptr2->file_size);
		ptr2 ++;
	}
	if (munmap ((void*)ptr1, size) < 0 ) my_error ("error with munmap \n");
	return 0;	
}
int my_error(char* s)
{ 
    	int n;
    	n = strlen(s);
    	write(2,s,n+1);
    	exit(-1);
}
void my_handler(int nsig)
{
	k++;
	return; 	
}
