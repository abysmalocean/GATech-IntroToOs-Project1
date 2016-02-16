#include <unistd.h>
#include "gfserver.h"
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
#include <string.h>

#define SOCKET_ERROR -1
#define CONNECTION_FAIL -1
#define BINDING_FAIL -1
#define ACCEPT_ERROR -1
#define ERROR_SENDING_MSG -3
#define SUCCESS 0
#define ERROR_UNABLE_TO_OPEN_FILE -1
#define ERROR_NO_MEMORY -5
#define BUFSIZE 4096
#define TRUE 1
#define FALSE 0
/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

/*
typedef struct gfcontext_t
{
	int nClientSocket; 
    struct sockaddr_in ClientAddress;

}gfcontext_t;

typedef struct gfserver_t 
{
  ssize_t (*handler)(gfcontext_t *, char *, void*);
  unsigned int portno; // port to listen on
  int nMaxPending;
  int nServerSocket;
  int nReuseAddr;
  int nClientSocket;
  struct sockaddr_in ServerAddress;
  char* pHandlerArg;  

}gfserver_t;
*/


static char*  gfs_getFilePath(char* pData)
{
	strtok(pData, " "); //Make sure that we get everything before the end of the request
	strtok(NULL, " ");			   //parse out the scheme
	char* pFilePath = strtok(NULL, " ");			   //parse out the scheme
	return pFilePath; 
}

ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len)
{
	char* pHeader = (char*)calloc(1, BUFSIZE);

	if (status == GF_OK)
	{
		strcat(pHeader, "GETFILE OK ");
		char Length[30] = {"\0"};
		sprintf(Length, "%d", (int)file_len);
		strcat(pHeader, Length);
		strcat(pHeader, "\r\n\r\n");   
	}
	else if (status == GF_FILE_NOT_FOUND)
	{
 		strcat(pHeader, "GETFILE FILE_NOT_FOUND ");
		strcat(pHeader, "\r\n\r\n");
	}
	else if (status == GF_ERROR)
	{
 		strcat(pHeader, "GETFILE ERROR ");
		strcat(pHeader, "\r\n\r\n");
	}
	fprintf(stderr, "\nHeader Sent is: %s", pHeader); 

	ctx->nFileLen = file_len; 
	int numBytesSent = send(ctx->nClientSocket, pHeader, file_len, MSG_DONTWAIT);
	free(pHeader);
	return numBytesSent;  
}

ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len)
{
	
	if(ctx->nBytesSent < ctx->nFileLen )
	{
  	  int BytesSent = send(ctx->nClientSocket, data, len, 0); 	
      ctx->nBytesSent += BytesSent;
      fprintf(stderr, "\nTotalBytesSent: %d\tBytesSentThisSend: %d ", ctx->nBytesSent, BytesSent);
      return BytesSent; 
  }
	return  0;  
}

void gfs_abort(gfcontext_t *ctx)
{
	close(ctx->nClientSocket);
	return; 
}

gfserver_t* gfserver_create()
{
  gfserver_t* getFileServer 	= (gfserver_t*) malloc(sizeof(gfserver_t));	
  getFileServer->handlerFunc	= NULL;
  getFileServer->portno 		= -1; 
  getFileServer->nMaxPending 	= -1;
  getFileServer->nServerSocket 	= -1;
  getFileServer->nReuseAddr		= TRUE;
  bzero((char*)&getFileServer->ServerAddress, sizeof(getFileServer->ServerAddress));
  getFileServer->pHandlerArg	= NULL; 

  return getFileServer; 
}

void gfserver_set_port(gfserver_t *gfs, unsigned short port)
{
	gfs->portno = port; 
}
void gfserver_set_maxpending(gfserver_t *gfs, int max_npending)
{
	gfs->nMaxPending = max_npending;  
}

void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void*))
{
	gfs->handlerFunc = handler; 
}

void gfserver_set_handlerarg(gfserver_t *gfs, void* arg)
{
    gfs->pHandlerArg = arg; 
}

void gfserver_serve(gfserver_t *gfs)
{
	char Data[BUFSIZE] = {"\0"}; 
	gfs->nServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(gfs->nServerSocket == SOCKET_ERROR)
    {
        exit(SOCKET_ERROR);
    }

    //Making sure that we are able to connectto our server more than once
    setsockopt(gfs->nServerSocket, SOL_SOCKET, SO_REUSEADDR, &gfs->nReuseAddr, sizeof(int));

    //Zero out or Server address structure for saftey
    bzero((char*)&gfs->ServerAddress, sizeof(gfs->ServerAddress));
    gfs->ServerAddress.sin_addr.s_addr = INADDR_ANY;
    gfs->ServerAddress.sin_port = htons(gfs->portno);
    gfs->ServerAddress.sin_family = AF_INET;

    //Bind to it
    if(bind( gfs->nServerSocket, (struct sockaddr*) &gfs->ServerAddress, sizeof(struct sockaddr_in)) == BINDING_FAIL)
    {
		return;
    }

    //Listen
    listen(gfs->nServerSocket, gfs->nMaxPending);
    gfcontext_t clientConnection;

    int nAddressSize = sizeof(clientConnection.ClientAddress);
    
    int bRunning = TRUE;
    int nBytesRecv = 0;   
    while(bRunning)
    {
        //Accept thte connection
        clientConnection.nClientSocket = accept(gfs->nServerSocket, 
        	                                   (struct sockaddr*)&clientConnection.ClientAddress, 
        	                                   (socklen_t*)&nAddressSize);
        
        if (clientConnection.nClientSocket == ACCEPT_ERROR)
        {
            return; 
        }
        nBytesRecv = recv(clientConnection.nClientSocket, Data, BUFSIZE, 0);
        if(nBytesRecv == -1)
        {
        	return;
        }
 
        char* pFilePath = gfs_getFilePath(Data);
        gfs->handlerFunc(&clientConnection, pFilePath, NULL);  
 
        if(clientConnection.nFileLen == clientConnection.nBytesSent)
        {
        	fprintf(stderr, "\nClosed socket"); 
          clientConnection.nFileLen = 0;  
          clientConnection.nBytesSent = 0; 
        	close(clientConnection.nClientSocket);  
        }
    }
    return;  
}

