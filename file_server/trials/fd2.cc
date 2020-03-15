#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include "tcp-utils.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <iostream>
#include "tokenize.h"
#include "tcp-utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void add_trailing_spaces(char *dest, int size, int num_of_spaces)
{
    int len = strlen(dest);

    if( len + num_of_spaces >= size )
    {
        num_of_spaces = size - len - 1;
    }

    memset( dest+len, ' ', num_of_spaces );
    dest[len + num_of_spaces] = '\0';
}

int initiate_descriptor()
{
  int fd = open("file_table",O_RDWR | O_CREAT,S_IRWXU);
  close(fd);
  if(fd!=-1)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

int write_descriptor(int pid, char file_name[80],int file_desc)
{
  int fd = open("file_table", O_RDWR | O_APPEND);
  if(fd==-1)
  {
    return -1;
  }
  char cfp[100];
  sprintf(cfp,"%d %s %d ",pid,file_name,file_desc);
  add_trailing_spaces(cfp,100,(100-(int)strlen(cfp)));
  write(fd,cfp,strlen(cfp));
  close(fd);
  return 0;
}


int read_descriptor()
{
  int fd = open("file_table", O_RDONLY);

  char buff[99 ];
  int lineno = 0;

  while(read(fd,&buff,99))
  {
    printf("%d th line : %s |\n",lineno,buff);
    lineno+=1;
  }

  close(fd);
  return 0;
}

int check_descriptor(char file_name[80])
{
  int fd = open("file_table", O_RDONLY);

  char buff[99];
  int lineno = 0;
  char* com_tok[129];

  while(read(fd,&buff,99))
  {
    str_tokenize(buff, com_tok, strlen(buff));
    printf("%d th line : By process: %s File:%s FD:%s |\n",lineno,com_tok[0],com_tok[1],com_tok[2]);
    if(strcmp(com_tok[1],file_name)==0)
    {
      close(fd);
      return atoi(com_tok[2]);
      break;
    }
    lineno+=1;
  }

  close(fd);
  return 0;
}


int terminate_descriptor(int fd)
{
  close(fd);
  return 0;
}

int main (int argc, char** argv)
{
  initiate_descriptor();
  char file_name[80]="test_file.txt";
  write_descriptor(getpid(),file_name,5);
  printf("The file descriptor : %d\n",check_descriptor(file_name));
  return 0;
}
