#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>  // getaddrinfo()
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>



/* constants */
#define DEBUG    1
#define MAXSIZE    512  // max buffer size
#define CLIENT_PORT_ID  30020

/* Holds command code and argument */
struct command
{
  char arg[255];
  char code[5];
};




/**
 * Create listening socket on remote host
 * Returns -1 on error, socket fd on success
 */
int socket_create (int port);


/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept (int sock_listen);


/**
 * Connect to remote host at given port
 * Returns socket fd on success, -1 on error
 */
int socket_connect (int port, char *host);



/**
 * Receive data on sockfd
 * Returns -1 on error, number of bytes received 
 * on success
 */
int recv_data (int sockfd, char* buf, int bufsize);


/**
 * Send resposne code on sockfd
 * Returns -1 on error, 0 on success
 */
int send_response (int sockfd, int rc);



//------------------- UTILITY FUNCTIONS-------------------//

/**
 * Trim whiteshpace and line ending
 * characters from a string
 */
void trimstr (char *str, int n);



/** 
 * Read input from command line
 */
void read_input (char* buffer, int size);

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply (int sock_control);


/**
 * Print response message
 */
void print_reply (int rc);


/**
 * Do get <filename> command 
 */
int ftclient_get (int data_sock, int sock_control, char* arg);


/**
 * Open data connection to client 
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn (int sock_control);


/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr (int sock_control, int sock_data, char* filename);



#endif







