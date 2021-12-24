#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <time.h> 

#define PORT "1234"   // Port we're listening on
#define CLIENTMAX 10
/*
typedef struct intObj{
    int id;
    struct intObj* next;
}intObj_t;
*/

typedef struct User_t {
    int pfdi;
    char *name;
    struct User_t *whoToGift;
} User_t;

User_t UserArr[CLIENTMAX];

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;//either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;//TCP
    hints.ai_flags = INADDR_ANY;//no specific IP address
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}
void swapStruct(struct User_t *p1, struct User_t *p2){
    User_t temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

void shuffleList(int fd_count){
    int listSize = fd_count-1;
    int listBeginning;
    // /*
    //     fisher-yates shuffle
    //     to shuffle an array userArr of listSize elements (indices 0 to listSize-1)
    // */

    for (listBeginning=1; listBeginning<listSize-1; listBeginning+=1){
        int randNumber = (rand() % listSize) + 1;
        printf("Swapping %d and %d\n", listBeginning, randNumber);
        swapStruct(&UserArr[listBeginning],&UserArr[randNumber]);
        
    }
    for (listBeginning=1; listBeginning<listSize-1; listBeginning+=1){
        UserArr[listBeginning].whoToGift = &UserArr[(listBeginning+1)%listSize];
    }

}
void assignGift(int fd_count){
    int listSize = fd_count-1;
    int listBeginning;
    for (listBeginning=1; listBeginning<=listSize; listBeginning+=1){
        int nextIndex = ((listBeginning)%listSize)+1;
        UserArr[listBeginning].whoToGift = &UserArr[nextIndex];
        printf("%s is gifting to %s\n", UserArr[listBeginning].name, (*(UserArr[listBeginning].whoToGift)).name);
    }
}


// Main
int main(void)
{
    int listener;     // Listening socket descriptor

    int newfd;        // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;

    char buf[256]; // Buffer for client data
    char writebuf[256]; //buffer to send client data
    //uint8_t buf[15];

    time_t t;
    srand((unsigned) time(&t));

    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection

    fd_count = 1; // For the listener

    // Main loop
    while (true) {
        int poll_count = poll(pfds, fd_count, -1);//-1 means infinite timeout

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {

            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN) { // We got one!!

                if (pfds[i].fd == listener) {
                    // If listener is ready to read, handle new connection
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // If not the listener, we're just a regular client
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
                    //check first char of buf for a number. if 1 add name
                    //buf[sizeof(buf)-1]='\0';
                    buf[strcspn(buf, "\n")] = 0;//remove newline 
                    int sender_fd = pfds[i].fd;
                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        // We got some good data from a client check different buffer flags 
                        if (buf[0]=='1'){

                            User_t user = {0};
                            user.name = (char *) malloc(100);
                            memmove(buf, buf+1, strlen(buf));//remove leading num                  
                            strcpy(user.name, buf);
                            user.pfdi=i;
                            user.whoToGift=NULL;
                            UserArr[i] = user;
                            int check;
                            check = snprintf(writebuf, sizeof(writebuf), "%s has been stored in secret santa!", buf);
                            printf("check is %d\n", check);
                            buf[0]='\0';

                            if (send(pfds[i].fd, writebuf, nbytes, 0) == -1) {
                                perror("send");

                            }

                        }
                        if (buf[0]=='2'){
                            if (send(pfds[i].fd, UserArr[i].name, nbytes, 0) == -1){
                                    perror("send2");
                            }
                        }
                        if (buf[0]=='3'){
                            for (int j=1; j<fd_count; j+=1){
                                printf("name is: %s\n", UserArr[j].name );
                            }
                        }
                        if (buf[0]=='4'){
                            shuffleList(fd_count);
                            shuffleList(fd_count);
                        }
                        if (buf[0]=='5'){
                            assignGift(fd_count);
                            int listSize = fd_count-1;
                            int listBeginning;
                            printf("5 in\n");
                            for (listBeginning=1; listBeginning<=listSize; listBeginning+=1){
                                //printf("%s is gifting to %s\n", UserArr[listBeginning].name, (*(UserArr[listBeginning].whoToGift)).name);
                                snprintf(writebuf, sizeof(writebuf), "%s, you are gifting to %s", UserArr[listBeginning].name, (*(UserArr[listBeginning].whoToGift)).name);
                                int pfdids = UserArr[listBeginning].pfdi;
                                send(pfds[pfdids].fd, writebuf, nbytes, 0);
                            }

                        }
                        memset(buf,0,strlen(buf));


                        // for(int j = 0; j < fd_count; j++) {
                        //     // Send to everyone!
                        //     int dest_fd = pfds[j].fd;

                        //     // Except the listener and ourselves
                        //     if (dest_fd != listener && dest_fd != sender_fd) {
                        //         if (send(dest_fd, buf, nbytes, 0) == -1) {
                        //             perror("send");
                        //         }
                        //         puts("SENDING DATA TO CLIENTS!");
                        //     }
                        // }
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}