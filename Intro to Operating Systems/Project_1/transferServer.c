#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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
#define ERROR_UNABLE_TO_OPEN_FILE -1
#define ERROR_NO_MEMORY -5
#define BUFSIZE 4096
#define True 1
#define False 0

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  transferserver [options]\n"                                                \
"options:\n"                                                                  \
"  -p                  Port (Default: 8888)\n"                                \
"  -f                  Filename (Default: bar.txt)\n"                         \
"  -h                  Show this help message\n"

int main(int argc, char **argv) {
    int option_char;
    int portno = 8888; /* port to listen on */
    char *filename = "bar.txt"; /* file to transfer */

    int nMaxPending   = 5;
    int nServerSocket = -1;
    int nReuseAddr    = True;
    int nClientSocket = -1;

    struct sockaddr_in ServerAddress;
    struct sockaddr_in ClientAddress;
    char* pBuffer;
    int bRunning = True;

    // Parse and set command line arguments
    while ((option_char = getopt(argc, argv, "p:f:h")) != -1)
    {
        switch (option_char) {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            case 'f': // listen-port
                filename = optarg;
                break;
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;
            default:
                fprintf(stderr, "%s", USAGE);
                exit(1);
        }
    }
    /* Socket Code Here */

    //Create a socket
    nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(nServerSocket == SOCKET_ERROR)
    {
        exit(SOCKET_ERROR);
    }

    //Making sure that we are able to connectto our server more than once
    setsockopt(nServerSocket, SOL_SOCKET, SO_REUSEADDR, &nReuseAddr, sizeof(int));


    //Zero out or Server address structure for saftey
    bzero((char*)&ServerAddress, sizeof(ServerAddress));
    ServerAddress.sin_addr.s_addr = INADDR_ANY;
    ServerAddress.sin_port = htons(portno);
    ServerAddress.sin_family = AF_INET;

    //Bind to it
    if(bind( nServerSocket, (struct sockaddr*) &ServerAddress, sizeof(struct sockaddr_in)) == BINDING_FAIL)
    {
        exit(BINDING_FAIL);
    }

    //Listen
    listen(nServerSocket, nMaxPending);
    int nAddressSize = sizeof(ClientAddress);

    while(bRunning)
    {
        //Accept thte connection
        nClientSocket = accept(nServerSocket,(struct sockaddr*)&ClientAddress, (socklen_t*)&nAddressSize);
        if (nClientSocket == ACCEPT_ERROR)
        {
            exit(ACCEPT_ERROR);
        }

        //Open a file and get its size
        FILE* pFile = fopen(filename, "r");
        if(pFile == NULL)
        {
            exit(ERROR_UNABLE_TO_OPEN_FILE);
        }
        //fseeK(pFile, 0,  SEEK_END);
        //ftell(pFile);
        //fseek(pFile, 0, SEEK_SET);
        pBuffer = (char*)calloc(BUFSIZE, sizeof(char));
        if (pBuffer == NULL)
        {
            exit(ERROR_NO_MEMORY);
        }

        int nBytesToSend = fread(pBuffer, sizeof(char), BUFSIZE, pFile);
        int nBufferOffset = 0;

        while(nBytesToSend > 0)
        {
            nBufferOffset += send(nClientSocket, pBuffer+nBufferOffset, nBytesToSend, MSG_DONTWAIT);
            if(nBytesToSend == nBufferOffset)
            {
                printf("sending Bytes");
                memset(pBuffer, 0, BUFSIZE);
                nBytesToSend = fread(pBuffer, sizeof(char), BUFSIZE, pFile);
            }
        }
        close(nClientSocket);

    }
    free(pBuffer);
    close(nServerSocket);
    return SUCCESS;
}