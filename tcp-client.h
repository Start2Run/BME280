#ifndef __TCP_CLIENT__
#define __TCP_CLIENT__

extern int                ret;
extern int                conn_fd;
extern int                port;
extern char               address[24];

extern struct sockaddr_in server_addr;

int connectClient(char* address, int port);

int disconnectClient();

void sendMessage(char* message);

#endif