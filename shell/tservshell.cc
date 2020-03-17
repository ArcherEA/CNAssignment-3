/*
 * Simple multithreaded client which repeatedly receives lines from
 * clients and return them prefixed by the string "ACK: ".  Release
 * the connection with a client upon receipt of the string "quit"
 * (case sensitive) or upon a connection close by the client.
 *
 * By Stefan Bruda
 */
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include "tcp-utils.h"
#include "tokenize.h"

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

/*
 * Repeatedly receives requests from the client and responds to them.
 * If the received request is an end of file or "quit", terminates the
 * connection.  Note that an empty request also terminates the
 * connection.  Same as for the purely iterative or the multi-process
 * server.
 */
 
 const char* path[] = {"/bin","/usr/bin",0}; // path, null terminated (static)
 char buffer[4096]; 
 char output[4096];

 char status_output[4096]="ERR EIO no command input! ";
 //char* envp[] = {0}; 
 void block_zombie_reaper (int signal) {
    /* NOP */
}

void zombie_reaper (int signal) {
    int ignore;
    while (waitpid(-1, &ignore, WNOHANG) >= 0)
        /* NOP */;
}
 
 int run_it (const char* command, char* const argv[], char* const envp[], const char** path) {

    // we really want to wait for children so we inhibit the normal
    // handling of SIGCHLD
    signal(SIGCHLD, block_zombie_reaper);
	int link[2];
	char result[4096];
    int status = 0;
	if (pipe(link)==-1)
	die("pipe");
    int childp = fork();
	if (childp == -1)
	die("fork");
    if (childp == 0) { // child does execve
		dup2 (link[1], STDOUT_FILENO);//STDOUT_FILENO
		close(link[0]);
		close(link[1]);
/*#ifdef DEBUG
        fprintf(stderr, "%s: attempting to run (%s)", __FILE__, command);
        for (int i = 1; argv[i] != 0; i++)
            fprintf(stderr, " (%s)", argv[i]);
        fprintf(stderr, "\n");
#endif*/
		
        execve(command, argv, envp);     // attempt to execute with no path prefix...
        for (size_t i = 0; path[i] != 0; i++) { // ... then try the path prefixes
            char* cp = new char [strlen(path[i]) + strlen(command) + 2];
            sprintf(cp, "%s/%s", path[i], command);
#ifdef DEBUG
            fprintf(stderr, "%s: attempting to run (%s)", __FILE__, cp);
            for (int i = 1; argv[i] != 0; i++)
                fprintf(stderr, " (%s)", argv[i]);
            fprintf(stderr, "\n");
#endif
            execve(cp, argv, envp);
            delete [] cp;
        }
        // If we get here then all execve calls failed and errno is set
        char* message = new char [strlen(command)+10];
        sprintf(message, "exec %s", command);
        perror(message);
        delete [] message;
        exit(errno);   // crucial to exit so that the function does not
                       // return twice!				   
    }

    else { // parent just waits for child completion
		close(link[1]);
		int nbytes = read(link[0], result, sizeof(result));
		for (int i=nbytes;i<4096;i++){result[i]='\0';}
		printf("Output: (%.*s)\n", nbytes, result);
		strcpy(buffer,result);
		
        waitpid(childp, &status, 0);
        // we restore the signal handler now that our baby answered
        signal(SIGCHLD, zombie_reaper);
        return status;
    }
}

 
 
void* do_client (int which,char** envp) {
    const int ALEN = 256;
    char req[ALEN];
    const char* ack = "ACK: ";
    int n;
    char* com_tok[ALEN];  // buffer for the tokenized commands
    size_t num_tok;      // number of tokens

	
	char exit_status[10];

	const char* status[3] = {"OK","FAIL","ERR"};
    printf("Incoming client...\n");
     // Loop while the client has something to say...
    while((n = readline(which,req,ALEN-1)) != recv_nodata) {
        printf("Client %d: received \"%s\"\n ", which, req);
        
        if (strcmp(req,"quit") == 0) {
            printf("Received quit, sending EOF.\n");
			shutdown(which,1);
            close(which);
            printf("Outgoing client...\n");
            return NULL;
        }
		else if (strcmp(req,"CPRINTF") == 0) {
            send(which,buffer,strlen(buffer),0);
			send(which,output,strlen(output),0);
        }
		
		else {
				
			if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
				req[strlen(req) - 1] = '\0';
			// Tokenize input:
			char** real_com = com_tok;  // may need to skip the first
										// token, so we use a pointer to
										// access the tokenized req
			num_tok = str_tokenize(req, real_com, strlen(req));
			real_com[num_tok] = 0;      // null termination for execve
			 int bg = 0;
			if (strcmp(com_tok[0], "&") == 0) { // background command coming
	#ifdef DEBUG
				fprintf(stderr, "%s: background command\n", __FILE__);
	#endif
				bg = 1;
				// discard the first token now that we know that it
				// specifies a background process...
				real_com = com_tok + 1;  
			}

			// ASSERT: num_tok > 0

			// Process input:
			if (strlen(real_com[0]) == 0) // no command, luser just pressed return
				{
					sprintf(status_output,"%s EIO no command input! ",status[2]);
					}
			else if(strcmp(real_com[0],"CPRINT")==0){
				strcpy(output,status_output);
				send(which,buffer,strlen(buffer),0);
				send(which,"\n",1,0);
				send(which,output,strlen(output),0);
				send(which,"\n",1,0);
				sprintf(status_output,"%s %s success!",status[0],real_com[0]);
			}		
			 else { // external command
						if (bg) {  // background command, we fork a process that
								   // awaits for its completion
							int bgp = fork();
							if (bgp == 0) { // child executing the command
								int r = run_it(real_com[0], real_com, envp, path);
								printf("& %s done (%d)\n", real_com[0], WEXITSTATUS(r));
								if (r != 0) {
									printf("& %s completed with a non-null exit code\n", real_com[0]);
									sprintf(status_output,"%s %s %s failed!",status[1],exit_status,real_com[0]);
								}
								else{sprintf(status_output,"%s %s %s success!",status[0],exit_status,real_com[0]);}
								return NULL;
							}
							else  // parent goes ahead and accepts the next command
								{continue;}
						}
						else {  // foreground command, we execute it and wait for completion
							int r = run_it(real_com[0], real_com, envp, path);
							sprintf(exit_status,"%d",WEXITSTATUS(r));
							if (r != 0) {
								printf("%s completed with a non-null exit code (%d)\n", real_com[0], WEXITSTATUS(r));
								sprintf(status_output,"%s %s %s failed!",status[1],exit_status,real_com[0]);
							}
							else{
								sprintf(status_output,"%s %s %s success!",status[0],exit_status,real_com[0]);
								}
						}
					}
			}
		strcpy(output,status_output);
        send(which,ack,strlen(ack),0);
        send(which,req,strlen(req),0);
		send(which,"\n",1,0);
		send(which,output,strlen(output),0);
		//printf("buffer:%s\n",buffer);
		//send(socks[which].fd,"\n",1,0);
		//send(socks[which].fd,buffer,strlen(buffer),0);
        send(which,"\n",1,0);
    }
         // read 0 bytes = EOF:
		printf("Connection closed by client.\n");
		shutdown(which,1);
		close(which);
		printf("Outgoing client...\n");
		return NULL;
}
struct do_client_param
{
    long int sd ;
    char** envp;
     
} ;
void* do_client1(void *arg)
{
    do_client_param* arg1 = (do_client_param *)arg;
    do_client(arg1->sd, arg1->envp);
}
int main (int argc, char** argv, char** envp) {
    const int port = 9003;   // port to listen to
    const int qlen = 32;     // incoming connections queue length
    
    // Note that the following are local variable, and thus not shared
    // between threads; especially important for ssock and client_addr.
    
    long int msock, ssock;               // master and slave socket
  
    struct sockaddr_in client_addr; // the address of the client...
    unsigned int client_addr_len = sizeof(client_addr); // ... and its length
    
    msock = passivesocket(port,qlen);
    if (msock < 0) {
        perror("passivesocket");
        return 1;
    }
    printf("Server up and listening on port %d.\n", port);

    // Setting up the thread creation:
    pthread_t tt;
    pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

    while (1) {
        // Accept connection:
        ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
		
        if (ssock < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            return 1;
        }
        struct do_client_param arg={ssock,envp};
        // Create thread instead of fork:
        if ( pthread_create(&tt, &ta, do_client1, &arg) != 0 ) {
            perror("pthread_create");
            return 1;
        }
        // main thread continues with the loop...
    }
    return 0;   // will never reach this anyway...
}
