//https://ops.tips/gists/a-minimal-tcp-client-in-c/

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "tcp-client.h"

int main()
{
	char message[24];
	printf("Enter the sever IP address: "); 
	scanf("%s", &address);

	printf("Enter port number: "); 
	scanf("%d", &port);

    if(connectClient(address,port)==-1)
	{
		return -1;
	}

    printf("\n Enter a message: "); 
    scanf("%s", message);
    sendMessage(message);
    
    disconnectClient;
    return 0;
}

int connectClient(char* address, int port)
{
	server_addr.sin_family = AF_INET;

	// sent.
	server_addr.sin_port = htons(port);

	ret = inet_pton(AF_INET, address, &server_addr.sin_addr);
	if (ret != 1) {
		if (ret == -1) {
			perror("inet_pton");
		}
		printf("\nfailed to convert address to binary net address\n");
		return -1;
	}

	conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (conn_fd == -1) {
		perror("socket");
		return -1;
	}

	ret =
	  connect(conn_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret == -1) {
		perror("connect");
		return -1;
	}
return 0;
}

void sendMessage(char* message)
{
    if (send(conn_fd, message, strlen(message), 0) == -1){
                      perror("send");
		      printf("error sending");
		}
}

int disconnectClient()
{
    ret = shutdown(conn_fd, SHUT_RDWR);
	if (ret == -1) {
		perror("shutdown");
		return -1;
	}

	ret = close(conn_fd);
	if (ret == -1) {
		perror("close");
		return -1;
	}
	return 0;
}