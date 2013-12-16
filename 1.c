#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <pwd.h>
#include <grp.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/msg.h>
#include <signal.h>
#define PATHFILE "./1.c"
#define FOR_EVERYBODY 2
#define DATE_SIZE 26
#define SIZE_OF_PERMISSIONS 10
#define TYPE_SIZE 20
#define HELLO 100
struct about_file //структура постоянного размера
{
	size_t string_length;
	char creation_date[DATE_SIZE]; //формат даты постоянный
	char last_changes_date[DATE_SIZE];// тоже самое
	off_t file_size;
	char permissions[SIZE_OF_PERMISSIONS]; // 9 - сами права и 1 - конец строки
	size_t file_offset;
	char file_type[TYPE_SIZE]; //размер самого длиного типа, ну как - то так
};
char** string_buffer;
size_t size = 0;
int my_error(char* s);

char* get_path(char* file_name, char* arg);

char* get_permissions(mode_t st_mode);

struct about_file get_data(struct stat *buf, char* file_name, int i);

int main(int argc, char** argv)
{
	if (argc < 2) my_error ("invalid input of directory \n");
	DIR* direct;
	if ((argv[1] = realpath (argv[1],NULL)) == NULL) my_error("no such directory exists \n");
	if ((direct = opendir(argv[1])) == NULL) my_error("error while open direstory \n");
	struct dirent** readres;
	struct stat* buf;
	readres = (struct dirent**)malloc(sizeof(struct dirent*));
	int fd, i = 0, file_count = 0;
	if ((fd = open("result.txt", O_RDWR | O_TRUNC | O_CREAT , 0666)) < 0) my_error("can not open result file\n");
	while ((readres[file_count] = readdir(direct)) != NULL) // получаю количество интересных файлов, которые лежат непосредственно в   
	{	// самой папке
		if ((strcmp ((readres[file_count]) -> d_name, ".") != 0) && (strcmp((readres[file_count]) -> d_name, ".." ) != 0))
		{
			file_count ++;
			readres = (struct dirent**) realloc (readres, (file_count + 1) * sizeof(struct dirent*));
		}
	}
	struct about_file* file_data;
	buf = (struct stat*)malloc(sizeof(struct stat));
	file_data = (struct about_file*)malloc(sizeof(struct about_file) * file_count);
	string_buffer = (char** )malloc(sizeof(char*) * file_count); //для имени, хозяина и группы  используется этот массив строк
	size = (file_count + 2) * sizeof(struct about_file); // в него они пишутся одной строкой
	char* path;
	for (; i < file_count; i++) //узнаю о них все, что надо
	{
	 	if ((strcmp ((readres[i]) -> d_name, ".") != 0) && (strcmp((readres[i]) -> d_name, ".." ) != 0))
		{
			path = get_path((readres[i]) -> d_name, argv[1]);
			printf("%s \n", path);
			if (lstat(path, buf) < 0) my_error("error with stat \n");
			file_data[i] = get_data( buf, (readres[i]) -> d_name, i ); // пишу в структуру все, кроме велечин переменного размера
		} // - имя, хозяин, группа.   
		else i--;
	}
	
	size += (file_data[file_count - 1].string_length ) ; // размер памяти, которую буду отборажать в адресное пространство
	if (ftruncate(fd, size) < 0) my_error("truncate error \n");
	void *ptr1, *ptr2;
	if ((ptr1 = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) my_error("mapping failed \n");
	if (close(fd) < 0) my_error("cannot close file \n");
	ptr2 = ptr1;
	i = 0;
	*((size_t* )ptr2) = size;
	ptr2 += sizeof(struct about_file);
	*((int* )ptr2) = file_count;
	ptr2 += sizeof(struct about_file);
	for (; i < file_count; i++) // пишу в файл сначала структуры
	{
		*((struct about_file* )(ptr2)) = (file_data[i]) ;
                ptr2 += sizeof(struct about_file);	
	}
	i = 0;
	for (; i < file_count; i++) // теперь строки. в структре хранится смещение относитель начала файла, чтобы потом отыскать 						//соответстующую строку
	{
		strcpy(((char* )ptr2),(string_buffer[i]));
                ptr2 += (strlen(string_buffer[i]) + 1);	
	}
	pid_t ppid;
	ppid = getppid();
	kill (ppid, SIGUSR1);
	if (munmap ((void*)ptr1, size) < 0 ) my_error ("error with munap \n");
	//free (readres);
	free (path);
	free (buf);
	return 0;
	
}
int my_error(char* s)
{ 
    	int n;
    	n = strlen(s);
    	write(2,s,n+1);
    	exit(-1);
}
struct about_file get_data(struct stat *buf, char* file_name, int i)
{
	char *user_name, *group_name, *permissions, *file_type, *result;
	struct passwd* about_user = NULL; 
        struct group* about_group = NULL;
	struct about_file work_data;
	if ((about_user = getpwuid(buf -> st_uid)) == NULL) my_error("error while getting information about user \n");
	if ((about_group = getgrgid(buf -> st_gid)) == NULL) my_error("error while getting information about group \n"); 
	user_name = (char*)malloc(sizeof(char) * (strlen(about_user -> pw_name) +1));
	strcpy(user_name, about_user -> pw_name);
	group_name = (char*)malloc(sizeof(char) * (strlen(about_group -> gr_name) +1));
	strcpy (group_name, about_group -> gr_name);
	string_buffer[i] = (char*)malloc((strlen(group_name) + strlen(user_name) + strlen(file_name) + 3) * sizeof(char));
	strcpy(string_buffer[i],file_name);
	strcat(string_buffer[i],"\t");
	strcat(string_buffer[i], user_name);
	strcat(string_buffer[i],"\t");
	strcat(string_buffer[i], group_name); // склеил строку с именем, хозяином и группой
	work_data.file_offset = size; //смещение
	work_data.string_length = strlen(string_buffer[i]); 
	size += (sizeof(char) * (strlen(string_buffer[i]) + 1));
	work_data.file_size = buf -> st_size;
	strcpy (work_data.creation_date , ctime (&(buf -> st_ctime))); //времена
        strcpy (work_data.last_changes_date , ctime (&(buf -> st_mtime)));	
	switch ((buf -> st_mode) & S_IFMT) //типы получаются умножением на соотовестующие маски
	{
                case S_IFBLK: strcpy (work_data.file_type , "block device\n" ); break;
                case S_IFCHR: strcpy (work_data.file_type , "symbol device\n" ); break;
                case S_IFDIR: strcpy (work_data.file_type , "directory\n" ); break;
                case S_IFIFO: strcpy (work_data.file_type , "FIFO\n" ); break;
                case S_IFLNK: strcpy (work_data.file_type , "symbolik link \n" ); break;
                case S_IFREG: strcpy (work_data.file_type , "regular file\n" ); break;
                case S_IFSOCK: strcpy (work_data.file_type , "socket\n" ); break;
                default: strcpy (work_data.file_type , "unknown\n" ); break;
        }
	char* vic;
	vic = get_permissions((buf) -> st_mode);
	strcpy(work_data.permissions, vic);
	free (user_name);
        free (group_name);
	free (vic);
	return work_data; 		
}
char* get_permissions(mode_t st_mode)
{
	char* permissions = NULL;
	permissions = (char*)malloc(SIZE_OF_PERMISSIONS * sizeof(char));
	mode_t owner, group, other;
	int i = 0;
	owner = st_mode & S_IRWXU; //также получаю права доступа, умножая на соответсвующие маски
        group = st_mode & S_IRWXG;
        other = st_mode & S_IRWXO;
        permissions = (char*) malloc (sizeof (char) * SIZE_OF_PERMISSIONS);
        for (i=0;i < SIZE_OF_PERMISSIONS; i++) permissions[i] = '-';
        permissions [SIZE_OF_PERMISSIONS - 1] = '\0';
        if ((owner & S_IRUSR)>0) permissions [0] = 'r';
        if ((owner & S_IWUSR)>0) permissions[1] = 'w';
        if ((owner & S_IXUSR)>0) permissions[2] = 'x';
        if (group & S_IRGRP) permissions[3] = 'r';
        if (group & S_IWGRP) permissions[4] = 'w';
        if (group & S_IXGRP) permissions[5] = 'x';
        if (other & S_IROTH) permissions[6] = 'r';
        if (other & S_IWOTH) permissions[7] = 'w';
        if (other & S_IXOTH) permissions[8] = 'x';
	//printf("%s\n",permissions);
	return permissions;
}
char* get_path(char* file_name, char* arg)
{
	char* result;
	result = (char*)malloc(sizeof(char)*(strlen(arg) + strlen(file_name) + 2));
	strcpy(result, arg);	
	strcat(result, "/");
	strcat(result, file_name);
	return result;
}





