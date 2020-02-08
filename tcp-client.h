#ifndef __TCP_CLIENT__
#define __TCP_CLIENT__

int                ret = 0;
int                conn_fd;
int                port = 9000;
char address[24] = "127.0.0.1";

struct sockaddr_in server_addr = { 0 };

int connectClient(char* address, int port);

int disconnectClient();

void sendMessage(char* message);

#endif