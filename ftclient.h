/* ftclient.h
 *
 * Rebecca Sagalyn
 * 11/15/13
 *
 * Client side of TCP file transfer implementation, runs with custom server, 
 * ftserve.c. Receives commands from input, and retreives list of files in current 
 * and files. 
   * 
 * Valid commands: 
 *    get <filename>
 *    list
 *    quit
 *    
 * Usage: 
 *    ./ftclient SERVER_HOSTNAME PORT#
 */

#ifndef FTCLIENT_H
#define FTCLIENT_H

#include "common.h"



/**
 * Parse command in cstruct
 */ 
int ftclient_read_command(char* buf, int size, struct command *cstruct);



/**
 * Open data connection
 */
int ftclient_open_conn(int sock_con);


/** 
 * Do list commmand
 */
int ftclient_list(int sock_data, int sock_con);


/**
 * Input: cmd struct with an a code and an arg
 * Concats code + arg into a string and sends to server
 */
int ftclient_send_cmd(struct command *cmd);


/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login();

void local_comms(struct command cmd);
#endif
