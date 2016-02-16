#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#if 0
/* 
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif


#define SOCKET_ERROR -1
#define CONNECTION_FAIL -1
#define BINDING_FAIL -1
#define ACCEPT_ERROR -1
#define ERROR_SENDING_MSG -3
#define SUCCESS 0
#define False 0
#define True 1

#define BUFSIZE 4096
#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 8888)\n"                                \
"  -n                  Maximum pending connections\n"                         \
"  -h                  Show this help message\n"

int main(int argc, char **argv)
{
    int option_char;
    int portno = 8888; /* port to listen on */
    int maxnpending = 5;
    int bRunning = True;

    //Socket specific varibles
    int nServerSocket = -1;
    int nReuseAddr = True;
    int nClientSocket = -1;

    struct hostent* pHostInfo;
    struct sockaddr_in ServerAddress;
    struct sockaddr_in ClientAddress;
    char* pBuffer;

    // Parse and set command line arguments
    while ((option_char = getopt(argc, argv, "p:n:h")) != -1)
    {
        switch (option_char)
        {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            case 'n': // server
                maxnpending = atoi(optarg);
                break;
            case 'h': // help
                printf(stdout, "%s", USAGE);
                exit(0);
                break;
            default:
                fprintf(std    struct sockaddr_in server_addr;             //A internet socket address struct to our server
    struct hostent *pServer;
    int nSocket = -1;err, "%s", USAGE);
                exit(1);
        }
    }

    //Create a socket
    nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(nServerSocket == SOCKET_ERROR)
    {
        printf("Failed to create a server for our socket\n");
        exit(SOCKET_ERROR);
    }
    //Making sure that we are able to connectto our server more than once

    setsockopt(nServerSocket, SOL_SOCKET, SO_REUSEADDR, &nReuseAddr, sizeof(int));


    bzero((char*)&ServerAddress, sizeof(ServerAddress));  //Zero out or Server address structure for saftey
    ServerAddress.sin_addr.s_addr = INADDR_ANY;
    ServerAddress.sin_port = htons(portno);
    ServerAddress.sin_family = AF_INET;

    //Bind to it
    if(bind( nServerSocket, (struct sockaddr*) &ServerAddress, sizeof(struct sockaddr_in)) == BINDING_FAIL)
    {
        printf("Failed to bind to the socket\n");
        exit(BINDING_FAIL);
    }

    //Listen
    listen(nServerSocket, maxnpending);
    int nAddressSize = sizeof(ClientAddress);

    while(bRunning)
    {
        //Accept thte connection
        nClientSocket = accept(nServerSocket,(struct sockaddr*)&ClientAddress, (socklen_t*)&nAddressSize);
        if (nClientSocket == ACCEPT_ERROR)
        {
            printf("Failed to accetp a connection\n");
            exit(ACCEPT_ERROR);
        }

        int numBytes = 0;
        pBuffer = (char*)calloc(BUFSIZE, sizeof(char));

        //Receive data
        numBytes = recv(nClientSocket, pBuffer, BUFSIZE, 0);
        //send(nClientSocket, pBuffer, strlen(pBuffer), 0);
        if(strcmp(pBuffer, "exit") == 0)
        {
            printf("Message recevied: %s\n", pBuffer);
            bRunning = False;
            close(nClientSocket);
        }
        else if (numBytes != 0)
        {
            send(nClientSocket, pBuffer, numBytes, MSG_WAITALL);
            memset(pBuffer, 0, BUFSIZE);
            close(nClientSocket);
        }
    }

    /* Socket Code Here */
    close(nServerSocket);
    exit(SUCCESS);
}