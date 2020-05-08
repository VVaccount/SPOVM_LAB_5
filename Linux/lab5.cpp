#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <aio.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

struct OperationInfo
{
  int hFile;			// Дескриптор файла
  char  buf[100];
  size_t NumberOfBytes;
  size_t NumberOfBytesTransferred;
  off_t pos_in_file;
  off_t pos_out_file;
  struct aiocb aiocbStruct;
} info;

void* ReaderThread(void * );
void* WriterThread(void * );

int (*read_async) (struct OperationInfo *);
int (*write_async) (struct OperationInfo *);

pthread_mutex_t write_completed = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_completed = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
  if(argc!=3)
  {
    printf("arguments error");
    return 0;
  }
  
  void *lib = dlopen("./libLab5.so",RTLD_NOW);
  
  if(lib == NULL)
  {
    printf("Library ERROR");
    return 0;
  }
  
  read_async =(int(*)(struct OperationInfo*)) dlsym(lib,"read_async");
  write_async = (int(*)(struct OperationInfo*))dlsym(lib,"write_async");  
  
  pthread_mutex_lock(&read_completed);
  pthread_mutex_lock(&mutex_exit);
  
  pthread_t hThread_read, hThread_write;
  
  info.aiocbStruct.aio_offset = 0;
  info.aiocbStruct.aio_buf = info.buf;
  info.NumberOfBytes = sizeof(info.buf);
  info.aiocbStruct.aio_sigevent.sigev_notify = SIGEV_NONE;
  info.pos_in_file = 0;
  info.pos_out_file = 0;
  
  pthread_create(&hThread_read,NULL,ReaderThread, (void *)argv[1]);
  pthread_create(&hThread_write,NULL,WriterThread, (void *)argv[2]);
  
  
  pthread_join(hThread_read, NULL);
  pthread_join(hThread_write, NULL);
  
  printf("\nOperation complete...\n\n");
  return 0;
}

void* ReaderThread(void *folder_path)
{
  DIR *folder;
  struct dirent entry;
  struct dirent *result;
  const char* folderPATH = (const char*) folder_path;
  
  folder = opendir(folderPATH);
  if(folder == NULL)
  {
    printf("\nOpenDir ERROR");
    return NULL;
  }
  int hReadFile;
  char ReadFilePATH[256];
  
  int readResult = 0;
  
  while ( readdir_r(folder, &entry, &result) == 0 && result != NULL)
  {
    if( strcmp( entry.d_name, "." ) != 0 && strcmp( entry.d_name, ".." ) != 0 )
      break;
  } 
  if (result == NULL)
  {
    printf("\nFolder is empty");
    return NULL;
  }
  
  while(1)
  {
    pthread_mutex_lock(&write_completed);
 
    if(readResult == 0)
    {
      info.pos_in_file = 0;
      
      strcpy(ReadFilePATH, folderPATH); strcat(ReadFilePATH, "/");
      strcat(ReadFilePATH, entry.d_name);  // Получаем путь к текущему читаемому файлу
      
      hReadFile = open(ReadFilePATH, O_RDONLY);
    }
    info.hFile = hReadFile;
    
    readResult = read_async(&info);
    
//  printf("\nreadResult = %d, file = %s", readResult, entry.d_name);

    if(readResult == 0)
    {    
      printf("\n%s added...", entry.d_name);
      while ( readdir_r(folder, &entry, &result) == 0 && result != NULL)
      {
	if( strcmp( entry.d_name, "." ) != 0 && strcmp( entry.d_name, ".." ) != 0 )
	  break;
      }      
      
      if (result != NULL)
      {
	close(hReadFile);
	pthread_mutex_unlock(&write_completed);
	continue;
      }
      else break;
    }
    pthread_mutex_unlock(&read_completed);
  }
  
  pthread_mutex_unlock(&mutex_exit); // Мьютекс выхода
  pthread_mutex_unlock(&read_completed);
  close(hReadFile);
  return NULL;
}

void* WriterThread(void * OutFilePATH)
{
  const char* OutputFilePATH = (const char*) OutFilePATH;
  int hOutputFile = open(OutputFilePATH, O_WRONLY | O_CREAT | O_APPEND |O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  while(1)
  {   
    pthread_mutex_lock(&read_completed);
    if(pthread_mutex_trylock(&mutex_exit) == 0)
      break;    

    info.hFile = hOutputFile;
    write_async(&info);
    
    pthread_mutex_unlock(&write_completed);
  }
  close(hOutputFile);
  return NULL; 
}