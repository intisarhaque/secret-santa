#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h> 


#include <arpa/inet.h>

#define PORT "1234" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void * clientWrite(void *ptr){
    if (ptr==NULL){
        printf("No sockFD to write;");
        return (NULL);
    }
    int sockfd = *((int*)ptr);
    //printf("SockFD to write is %d\n", sockfd );
    
    char writebuf[MAXDATASIZE];
    int test;

    while (true) {
        printf("What do you want to do?\n");
        fgets(writebuf, MAXDATASIZE, stdin);
        
        if ((test = send(sockfd, writebuf, MAXDATASIZE-1, 0)) == -1) {
            perror("send");
            exit(1);
        }

    }
    return (NULL);
}

int main(int argc, char *argv[])
{
    //if using threads does it have to be global?
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    pthread_t writerThread;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    /*
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s); */

    freeaddrinfo(servinfo); // all done with this structure

    //makewriterthread
    int *arg = &sockfd;
    pthread_create(&writerThread, NULL, &clientWrite, (void*)arg);
    
    while (true) {
        printf("entering loop...\n");
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        printf("client: received '%s'\n",buf);
        sleep(1);
    }

    close(sockfd);

    return 0;
}