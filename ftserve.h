/* ftserve.h
 *
 * Rebecca Sagalyn
 * CS372, Program 1
 * 11/15/13
 *
 * Server side of TCP file transfer implementation, runs with custom client, 
 * ftclient.c. Sends list of files in current directory and files to ftclient. 
 * Requires user login.
 * 
 * Usage: 
 *    ./ftserve PORT#
 */

#ifndef FTSERVE_H
#define FTSERVE_H

#include "common.h"



/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftserve_list(int sock_data, int sock_control);





/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char*user, char*pass);



/** 
 * Log in connected client
 */
int ftserve_login(int sock_control);


/**
 * Wait for command from client and send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char*cmd, char*arg);



/** 
 * Child process handles connection to client
 */
void ftserve_process(int sock_control);


#endif
