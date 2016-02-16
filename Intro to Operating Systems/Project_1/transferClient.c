#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


#define SOCKET_ERROR -1
#define CONNECTION_FAIL -1
#define BINDING_FAIL -1
#define ACCEPT_ERROR -1
#define ERROR_SENDING_MSG -3
#define SUCCESS 0
#define FALSE 0
#define TRUE 1
#define ERROR_UNABLE_TO_OPEN_FILE -1
#define HOST_UNKNOWN -4
#define ERROR_NO_MEMORY -5

#define BUFSIZE 4096
#define USAGE                                                                 \
"usage:\n"                                                                    \
"  transferclient [options]\n"                                                \
"options:\n"                                                                  \
"  -s                  Server (Default: localhost)\n"                         \
"  -p                  Port (Default: 8888)\n"                                \
"  -o                  Output file (Default foo.txt)\n"                       \
"  -h                  Show this help message\n"

/* Main ========================================================= */
int main(int argc, char **argv) {
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8888;
    char *filename = "foo.txt";
    char *pBuffer = NULL;

    struct sockaddr_in server_addr;             //A internet socket address struct to our server
    struct hostent *pServer;
    int nSocket = -1;


    // Parse and set command line arguments
    while ((option_char = getopt(argc, argv, "s:p:o:h")) != -1) {
        switch (option_char) {
            case 's': // server
                hostname = optarg;
                break;
            case 'p': // listen-port
                portno = atoi(optarg);
                break;
            case 'o': // filename
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
    //Create our socket
    nSocket =  socket(AF_INET, SOCK_STREAM, 0);
    if (nSocket == SOCKET_ERROR)
    {
        exit(SOCKET_ERROR);
    }

    //Set up our server address so we can connect to it
    pServer = gethostbyname(hostname);
    if(pServer == NULL)
    {
        exit(HOST_UNKNOWN);
    }

    bzero((char*) &server_addr, sizeof(server_addr)); //Zero out our server address struct
    server_addr.sin_family = AF_INET;
    bcopy((char*)pServer->h_addr,                   //Source
          (char*)&server_addr.sin_addr.s_addr,      //Destination
          pServer->h_length);                       //Size
    server_addr.sin_port = htons(portno);

    //Connect to our server
    if(connect(nSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == CONNECTION_FAIL)
    {
        
        exit(CONNECTION_FAIL);
    }

    //Open a file to write to
    int fileHandle = open(filename, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR );
    if(fileHandle == ERROR_UNABLE_TO_OPEN_FILE)
    {
        exit(ERROR_UNABLE_TO_OPEN_FILE);
    }

    //Allocate memory
    pBuffer = (char*)calloc(BUFSIZE, sizeof(char));
    if(pBuffer == NULL)
    {
        exit(ERROR_NO_MEMORY);
    }
    int numBytesRecv = 0;
    do
    {
        numBytesRecv = recv(nSocket, pBuffer, sizeof(pBuffer), MSG_DONTWAIT);
        if(numBytesRecv != 0)
        {
            if (write(fileHandle, pBuffer, numBytesRecv) == -1)
            {
                printf("Failed to rite to the file\n");
            }
        }
    }while(numBytesRecv != 0);

    free(pBuffer);
    close(fileHandle);
    close(nSocket);
    return(SUCCESS);
}