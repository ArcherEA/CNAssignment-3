#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "tokenize.h"
#include "tcp-utils.h"

const char* path[] = {"/bin","/usr/bin",0};
const char* prompt = "sshell> ";
const char* config = "shconfig";

/* Function Declarations */
void zombie_reaper (int signal);
void block_zombie_reaper (int signal);
int run_it (const char* command, char* const argv[], char* const envp[], const char** path);
void do_more(const char* filename, const size_t hsize, const size_t vsize);
void closePort(int sd);
int connectToRemote();
void sendData(int sd, char command[129]);
int receiveData(int sd, char** argv, char ans[126], int ALEN);
int bgServer(char command[129], char** argv, int keepalive);
/* End of Function Declaration */

int sd;

int main (int argc, char** argv, char** envp)
{
	size_t hsize = 0, vsize = 0, rhost = 0, rport = 0;
	char *hostname;

	char init_command[129];
	init_command[128] = '\0';
	char command[129];
	command[128] = '\0';
	char* com_tok[129];
	size_t num_tok;
	int keepalive =0; //sarthak

	printf("Simple shell v1.0.\n");

	int confd = open(config, O_RDONLY);
	if (confd < 0)
	{
		perror("config");
		fprintf(stderr, "config: cannot open the configuration file.\n");
        	fprintf(stderr, "config: will now attempt to create one.\n");
		confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		if (confd < 0) while (hsize == 0 || vsize == 0 || rhost == 0 || rport == 0)
		{
			perror(config);
			fprintf(stderr, "config: giving up...\n");
			return 1; //If here program terminates.
		}
		close(confd);

		confd = open(config, O_RDONLY);
		if (confd < 0)
		{
			perror(config);
            		fprintf(stderr, "config: giving up...\n");
            		return 1;
		}
	}
	while (hsize == 0 || vsize == 0 || rhost == 0 || rport == 0)
	{
		int n = readline(confd, command, 128);

		if (n == recv_nodata)
		{
			break;
		}

		if (n < 0)
		{
			sprintf(command, "config: %s", config);
			perror(command);
			break;
		}
		num_tok = str_tokenize(command, com_tok, strlen(command));
		if (strcmp(com_tok[0], "VSIZE") == 0 && atol(com_tok[1]) > 0)
		{
            		vsize = atol(com_tok[1]);
		}
        else if (strcmp(com_tok[0], "HSIZE") == 0 && atol(com_tok[1]) > 0)
		{
            		hsize = atol(com_tok[1]);
		}
		else if (strcmp(com_tok[0], "RHOST") == 0 && strlen(com_tok[1]) > 0)
		{
               //printf("%s in rhost", com_tok[1]);
            		//rhost = atol(com_tok[1]);
            		//rhost =1;
		}
		else if (strcmp(com_tok[0], "RPORT") == 0 && atol(com_tok[1]) > 0)
		{
            		rport = atol(com_tok[1]);
		}
	}
	//printf("\n%d printin host and port", count);
	close(confd);

	if (hsize <= 0)
	{
		hsize = 75;
		confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		write(confd, "HSIZE 75\n", strlen( "HSIZE 75\n"));
		close(confd);
		fprintf(stderr, "%s: cannot obtain a valid horizontal terminal size, will use the default\n",config);
	}

	if (vsize <= 0)
	{
		vsize = 40;
		confd = open(config, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
		write(confd, "VSIZE 40\n", strlen( "VSIZE 40\n"));
		close(confd);
		fprintf(stderr, "%s: cannot obtain a valid vertical terminal size, will use the default\n",config);
	}

	printf("Terminal set to %ux%u.\n", (unsigned int)hsize, (unsigned int)vsize);

	/* Signal Handler */
	signal(SIGCHLD, zombie_reaper);

	while(1)
	{
		printf("%s",prompt);
		fflush(stdin);

		if (fgets(command, 128, stdin) == 0)
		{
			printf("\nBye\n");
			return 0;
		}
        if(*command == '\n')
        {
            continue;
        }

		if((strlen(command) > 0 && command[strlen(command) - 1] == '\n'))
		{
			command[strlen(command) - 1] = '\0';
		}


        int stringLength = strlen(command); //finds length of the array
        int j=0;
        if(command[0] == '&' && command[1] == ' ')
        {
            for(int i = 2 ; i <= (stringLength); i++)
            {
                init_command[j] = command[i]; //copies UserInput in reverse to TempInput
                j++;
            }
        }
        else
        {
            for(int i = 0; i <= (stringLength); i++)
            {
                init_command[i] = command[i]; //copies UserInput in reverse to TempInput
            }
        }

		char** real_com = com_tok;

		num_tok = str_tokenize(command, real_com, strlen(command));
		real_com[num_tok] = 0;

		int bg = 0;
		int lcl_fg =0;//kamesh
		int lcl_bg =0;//kamesh

		int srv_fg =0;//kamesh
		int srv_bg =0;//kamesh
		int close =0;

		if(strcmp(com_tok[0], "!") == 0)//kamesh
		{
            close = 0 ;
			if (strcmp(com_tok[1], "&") == 0)//kamesh
			{
				lcl_bg = 1;
				real_com = com_tok + 2;
				fprintf(stderr,"This is a local BG : %s \n",*real_com);
			}
			else if(strcmp(com_tok[1], "keepalive") == 0)
            {
                keepalive = 1;
                printf("%s sent to server for execution \n",real_com[0]);
                sd = connectbyport("localhost","9000");
                if (sd == err_host)
                {
                    fprintf(stderr, "Cannot find host %s.\n", argv[1]);
                    return 1;
                }
                if (sd < 0)
                {
                    perror("connectbyport");
                    return 1;
                }
            printf("Connected to %s on port %s.\n","localhost","9000");
            }
            else if(strcmp(com_tok[1], "close") == 0)
            {
                close = 1;
            }
			else
			{
				lcl_fg = 1;
				real_com = com_tok + 1;
				fprintf(stderr,"This is a local FG : %s \n",*real_com);
			}

		}
		else//kamesh
		{

			if (strcmp(com_tok[0], "&") == 0)
			{
				srv_bg = 1;
				real_com = com_tok + 1;
				fprintf(stderr,"This is a server BG : %s \n",*real_com);
			}
			else
			{
				srv_fg = 1;
				fprintf(stderr,"This is a server FG : %s \n",*real_com);
			 }

		}

		//Local control
		if(((lcl_fg || lcl_bg) && (!srv_fg || !srv_bg)) && (keepalive == 0 || close == 0 ))//kamesh
		{
			if (strlen(real_com[0]) == 0)
			{
				continue;
			}
			else if (strcmp(real_com[0], "exit") == 0)
			{
				printf("Bye\n");
				return 0;
			}
			else if (strcmp(real_com[0], "more") == 0)
			{
				if (real_com[1] == 0)
				{
					printf("more: too few arguments\n");
				}

				for (size_t i = 1; real_com[i] != 0; i++)
				{
					do_more(real_com[i], hsize, vsize);
				}
			}
			else
			{ //not more, not exit, not "ENTER", this is an external command
				if (lcl_bg)
				{
					int bgp = fork();
					if (bgp == 0)
					{
						int r = run_it(real_com[0], real_com, envp, path);
						printf("& %s done (%d)\n", real_com[0], WEXITSTATUS(r));
						if (r != 0)
						{
							printf("& %s completed with a non-null exit code\n", real_com[0]);
						}
						return 0;
					}
					else
					{
						continue;
					}
				}
				else
				{
					int r = run_it(real_com[0], real_com, envp, path);
					if (r != 0)
					{
						printf("%s completed with a non-null exit code (%d)\n", real_com[0], WEXITSTATUS(r));
					}
				}
			}

		}
		else//kamesh
		{
            if(keepalive==0 && close == 0)
            {	int status = 0;
                if(srv_bg == 1)
                {
                    int bgp = fork();
					if (bgp == 0)
					{
						int r = bgServer(init_command,argv,keepalive);
						printf("& %s sent to server.  (%d)\n", real_com[0], WEXITSTATUS(r));
					}
					else
                    {
                        continue;
                    }
                }

		else
		{
            const int ALEN = 256;
                char ans[ALEN];
                sd = connectToRemote();
                if (sd == err_host)
                {
                    fprintf(stderr, "Cannot find host %s.\n", argv[1]);
                    return 1;
                }
                if (sd < 0)
                {
                    perror("connectbyport");
                    return 1;
                }
                printf("Connected to %s on port %s.\n","localhost","9000");
                sendData(sd, init_command);
                int n;
                while ((n = receiveData(sd,argv, ans, ALEN)) != recv_nodata)
                {
                    if (n == 0)
                    {
                        closePort(sd);
				        printf("Connection closed by %s.\n", argv[1]);
				        return 0;
                    }
                    if (n < 0)
                    {
                        perror("recv_nonblock");
                        closePort(sd);
                        break;
                    }
                    ans[n] = '\0';
                    printf("%s", ans);
                    fflush(stdout);
                }
                closePort(sd);
		}

            }
            else if(keepalive==1 && close ==0){
            	int status = 0;
                if(srv_bg == 1)
                {
                    int bgp = fork();
					if (bgp == 0)
					{
						bgServer(init_command,argv,keepalive);
					}
					else
	{
		continue;
	}
                }
                else
                {
                    const int ALEN = 256;
                    char ans[ALEN];
                    sendData(sd, init_command);
                    int n;
                    while ((n = receiveData(sd,argv, ans, ALEN)) != recv_nodata)
                    {
                        if (n == 0)
                        {
                                closePort(sd);
                                printf("Connection closed by %s.\n", argv[1]);
                                return 0;
                        }
                        if (n < 0)
                        {
                            perror("recv_nonblock");
                            closePort(sd);
                            break;
                        }

                        ans[n] = '\0';
                        printf("%s", ans);
                        fflush(stdout);
                    }
                }

            }
            else if(keepalive==1 && close ==1)
            {
                closePort(sd);
                keepalive =0;
                close =0;
            }
        }
    }
}


int connectToRemote()
{
     int sd = connectbyport("localhost","9000");

     return sd;

}
void sendData(int sd, char command[129])
{
    fflush(stdout);
    send(sd,command,strlen(command),0);
    send(sd,"\n",1,0);
}
int receiveData(int sd, char** argv, char ans[126], int ALEN)
{
    int n;
    n = recv_nonblock(sd,ans,ALEN-1,500);
    return n;
}

void closePort(int sd)
{
    shutdown(sd, SHUT_RDWR);
    close(sd);
}
void zombie_reaper (int signal)
{
	int ignore;
	while (waitpid(-1, &ignore, WNOHANG) >= 0)
	{/* NOP */}
}

void block_zombie_reaper (int signal)
{
    /* NOP */
}

int bgServer(char command[129], char** argv, int keepalive)
{
    signal(SIGCHLD, block_zombie_reaper);

	int childp = fork();
	int status = 0;
	const int ALEN = 256;
                char ans[ALEN];

	if (childp == 0)
	{
        if(keepalive == 0)
        {
            sd = connectToRemote();
        if (sd == err_host)
                {
                    fprintf(stderr, "Cannot find host %s.\n", argv[1]);
                }
                if (sd < 0)
                {
                    perror("connectbyport");
                }
                printf("Connected to %s on port %s.\n","localhost","9000");
        }
            sendData(sd, command);
            int n;
                while ((n = receiveData(sd,argv, ans, ALEN)) != recv_nodata)
                {
                    if (n == 0)
                    {
                        closePort(sd);
				        printf("Connection closed by %s.\n", argv[1]);
                    }
                    if (n < 0)
                    {
                        perror("recv_nonblock");
                        closePort(sd);
                        break;
                    }
                    ans[n] = '\0';
                    printf("%s", ans);
                    fflush(stdout);
                }
                //closePort(sd);


	}
	else
	{
		waitpid(childp, &status, 0);
		signal(SIGCHLD, zombie_reaper);
		return status;
	}

}

int run_it (const char* command, char* const argv[], char* const envp[], const char** path)
{
	signal(SIGCHLD, block_zombie_reaper);

	int childp = fork();
	int status = 0;

	if (childp == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "%s: attempting to run (%s)", __FILE__, command);
		for (int i = 1; argv[i] != 0; i++)
		{
			fprintf(stderr, " (%s)", argv[i]);
		}
		fprintf(stderr, "\n");
#endif
		execve(command, argv, envp);
		for (size_t i = 0; path[i] != 0; i++)
		{
			char* cp = new char [strlen(path[i]) + strlen(command) + 2];
			sprintf(cp, "%s/%s", path[i], command);

#ifdef DEBUG
			fprintf(stderr, "%s: attempting to run (%s)", __FILE__, cp);
			for (int i = 1; argv[i] != 0; i++)
			{
				fprintf(stderr, " (%s)", argv[i]);
			}
			fprintf(stderr, "\n");
#endif

			execve(cp, argv, envp);
			delete [] cp;
		}

			char* message = new char [strlen(command)+10];
        		sprintf(message, "exec %s", command);
        		perror(message);
        		delete [] message;
        		exit(errno);
	}
	else
	{
		waitpid(childp, &status, 0);
		signal(SIGCHLD, zombie_reaper);
		return status;
	}
}


void do_more(const char* filename, const size_t hsize, const size_t vsize)
{
	const size_t maxline = hsize + 256;
    	char* line = new char [maxline + 1];
    	line[maxline] = '\0';

    	// Print some header (useful when we more more than one file)
    	printf("--- more: %s ---\n", filename);

    	int fd = open(filename, O_RDONLY);
    	if (fd < 0)
	{
        	sprintf(line, "more: %s", filename);
        	perror(line);
        	delete [] line;
        	return;
    	}

    	// main loop
    	while(1)
	{
        	for (size_t i = 0; i < vsize; i++)
		{
	            	int n = readline(fd, line, maxline);
		        if (n < 0)
			{
                		if (n != recv_nodata)
				{  // error condition
                    			sprintf(line, "more: %s", filename);
                    			perror(line);
                		}
                		// End of file
                		close(fd);
                		delete [] line;
                		return;
            		}
            		line[hsize] = '\0';  // trim longer lines
            		printf("%s\n", line);
        	}

        	printf(":");
        	fflush(stdout);
        	fgets(line, 10, stdin);
        	if (line[0] != ' ')
		{
            		close(fd);
            		delete [] line;
            		return;
        	}
    	}
    	delete [] line;
}




