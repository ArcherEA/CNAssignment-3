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
#include "tokenize.h"

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <array>

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

//Miscellaneous initialization for do_client
char init_command[129];
char* com_tok[129];
size_t num_tok;

const char* path[] = {"/bin","/usr/bin",0};
int run_it (const char* command, char* const argv[], char* const envp[], const char** path);

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
void do_client (int which, char** envp, char** argv) {
    const int ALEN = 256;
    char req[ALEN];
    const char* ack = "ACK: ";
    int n;

    // Remember, we now deal with one request at a time.
    if ((n = readline(socks[which].fd,req,ALEN-1)) != recv_nodata) {
        printf("Client %d: received \"%s\"\n ", which, req);

        if (strcmp(req,"quit") == 0) {
            printf("Client %d sent quit, sending EOF.\n", which);
            shutdown(socks[which].fd,1);
            delete_socket(which);
        }

	char** real_com = com_tok;
	num_tok = str_tokenize(req, com_tok, strlen(req));
        //strcpy(real_com, req);
        run_it(real_com[0], real_com, envp, path);



        send(socks[which].fd,ack,strlen(ack),0);
        send(socks[which].fd,req,strlen(req),0);
        send(socks[which].fd,"\n",1,0);
    }
    else if (n == recv_nodata) {
        // read 0 bytes = EOF = done
        printf("Connection closed by client %d.\n", which);
        delete_socket(which);
    }
}





int run_it (const char* command, char* const argv[], char* const envp[], const char** path) {

    // we really want to wait for children so we inhibit the normal
    // handling of SIGCHLD
    //signal(SIGCHLD, block_zombie_reaper);

    int childp = fork();
    int status = 0;

    if (childp == 0) { // child does execve
#ifdef DEBUG
        fprintf(stderr, "%s: attempting to run (%s)", __FILE__, command);
        for (int i = 1; argv[i] != 0; i++)
            fprintf(stderr, " (%s)", argv[i]);
        fprintf(stderr, "\n");
#endif
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
        waitpid(childp, &status, 0);
        // we restore the signal handler now that our baby answered
       // signal(SIGCHLD, zombie_reaper);
        return status;
    }
}




int main (int argc, char** argv, char** envp) {
    const int port = 9001;  // port to listen to
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
            do_client(i, envp, argv);
        }
        else if (! (socks[0].revents != POLLIN) ) { // master socket sayeth...
            accept_new();
            // note that the master socket is the last to be polled (why?)
        }
    }
    return 0;  // will never reach this anyway...
}
