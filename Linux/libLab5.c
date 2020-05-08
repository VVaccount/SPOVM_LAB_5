#include <aio.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct OperationInfo
{
  int hFile;			// Дескриптор файла
  char  buf[100];
  size_t NumberOfBytes;
  size_t NumberOfBytesTransferred;
  off_t pos_in_file;
  off_t pos_out_file;
  struct aiocb aiocbStruct;
};


int read_async(struct OperationInfo *info)
{
  int bytesTransferred;
  info->aiocbStruct.aio_offset = info->pos_in_file;
  info->aiocbStruct.aio_fildes = info->hFile;
  info->aiocbStruct.aio_nbytes = info->NumberOfBytes;
  aio_read(&info->aiocbStruct);
  
  while(aio_error(&info->aiocbStruct) == EINPROGRESS);
  info->NumberOfBytesTransferred = aio_return(&info->aiocbStruct);
   
  if(info->NumberOfBytesTransferred) 
    info->pos_in_file = info->pos_in_file + info->NumberOfBytesTransferred;
/*  
  printf("\nbytesTransferred %d", bytesTransferred);
  printf("\nreadResult %d", readResult);
  usleep(1000000);
 */ 
  
  return info->NumberOfBytesTransferred;
}

int write_async(struct OperationInfo *info)
{
  int writeResult;
  info->aiocbStruct.aio_offset = info->pos_out_file;
  info->aiocbStruct.aio_fildes = info->hFile;
  info->aiocbStruct.aio_nbytes = info->NumberOfBytesTransferred;
  aio_write(&info->aiocbStruct);
  
  while((writeResult = aio_error(&info->aiocbStruct)) == EINPROGRESS);
  
  info->pos_out_file = info->pos_out_file + aio_return(&info->aiocbStruct);
  
  return writeResult;
}




/*
 EINPROGRESS - возвращает AIO_ERROR(3)  
 0 - успех 
 EOVERFLOW - конец файла 
 
 
 */