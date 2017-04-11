#include "common.h"

/**
 * Create listening socket on remote host
 * Returns -1 on error, socket fd on success
 */
int socket_create(int port)
{
    int sockfd;
    int yes = 1;
    struct sockaddr_in sock_addr;

    // create new socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() error");
        return -1;
    }

    // set local address info
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)
    {
        close(sockfd);
        perror("setsockopt() error");
        return -1;
    }

    // bind
    if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof (sock_addr)) < 0)
    {
        close(sockfd);
        perror("bind() error");
        return -1;
    }

    // begin listening for incoming TCP requests
    if (listen(sockfd, 5) < 0)
    {
        close(sockfd);
        perror("listen() error");
        return -1;
    }
    return sockfd;
}

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply(int sock_control)
{
    int retcode = 0;
    if (recv(sock_control, &retcode, sizeof retcode, 0) < 0)
    {
        perror("client: error reading message from server\n");
        return -1;
    }
    return ntohl(retcode);
}

/**
 * Print response message
 */
void print_reply(int rc)
{
    switch (rc)
    {
    case 220:
        printf("220 Welcome, server ready.\n");
        break;
    case 221:
        printf("221 Goodbye!\n");
        break;
    case 226:
        printf("226 Closing data connection. Requested file action successful.\n");
        break;
    case 550:
        printf("550 Requested action not taken. File unavailable.\n");
        break;
    }

}

/**
 * Do get <filename> command 
 */
int ftclient_get(int data_sock, int sock_control, char* arg)
{
    char data[MAXSIZE];
    int size;
    FILE* fd = fopen(arg, "w");
    int i = 0;
    while ((size = recv(data_sock, data, MAXSIZE, 0)) > 0)
    {
        fwrite(data, 1, size, fd);
        i += size;
        printf("%d\n", i);
    }

    if (size < 0)
    {
        perror("error\n");
    }

    fclose(fd);
    return 0;
}

/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept(int sock_listen)
{
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof (client_addr);

    // Wait for incoming request, store client info in client_addr
    sockfd = accept(sock_listen, (struct sockaddr *) &client_addr, &len);

    if (sockfd < 0)
    {
        perror("accept() error");
        return -1;
    }
    return sockfd;
}

/**
 * Connect to remote host at given port
 * Returns:	socket fd on success, -1 on error
 */
int socket_connect(int port, char*host)
{
    int sockfd;
    struct sockaddr_in dest_addr;

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("error creating socket");
        return -1;
    }

    // create server address
    memset(&dest_addr, 0, sizeof (dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host);

    // Connect on socket
    if (connect(sockfd, (struct sockaddr *) &dest_addr, sizeof (dest_addr)) < 0)
    {
        perror("error connecting to server");
        return -1;
    }
    return sockfd;
}

/**
 * Receive data on sockfd
 * Returns -1 on error, number of bytes received 
 * on success
 */
int recv_data(int sockfd, char* buf, int bufsize)
{
    size_t num_bytes;
    memset(buf, 0, bufsize);
    num_bytes = recv(sockfd, buf, bufsize, 0);
    if (num_bytes < 0)
    {
        return -1;
    }
    return num_bytes;
}

/**
 * Trim whiteshpace and line ending
 * characters from a string
 */
void trimstr(char *str, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        if (isspace(str[i])) str[i] = 0;
        if (str[i] == '\n') str[i] = 0;
    }
}

/**
 * Send resposne code on sockfd
 * Returns -1 on error, 0 on success
 */
int send_response(int sockfd, int rc)
{
    int conv = htonl(rc);
    if (send(sockfd, &conv, sizeof conv, 0) < 0)
    {
        perror("error sending...\n");
        return -1;
    }
    return 0;
}

/** 
 * Read input from command line
 */
void read_input(char* buffer, int size)
{
    char *nl = NULL;
    memset(buffer, 0, size);

    if (fgets(buffer, size, stdin) != NULL)
    {
        nl = strchr(buffer, '\n');
        if (nl) *nl = '\0'; // truncate, ovewriting newline
    }
}

/**
 * Open data connection to client 
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn(int sock_control)
{
    char buf[1024];
    int wait, sock_data;

    // Wait for go-ahead on control conn
    if (recv(sock_control, &wait, sizeof wait, 0) < 0)
    {
        perror("Error while waiting");
        return -1;
    }

    // Get client address
    struct sockaddr_in client_addr;
    socklen_t len = sizeof client_addr;
    getpeername(sock_control, (struct sockaddr*) &client_addr, &len);
    inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof (buf));

    // Initiate data connection with client
    if ((sock_data = socket_connect(CLIENT_PORT_ID, buf)) < 0)
        return -1;

    return sock_data;
}

/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr(int sock_control, int sock_data, char* filename)
{
    FILE* fd = NULL;
    char data[MAXSIZE];
    size_t num_read;

    fd = fopen(filename, "r");

    if (!fd)
    {
        // send error code (550 Requested action not taken)
        send_response(sock_control, 550);

    }
    else
    {
        // send okay (150 File status okay)
        send_response(sock_control, 150);

        do
        {
            num_read = fread(data, 1, MAXSIZE, fd);

            if (num_read < 0)
            {
                printf("error in fread()\n");
            }

            // send block
            if (send(sock_data, data, num_read, 0) < 0)
                perror("error sending file\n");

        } while (num_read > 0);

        // send message: 226: closing conn, file transfer successful
        send_response(sock_control, 226);

        fclose(fd);
    }
}

