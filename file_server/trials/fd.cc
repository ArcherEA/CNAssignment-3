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

void add_spaces(char *dest, int size, int num_of_spaces) {
    int len = strlen(dest);
    // for the check i still assume dest tto contain a valid '\0' terminated string, so len will be smaller than size
    if( len + num_of_spaces >= size ) {
        num_of_spaces = size - len - 1;
    }
    memset( dest+len, ' ', num_of_spaces );
    dest[len + num_of_spaces] = '\0';
}



int main (int argc, char** argv)
{
  int fp;
  int fp2;

  fp = open("file_table",O_RDWR | O_CREAT,S_IRWXU);
  close(fp);

  fp2=open("file_table", O_RDWR | O_APPEND);


  int pid = getpid();
  printf("PID : %d\n",pid);
  char file_name[12]="kamesh.txt";
  char cfp[100];
  sprintf(cfp,"%d;%s;%d;",pid,file_name,fp);
  printf("Current length of cfp : %d\n",(int)strlen(cfp));
  printf("Spaces to be appended : %d\n",(100-(int)strlen(cfp)));
//
  int count=0;
  if(strlen(cfp)<100)
  {
    for(int x=0;x<=(100-(int)strlen(cfp));x++)
    {
      sprintf(cfp,"%s  ",cfp);
      count+=1;
    }
  }
  //
  add_spaces(cfp,100,(100-(int)strlen(cfp)));
  printf("CFP is -%s-\n",cfp);
  printf("Length of cfp : %d\n",(int)strlen(cfp));
  //printf("Spaces in cfp : %d",count);

  write(fp2,cfp,strlen(cfp));
  printf("<-HERE->\n");

//  char line_buff[50];
  close(fp2);

  char buff[99 ];
  int lineno = 0;

  int fp3=open("file_table", O_RDWR);
  while(read(fp,&buff,99))
  {
    printf("%d th line : %s |\n",lineno,buff);
    lineno+=1;
  }
  close(fp3);

  return 0;
}
