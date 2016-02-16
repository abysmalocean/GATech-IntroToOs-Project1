#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>

/* Be prepared accept a response of this length */
#define BUFSIZE 4096

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoclient [options]\n"                                                    \
"options:\n"                                                                  \
"  -s                  Server (Default: localhost)\n"                         \
"  -p                  Port (Default: 8888)\n"                                \
"  -m                  Message to send to server (Default: \"Hello World!\"\n"\
"  -h                  Show this help message\n"


#define SOCKET_ERROR -1
#define HOST_UNKNOWN -2
#define CONNECTION_FAIL -1
#define ERROR_SENDING_MSG -3
#define SUCCESS 0
/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8888;
    char *message = "Hello World!";
    char *messageRecv = NULL;
    int nSocket = -1;                           //A handle to my socket
    struct sockaddr_in server_addr;             //A Internet socket address struct to our server
    struct hostent *pServer;                    //Holds host information about our server


    // Parse and set command line arguments
    while ((option_char = getopt(argc, argv, "s:p:m:h")) != -1) {
        switch (option_char) {
            case 's': // server
                hostname = optarg;
                break;
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            case 'm': // server
                message = optarg;
                break;https://docs.google.com/spreadsheets/d/1BojnhDs7hwy6TNiZmE-9ua43_tZx9ejkY3ge1Nl8j10/edit#gid=1868198252
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

    //Create our socket
    nSocket =  socket(AF_INET, SOCK_STREAM, 0);
    if (nSocket == SOCKET_ERROR)
    {
        printf("Failed to create\n");
        exit(SOCKET_ERROR);
    }

    //Set up our server address so we can connect to it
    pServer = gethostbyname(hostname);
    if(pServer == NULL)
    {
        printf("Failed to get host info, are you sure host exist?\n");
        exit(HOST_UNKNOWN);
    }

    bzero((char*) &server_addr, sizeof(server_addr)); //Zero out our server address structure to make sure no

    server_addr.sin_family = AF_INET;
    bcopy((char*)pServer->h_addr,                   //Source
          (char*)&server_addr.sin_addr.s_addr,      //Destination
          pServer->h_length);                       //Size

    server_addr.sin_port = htons(portno);

    //Connect to our server
    if(connect(nSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == CONNECTION_FAIL)
    {
        printf("Failed to connect to the server");
        exit(CONNECTION_FAIL);
    }

    //Send our message
    int numBytesSent = send(nSocket, message, strlen(message), MSG_WAITALL);
    messageRecv = malloc(strlen(message));
    int numBytesRecv = recv(nSocket, messageRecv, strlen(message), MSG_WAITALL);

    printf("%s\n", messageRecv);

    if(numBytesSent != strlen(message))
    {
        printf("Failed to send the message, only sent %d\n", numBytesSent);
        exit(ERROR_SENDING_MSG);
    }
    close(nSocket);
    return(SUCCESS);
}