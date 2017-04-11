#include "ftserve.h"
#include "ftclient.h"

int main(int argc, char *argv[])
{
    int sock_listen, sock_control, port, pid;

    if (argc != 2)
    {
        printf("usage: ./ftserve port\n");
        exit(0);
    }

    port = atoi(argv[1]);

    // create socket
    if ((sock_listen = socket_create(port)) < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    while (1)
    { // wait for client request

        // create new socket for control connection
        if ((sock_control = socket_accept(sock_listen)) < 0)
            break;

        // create child process to do actual file transfer
        if ((pid = fork()) < 0)
        {
            perror("Error forking child process");
        }
        else if (pid == 0)
        {
            close(sock_listen);
            ftserve_process(sock_control);
            close(sock_control);
            exit(0);
        }

        close(sock_control);
    }

    close(sock_listen);

    return 0;
}

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftserve_list(int sock_data, int sock_control)
{
    char data[MAXSIZE];
    size_t num_read;
    FILE* fd;

    int rs = system("ls -l | tail -n+2 > tmp.txt");
    if (rs < 0)
    {
        exit(1);
    }

    fd = fopen("tmp.txt", "r");
    if (!fd)
    {
        exit(1);
    }

    /* Seek to the beginning of the file */
    fseek(fd, SEEK_SET, 0);

    send_response(sock_control, 1); //starting

    memset(data, 0, MAXSIZE);
    while ((num_read = fread(data, 1, MAXSIZE, fd)) > 0)
    {
        if (send(sock_data, data, num_read, 0) < 0)
        {
            perror("err");
        }
        memset(data, 0, MAXSIZE);
    }

    fclose(fd);

    send_response(sock_control, 226); // send 226

    return 0;
}

/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char*user, char*pass)
{
    char username[MAXSIZE];
    char password[MAXSIZE];
    char *pch;
    char buf[MAXSIZE];
    char *line = NULL;
    size_t num_read;
    size_t len = 0;
    FILE* fd;
    int auth = 0;

    fd = fopen(".auth", "r");
    if (fd == NULL)
    {
        perror("file not found");
        exit(1);
    }

    while ((num_read = getline(&line, &len, fd)) != -1)
    {
        memset(buf, 0, MAXSIZE);
        strcpy(buf, line);

        pch = strtok(buf, " ");
        strcpy(username, pch);

        if (pch != NULL)
        {
            pch = strtok(NULL, " ");
            strcpy(password, pch);
        }

        // remove end of line and whitespace
        trimstr(password, (int) strlen(password));

        if ((strcmp(user, username) == 0) && (strcmp(pass, password) == 0))
        {
            auth = 1;
            break;
        }
    }
    free(line);
    fclose(fd);
    return auth;
}

/** 
 * Log in connected client
 */
int ftserve_login(int sock_control)
{
    char buf[MAXSIZE];
    char user[MAXSIZE];
    char pass[MAXSIZE];
    memset(user, 0, MAXSIZE);
    memset(pass, 0, MAXSIZE);
    memset(buf, 0, MAXSIZE);

    // Wait to recieve username
    if ((recv_data(sock_control, buf, sizeof (buf))) == -1)
    {
        perror("recv error\n");
        exit(1);
    }

    int i = 5;
    int n = 0;
    while (buf[i] != 0)
        user[n++] = buf[i++];

    // tell client we're ready for password
    send_response(sock_control, 331);

    // Wait to recieve password
    memset(buf, 0, MAXSIZE);
    if ((recv_data(sock_control, buf, sizeof (buf))) == -1)
    {
        perror("recv error\n");
        exit(1);
    }

    i = 5;
    n = 0;
    while (buf[i] != 0)
    {
        pass[n++] = buf[i++];
    }

    return (ftserve_check_user(user, pass));
}

/**
 * Wait for command from client and
 * send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char*cmd, char*arg)
{
    int rc = 200;
    char buffer[MAXSIZE];

    memset(buffer, 0, MAXSIZE);
    memset(cmd, 0, 5);
    memset(arg, 0, MAXSIZE);

    // Wait to recieve command
    if ((recv_data(sock_control, buffer, sizeof (buffer))) == -1)
    {
        perror("recv error\n");
        return -1;
    }

    //    strncpy(cmd, buffer, 4);
    //    char *tmp = buffer + 5;
    //    strcpy(arg, tmp);


    strcpy(cmd,strtok(buffer, " "));
    strcpy(arg,strtok(NULL, " "));
    printf("in:%s %s\n", cmd, arg);
    if (strcmp(cmd, "QUIT") == 0)
    {
        rc = 221;
    }
    else if ((strcmp(cmd, "USER") == 0) || (strcmp(cmd, "PASS") == 0) ||
            (strcmp(cmd, "SLS") == 0) || (strcmp(cmd, "GET") == 0)
            || (strcmp(cmd, "PUT") == 0) || (strcmp(cmd, "SCD") == 0) || (strcmp(cmd, "SPWD") == 0))
    {
        rc = 200;
    }
    else
    { //invalid command
        rc = 502;
    }

    send_response(sock_control, rc);

    return rc;
}

/** 
 * Child process handles connection to client
 */
void ftserve_process(int sock_control)
{
    int sock_data;
    char cmd[5];
    char arg[MAXSIZE];

    // Send welcome message
    send_response(sock_control, 220);

    // Authenticate user
    if (ftserve_login(sock_control) == 1)
    {
        send_response(sock_control, 230);
    }
    else
    {
        send_response(sock_control, 430);
        exit(0);
    }

    while (1)
    {
        // Wait for command
        //        printf("test");
        fflush(stdout);
        int rc = ftserve_recv_cmd(sock_control, cmd, arg);
        printf("out:%s %s\n", cmd, arg);

        fflush(stdout);

        if ((rc < 0) || (rc == 221))
        {
            break;
        }

        if (rc == 200)
        {
            // Open data connection with client
            if ((sock_data = ftserve_start_data_conn(sock_control)) < 0)
            {
                close(sock_control);
                exit(1);
            }

            // Execute command
            if (strcmp(cmd, "SLS") == 0)
            { // Do list
                ftserve_list(sock_data, sock_control);
            }
            else if (strcmp(cmd, "GET") == 0)
            { // Do get <filename>
                ftserve_retr(sock_control, sock_data, arg);
                printf("Sent\n");
            }
            else if (strcmp(cmd, "PUT") == 0)
            {
                if (read_reply(sock_control) == 550)
                {
                    print_reply(550);
                    close(sock_data);
                    continue;
                }
                ftclient_get(sock_data, sock_control, arg);
                print_reply(read_reply(sock_control));

                // Close data connection
                close(sock_data);
            }
            else if(strcmp(cmd,"SCD")==0)
            {
                chdir(arg);
            }
        }
    }

}
