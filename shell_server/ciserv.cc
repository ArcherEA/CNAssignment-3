/*
 * Simple iterative client which repeatedly receives lines from
 * clients and return them prefixed by the string "ACK: ".  Release
 * the connection with a client upon receipt of the string "quit"
 * (case sensitive) or upon a connection close by the client.  This
 * variant simulates concurrency in a single thread of execution, so
 * that more clients can be served simultaneously.
 *
 * By Stefan Bruda
 */

#include <stdio.h>
#include "tcp-utils.h"
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//#include "tokenize.h"
#include "tcp-utils.h" 

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <array>

const char* path[] = {"/bin","/usr/bin",0};
const char* prompt = "sshell> ";           
const char* config = "shconfig";

/* Function Declarations */
void zombie_reaper (int signal);
void block_zombie_reaper (int signal);
int run_it (const char* command, char* const argv[], char* const envp[], const char** path);
void do_more(const char* filename, const size_t hsize, const size_t vsize);
size_t str_tokenize(char* str, char** tokens);
void put_cmnd_output(int* filedes_sout,int* filedes_serr);
void print_err_msg(char* command_val, char* content, int msg_len);
/* End of Function Declaration */


// Maximum number of simultaneous connections:
const int max_clients = 100;

// Poll structures for the opened sockets (also include the socket
// descriptor in the fd field):
struct pollfd socks[max_clients+1];  // master socket is socks[0], 
                                     // rest is slave sockets

// Address structures for the opened clients:
struct sockaddr_in* client_addr[max_clients+1];

// The sizes of the addresses above (needed for the call to accept):
unsigned int* client_addr_len[max_clients+1];

// Number of open sockets:
int open_socks = 1;

/*
 * Removes client which from the list of active clients and releases
 * the resources associated with this client. After this, client
 * numbers may change.
 */
void delete_socket (int which) {
    if (which < open_socks) {
        // Release resources:
        close(socks[which].fd);
        delete client_addr[which];
        delete client_addr_len[which];
        
        // Shift the remaining open sockets:
        for (int i = which+1; i < open_socks; i++) {
            socks[i-1] = socks[i];
            client_addr[i-1] = client_addr[i];
            client_addr_len[i-1] = client_addr_len[i];
        }
        
        open_socks--;
        printf("Outgoing client %d removed.\n", which);
    }
}

/*
 * Accepts a new connection and allocates resources for the incoming
 * client.
 */
void accept_new() {
    if (open_socks < max_clients) { // we have room for another client...
        // Alloocate resources:
        client_addr[open_socks] = new sockaddr_in;
        client_addr_len[open_socks] = new unsigned int;
        // Accept call:
        socks[open_socks].events = POLLIN;  // we will monitor the socket
        socks[open_socks].fd = accept(socks[0].fd, 
                                      (struct sockaddr*)(client_addr[open_socks]), 
                                      client_addr_len[open_socks]);
        printf("Incoming client, assigned number %d.\n", open_socks);
        if (socks[open_socks].fd < 0) {
            perror("accept");
            delete_socket(open_socks);
            // we do not return from the program though (clients still to be served).
        }
        else
            open_socks++;
    }
    // If there is no room for the client, we do nothing (the incoming
    // client will be placed in the queue, maybe it will be lucky next
    // time).
}

/*
 * Receives a request for client i and responds to it.  If the
 * received request is an end of file or "quit", terminates the
 * connection to the client.  Note that an empty request also
 * terminates the connection.
 */
void do_client (int which, char** envp) {
    const int ALEN = 129;
    char req[ALEN];
    //const char* ack = "ACK: ";
    int n;
    
    // Remember, we now deal with one request at a time.
    if ((n = readline(socks[which].fd,req,ALEN-1)) != recv_nodata) 
    {
        printf("Client %d: received \"%s\" of length %ld.\n ", which, req,strlen(req));
	printf("!!%s!! \n",req);
        
        if (strcmp(req,"quit") == 0) 
	{
            printf("Client %d sent quit, sending EOF.\n", which);
            shutdown(socks[which].fd,1);
            delete_socket(which);
        }



 		    char command[129];
		    command[128] = '\0';

		    int stringLength = strlen(req); //finds length of the array
		    for(int i = 0; i <= (stringLength); i++) 
		    {
			command[i] = req[i]; //copies UserInput in reverse to TempInput
    		    }


		    std::array<char, 128> buffer;
		    std::string result;

		    //std::cout << "Opening reading pipe" << std::endl;
		    FILE* pipe = popen(command, "r");
		    if (!pipe)
		    {
			std::cerr << "Couldn't start command." << std::endl;
			
		    }


		    while (fgets(buffer.data(), 128, pipe) != NULL) {
		        //std::cout << "Reading..." << std::endl;
		        result += buffer.data();
		    }
		    //auto returnCode =pclose(pipe);
		    pclose(pipe);
	
		    std::cout << result << std::endl;
		    //std::cout << returnCode << std::endl;

	char done_rep[129];
	done_rep[128] = '\0';
	strcat(done_rep,command);
	strcat(done_rep," Command Done! \n");

        send(socks[which].fd,result.c_str(),strlen(result.c_str()),0);
	send(socks[which].fd,done_rep,strlen(done_rep),0);
        //send(socks[which].fd,req,strlen(req),0);
	//send(socks[which].fd,"\n <content here> \n",strlen("\n <content here> \n"),0);	
        send(socks[which].fd,"\n",1,0);
    }
    else if (n == recv_nodata) {
        // read 0 bytes = EOF = done
        printf("Connection closed by client %d.\n", which);
        delete_socket(which);
    }
}

int main (int argc, char** argv, char** envp) {
    const int port = 9002;  // port to listen to
    const int qlen = 32;    // queue length for the master socket
    const int timeout = 50; // timeout for polling connections in
                            // miliseconds; if increased, then the
                            // server responses are slower, but it also
                            // uses less CPU time

    // Set up master socket:
    socks[0].fd = passivesocket(port,qlen);
    if (socks[0].fd < 0) {
        perror("passivesocket");
        return 1;
    }
    socks[0].events = POLLIN;  // we do monitor the master socket.
    
    printf("Server up and listening on port %d,\n", port);
    
    while (1) {
        // Wait from incoming data:
        int poll_result = poll(socks,open_socks,timeout);
        if (poll_result < 0) {
            perror("poll");
            return 1;
        }
        
        // Some data arrived; poll clients and find the first that has
        // something to say:
        int i = 1;
        while (i < open_socks && socks[i].revents != POLLIN)
            i++;
        
        if ( i < open_socks ) { // we have a client request...
            do_client(i, envp);
        }
        else if (! (socks[0].revents != POLLIN) ) { // master socket sayeth...
            accept_new();
            // note that the master socket is the last to be polled (why?)
        }
    }
    return 0;  // will never reach this anyway...
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

size_t str_tokenize(char* str, char** tokens) {
  size_t tok_size = 1;
  tokens[0] = str;
  const size_t n = strlen(str);
  size_t i = 0;
  while (i < n) {
    if (str[i] == ' ') {
      str[i] = '\0';
      i++;
      for (; i < n && str[i] == ' '; i++) 
        /* NOP */;
      if (i < n) {
        tokens[tok_size] = str + i;
        tok_size++;
      }
    }
    else 
      i++;
  }

  return tok_size;
}

void put_cmnd_output(int* filedes_sout,int* filedes_serr)
{



			close(filedes_sout[1]);
		
			char* buffer=(char*)malloc(4096);
			while (1) 
			{
	  			ssize_t count = read(filedes_sout[0], buffer, 4096);
	  			if (count == -1) 
				{
		  			if (errno == EINTR) 
					{
			      			continue;
		    			} 
					else 
					{
			      			perror("read");
			      			exit(1);
		    			}
	  			} 
				else if (count == 0) 
				{
		    			break;
	  			} 
				else 
				{
					//print_err_msg("\nCommand Output  : \n",buffer,4096);
					//write(2,"\n",1);
					printf("Command output:\n%s\n", buffer);
					buffer=(char*)malloc(0);
	  			}
			}
			close(filedes_sout[0]);
			wait(0);


			close(filedes_serr[1]);
		
			char* err_buffer=(char*)malloc(4096);
			while (1) 
			{
	  			ssize_t err_count = read(filedes_serr[0], err_buffer, 4096);
	  			if (err_count == -1) 
				{
		  			if (errno == EINTR) 
					{
			      			continue;
		    			} 
					else 
					{
			      			perror("read");
			      			exit(1);
		    			}
	  			} 
				else if (err_count == 0) 
				{
		    			break;
	  			} 
				else 
				{
					//print_err_msg("\nCommand Execution Error : \n",err_buffer,4096);
					printf("Command Error:\n%s\n", err_buffer);
					errno=0;
					err_buffer=(char*)malloc(0);
	  			}
			}
			close(filedes_serr[0]);
			wait(0);
		

}

void print_err_msg(char* command_val, char* content, int msg_len)
{
	char* err_msg = (char*)malloc(msg_len);
	strcat(err_msg,command_val);
	strcat(err_msg,content);
	write(2,err_msg,msg_len);
	err_msg = (char*)malloc(0);

}
	
