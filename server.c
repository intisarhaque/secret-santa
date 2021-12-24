#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

short socketCreate(void){
    short hSocket;
    printf("Create the socket\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    return hSocket;
}

int bindCreatedSocket(int hSocket){
    int iRetval = -1;
    int clientPort = 12345;

    struct sockaddr_in remote = {0};

    //internet address family
    remote.sin_family = AF_INET;

    //any incoming interface
    remote.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote.sin_port = htons(clientPort);//local port

    iRetval = bind(hSocket, (struct sockaddr *) &remote, sizeof(remote));

    return iRetval;

}

int main(){
    int socket_desc = 0, sock=0, clientLen = 0;
    struct sockaddr_in client;
    char client_message[200] = {0};
    char server_message[100] = {0};
    const char *pMessage = "Hello to C Server\n";






}