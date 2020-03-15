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

int write_descriptor(int pid, char file_name[80],int file_desc,int deldes=0)
{
  int fd;

  if(deldes==0)
  {
    fd = open("file_table", O_RDWR | O_APPEND);
  }
  else
  {
    //desc_name="file_table_cache";
    fd=deldes;
  }

  if(fd==-1)
  {
    return -1;
  }
  char cfp[100];
  sprintf(cfp,"%d %s %d ",pid,file_name,file_desc);
  add_trailing_spaces(cfp,100,(100-(int)strlen(cfp)));
  write(fd,cfp,strlen(cfp));
  if(deldes==0)
  {
    close(fd);
  }
  else
  {
    return 0;
  }
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
    //printf("%d th line : By process: %s File:%s FD:%s |\n",lineno,com_tok[0],com_tok[1],com_tok[2]);
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

int delete_descriptor(int file_desc)
{
  //std::cout<<"<______________DELETE DESCRIPTOR OUTPUT_______________________>";
  int source = open("file_table", O_RDONLY);
  int destination = open("file_table_cache",O_WRONLY | O_CREAT,S_IRWXU);

  char buff[99];
  int lineno = 0;
  char* com_tok[129];
  int rec_found=0;
  int orig_del=0;
  int cac_rnm=0;
  while(read(source,&buff,99))
  {
    str_tokenize(buff, com_tok, strlen(buff));
    printf("X-> %d th line : By process: %s File:%s FD:%s |\n",lineno,com_tok[0],com_tok[1],com_tok[2]);
    //std::cout<<"Expected file : "<<file_name<<", Current Desc. :"<<com_tok[1];
    if(atoi(com_tok[2])==file_desc)
    {
      rec_found=1;
    }
    else
    {
      //write(destination,buff,strlen(buff));
      write_descriptor(atoi(com_tok[0]),com_tok[1],atoi(com_tok[2]),destination);
    }
    lineno+=1;
  }

  close(source);
  close(destination);
  //std::cout<<"<______________DELETE DESCRIPTOR OUTPUT_______________________>";
  if(remove("file_table") == 0)
    orig_del=1;
  else
    orig_del=-1;

  if(rename("file_table_cache", "file_table") == 0)
	{
		cac_rnm=1;
	}
	else
	{
		cac_rnm=-1;
	}

  if(rec_found==1 && orig_del==1 && cac_rnm==1)
  {
    return 0;
  }
  else
  {
    return -1;
  }



}

int clear_descriptor()
{

    if(remove("file_table") == 0)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  return 0;
}
