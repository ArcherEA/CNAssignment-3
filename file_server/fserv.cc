#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "tcp-utils.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <iostream>
#include "tokenize.h"
#include "descriptor.h"


void* do_client (int sd);
int create_file(char* file_name);

pthread_mutex_t lock;

int main (int argc, char** argv)
{
  const int port = 9002;
  const int qlen = 32;

  long int msock, ssock;

  struct sockaddr_in client_addr;
  unsigned int client_addr_len = sizeof(client_addr);

  clear_descriptor();
  initiate_descriptor();

  msock = passivesocket(port,qlen);
  if (msock < 0)
  {
      perror("passivesocket");
      return 1;
  }
  printf("Server up and listening on port %d.\n", port);

  pthread_t tt;
  pthread_attr_t ta;
  pthread_attr_init(&ta);
  pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);


  while (1)
  {
    ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
    if (ssock < 0) {
        if (errno == EINTR) continue;
        perror("accept");
        return 1;
    }

    if (pthread_create(&tt, &ta, (void* (*) (void*))do_client, (void*)ssock) != 0 )
    {
        perror("pthread_create");
        return 1;
    }
}
return 0;
}


void* do_client (int sd)
{
    const int ALEN = 256;
    char req[ALEN];
    const char* ack = "ACK : ";
    const char* ackOK = "OK";
    const char* ackERR = "ERR";
    const char* ackFAIL = "FAIL";
    char ack1[1000];
    int n,fp;
    char* com_tok[129];
    size_t num_tok;

    pid_t x = syscall(__NR_gettid);
    //std::cout<<"Thread id: "<<x;

    char str[10];
    sprintf(str, "%d", x);

    printf("Incoming client...\n");

    // Loop while the client has something to say...
    while ((n = readline(sd,req,ALEN-1)) != recv_nodata)
    {

        num_tok = str_tokenize(req, com_tok, strlen(req));
        //str_tokenize(req, com_tok, strlen(req));

        if (strcmp(com_tok[0],"quit") == 0)
        {
            printf("Received quit, sending EOF.\n");
            shutdown(sd,1);
            close(sd);
            printf("Outgoing client...\n");
            return NULL;
        }
        else if(strcmp(com_tok[0],"FOPEN")==0)
        {
          if(num_tok!=2)
          {
            snprintf(ack1, sizeof ack1,"%s %d Improper FOPEN.\nFOPEN format -> FOPEN filename \n", ackFAIL,errno);
            send(sd,ack1,strlen(ack1),0);
          }
          else
          {
                int aclck;
                aclck=pthread_mutex_trylock(&lock);
                if(aclck==0)
                {
                  int chkfd = check_descriptor(com_tok[1]);
                  if(chkfd==0)
                  {
                    fp = create_file(com_tok[1]);
                    int wd = write_descriptor(getpid(),com_tok[1],fp);
                    if(fp!=-1 && wd!=-1)
                    {
                      snprintf(ack1, sizeof ack1,"%s %d\n", ackOK,fp);
                      send(sd,ack1,strlen(ack1),0);
                    }
                    else
                    {
                      snprintf(ack1, sizeof ack1,"%s %d\n", ackFAIL,errno);
                      send(sd,ack1,strlen(ack1),0);
                    }
                  }
                  else
                  {
                    snprintf(ack1, sizeof ack1,"%s %d\n", ackERR,chkfd);
                    send(sd,ack1,strlen(ack1),0);
                  }
                  pthread_mutex_unlock(&lock);

                }
                else
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackFAIL,errno);
                  send(sd,ack1,strlen(ack1),0);
                }
                //pthread_mutex_unlock(&lock);
            }
        }
        else if(strcmp(com_tok[0],"FSEEK")==0)
        {
          if(num_tok!=3)
          {
            snprintf(ack1, sizeof ack1,"%s %d Improper FSEEK.\nFSEEK format -> FSEEK identifier offset \n", ackFAIL,errno);
            send(sd,ack1,strlen(ack1),0);
          }
          else
          {
                int skrt = lseek(atoi(com_tok[1]), atoi(com_tok[2]), SEEK_CUR);
                if(skrt==-1)
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackERR,ENOENT);
                  send(sd,ack1,strlen(ack1),0);
                }
                else
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackOK,0);
                  send(sd,ack1,strlen(ack1),0);
                }
          }
        }
        else if(strcmp(com_tok[0],"FREAD")==0)
        {
          if(num_tok!=3)
          {
            snprintf(ack1, sizeof ack1,"%s %d Improper FREAD.\nFREAD format -> FREAD identifier length \n", ackFAIL,errno);
            send(sd,ack1,strlen(ack1),0);
          }
          else
          {

                char rdbyts[1000];
                int rdrt = read(atoi(com_tok[1]), rdbyts,atoi(com_tok[2]));
                if(rdrt==-1)
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackERR,-1);
                  send(sd,ack1,strlen(ack1),0);
                }
                else
                {
                  sprintf(ack1,"%s %d %s\n", ackOK,rdrt,rdbyts);
                  send(sd,ack1,strlen(ack1),0);
                }
          }
        }
        else if(strcmp(com_tok[0],"FWRITE")==0)
        {
          if(num_tok!=3)
          {
            snprintf(ack1, sizeof ack1,"%s %d Improper FWRITE.\nFWRITE format -> FWRITE identifier bytes \n", ackFAIL,errno);
            send(sd,ack1,strlen(ack1),0);
          }
          else
          {

                int wtrt = write(atoi(com_tok[1]),com_tok[2],strlen(com_tok[2]));
                if(wtrt==-1)
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackERR,-1);
                  send(sd,ack1,strlen(ack1),0);
                }
                else
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackOK,0);
                  send(sd,ack1,strlen(ack1),0);
                }
            }
        }
        else if(strcmp(com_tok[0],"FCLOSE")==0)
        {
          if(num_tok!=3)
          {
            snprintf(ack1, sizeof ack1,"%s %d Improper FCLOSE.\nFCLOSE format -> FCLOSE identifier \n", ackFAIL,errno);
            send(sd,ack1,strlen(ack1),0);
          }
          else
          {

                //close(atoi(com_tok[1]));
                int ddrt = delete_descriptor(atoi(com_tok[1]));
                if(ddrt==0)
                {
                  close(atoi(com_tok[1]));
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackOK,ddrt);
                  send(sd,ack1,strlen(ack1),0);
                }
                else
                {
                  snprintf(ack1, sizeof ack1,"%s %d\n", ackERR,ddrt);
                  send(sd,ack1,strlen(ack1),0);
                }
          }

        }
        else
        {
          send(sd,ack,strlen(ack),0);
          send(sd,"I hope that's not a valid command for me.",strlen("I hope that's not a valid command for me."),0);
          send(sd,req,strlen(req),0);
          send(sd,"\n",1,0);
        }

    }
    // read 0 bytes = EOF:
    printf("Connection closed by client.\n");
    shutdown(sd,1);
    close(sd);
    printf("Outgoing client...\n");
    return NULL;
}


int create_file(char* file_name)
{
	//This function handles user-program-file-descriptor-table.
	int fp;

	//cooking the file path and name
	char *path;
	path = (char*) malloc(50*sizeof(char));
	strcat(path,"");
	strcat(path,file_name);
	printf("-----> File %s created for client",path);

	//creating the file
	fp = open(path,O_RDWR | O_CREAT,S_IRWXU);

  if(fp!=-1)
  {
    return fp;
  }
  else
  {
    return -1;
  }

}
