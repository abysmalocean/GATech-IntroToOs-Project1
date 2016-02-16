#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h> 
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include "gfclient.h"
#include <errno.h>

#define SOCKET_ERROR -1
#define CONNECTION_FAIL -1
#define BINDING_FAIL -1
#define ACCEPT_ERROR -1
#define ERROR_SENDING_MSG -3
#define SUCCESS          0
#define ERROR            -5
#define ERROR_INVALID_RESPONSE -6
#define FALSE            0
#define TRUE             1
#define ERROR_UNABLE_TO_OPEN_FILE -1
#define HOST_UNKNOWN    -4
#define ERROR_NO_MEMORY -5
#define BUFFSIZE 4096


/*struct for a getfile request*/
/*
typedef struct gfcrequest_t
{
  gfstatus_t gfStatus; 
  void (*headerfunc)(void*, size_t, void *);
  void (*writefunc)(void*, size_t, void *);
  FILE* pFile;  

  char* pFilePath;  
  char* pHeaderResponse;  
  char* pData; 
  char* pHeaderRequest; 

  char* pHostName;
  struct sockaddr_in server_addr;            
  struct hostent *pServer;
  int nSocket;
  int nBytesReceived;
  int nFileLen;  
  unsigned short portno;
}gfcrequest_t;*/

static gfcrequest_t* pFileTransReq;  

static int gfc_create_socket(gfcrequest_t* gfr)
{
    gfr->nSocket =  socket(AF_INET, SOCK_STREAM, 0);
    return gfr->nSocket;  
}

static int gfc_connect_socket(gfcrequest_t* gfr)
{
  return connect(gfr->nSocket, (struct sockaddr*)&gfr->server_addr, sizeof(gfr->server_addr));
}

static char* gfc_create_request(gfcrequest_t* gfr)
{
  char* pSchemeAndMethod = "GETFILE GET ";
  int nLenOfScheme = strlen(pSchemeAndMethod);
  int nEndOfRequest = strlen(" \r\n\r\n");
  int nLenOfRequest =  nLenOfScheme + strlen(gfr->pFilePath) + nEndOfRequest +1;
  char* pHeaderRequest = (char*) calloc(nLenOfRequest, 1); 
  strcpy(pHeaderRequest, pSchemeAndMethod);
  strcat(pHeaderRequest, gfr->pFilePath);
  strcat(pHeaderRequest, " \r\n\r\n\0"); 
  return pHeaderRequest;  
}

static void gfc_parse_header(gfcrequest_t* gfr)
{                      
  gfr->pHeaderResponse = strdup(strtok(gfr->pData, "\r\n\r\n"));
  char *tmp = strtok(NULL, "\r\n\r\n");                    //Seperate our header from our data
  if(tmp) 
  {
    memmove(gfr->pData, tmp, strlen(tmp) + 1);
  }
  fprintf(stderr, "\npData is: %s", gfr->pData); 
  
  strtok(gfr->pHeaderResponse, " ");                       //Clear the scheme out of our header
  char* pStatus = strtok(NULL, " ");            //Clear the status out of our header    
  if(strcmp(pStatus, "FILE_NOT_FOUND") == 0)    //Set gfr status accordingly
  {
    gfr->gfStatus = GF_FILE_NOT_FOUND; 
  }
  else if(strcmp(pStatus, "ERROR") == 0)
  {
    gfr->gfStatus = GF_ERROR; 
  }
  else if(strcmp(pStatus, "OK") == 0)
  {
    gfr->gfStatus = GF_OK;  
    gfr->nFileLen = atoi(strtok(NULL, "\r\n\r\n"));   //Get the file len    
  }
}

gfcrequest_t* gfc_create()
{
   gfcrequest_t* getFileRequest = (gfcrequest_t*) malloc(sizeof(gfcrequest_t));
  
   getFileRequest->gfStatus                    = GF_INVALID; 
   getFileRequest->pFilePath                   = NULL;  
   getFileRequest->nFileLen                    = -1;  
   getFileRequest->nBytesReceived              = 0; 
    
   getFileRequest->pHeaderResponse             = NULL;  
   getFileRequest->pData                       = (char*) calloc(BUFFSIZE, 1); 
   getFileRequest->pHeaderRequest              = NULL; 

   getFileRequest->pHostName                   = "localhost";
   bzero((char*) &getFileRequest->server_addr, 
         sizeof(getFileRequest->server_addr));    //A internet socket address struct to our server
   getFileRequest->pServer                     = NULL;
   getFileRequest->nSocket                     = -1;
   getFileRequest->portno                      = -1;
   return getFileRequest; 
}

void gfc_set_server(gfcrequest_t *gfr, char* server)
{
    //Set up our server address so we can connect to it
    gfr->pHostName = server; 
    gfr->pServer = gethostbyname(gfr->pHostName);
    if(gfr->pServer == NULL)
    {
      fprintf( stderr, "Server not set\n");
        return;
    }

    bzero((char*) &gfr->server_addr, sizeof(gfr->server_addr));  //Zero out server struct 
    gfr->server_addr.sin_family = AF_INET;                      //Set to TCP socket
    bcopy((char*)gfr->pServer->h_addr,                          //Source
          (char*)&gfr->server_addr.sin_addr.s_addr,             //Destination
          gfr->pServer->h_length);                              //Size
    gfr->server_addr.sin_port = htons(gfr->portno);             //Set Port number
}

void gfc_set_path(gfcrequest_t *gfr, char* path)
{
  gfr->pFilePath = path;  
}

void gfc_set_port(gfcrequest_t *gfr, unsigned short port)
{
  gfr->portno = port;

  //This is only being called as a safety measure it probably dosent need to be 
  //called here if we assume we are going to call gfc_set_server after this but 
  //before we try to connect
  gfr->server_addr.sin_port = htons(gfr->portno);     //Set Port number 
}
//Re check this tomrrow
void gfc_set_headerfunc(gfcrequest_t *gfr, void (*headerfunc)(void*, size_t, void *))
{
  gfr->headerfunc = headerfunc;  
}

void gfc_set_headerarg(gfcrequest_t *gfr, void *headerarg)
{
  gfr->pHeaderResponse = headerarg; 
}

void gfc_set_writefunc(gfcrequest_t *gfr, void (*writefunc)(void*, size_t, void *))
{
  gfr->writefunc = writefunc; 
}

void gfc_set_writearg(gfcrequest_t *gfr, void *writearg)
{
  gfr->pFile = (FILE*) writearg;  
}

int gfc_perform(gfcrequest_t *gfr)
{
  //STEP 1: We first need to connect to our server and send a request
  gfc_create_socket(gfr);                       
  if(gfr->nSocket == SOCKET_ERROR)
  {
    return SOCKET_ERROR; 
  }
  gfc_connect_socket(gfr);                      
  if(gfr->gfStatus == HOST_UNKNOWN)
  {
    close(gfr->nSocket);
    return HOST_UNKNOWN;
  }  
  char* pRequest =  gfc_create_request(gfr);    //Create a get file request
  int numBytesSent = send(gfr->nSocket, pRequest, strlen(pRequest), MSG_WAITALL);  //Send a request 
  if(numBytesSent == -1)                 //Failed to send message
  {
    fprintf(stderr, "\nClosing Scoket after send");
    close(gfr->nSocket); 
    return(-1); 
  }
    
  //STEP 2:  receive data back from the Server 
  int numBytesRecv    = 0;   
  int bRunning        = TRUE;
  while(bRunning == TRUE)
  {      
      numBytesRecv = recv(gfr->nSocket, gfr->pData, 4096, 0);
      if((numBytesRecv == -1) || (numBytesRecv == 0))
      {
          fprintf(stderr, "\rrecv: %s (%d)\n", strerror(errno), errno);
          bRunning = FALSE; 
      }
      else
      {
          if(gfr->pHeaderResponse == NULL)
          {
              gfc_parse_header(gfr); 
              if(gfr->gfStatus == GF_OK)
              {
                  if(gfr->pData != NULL)
                  {
                      gfr->nBytesReceived += numBytesRecv;
                      gfr->writefunc(gfr->pData, numBytesRecv, gfr->pFile);    
                      if(gfr->nBytesReceived == gfr->nFileLen)
                      {
                         fprintf(stderr, "\nExiting as Bytes Received: %d is same as FileLen: %d", gfr->nBytesReceived, gfr->nFileLen);
                         bRunning = FALSE;  
                      }
                  }
              }
              else
              {
                  bRunning = FALSE;
              }
          }  
          else
          {
              gfr->nBytesReceived += numBytesRecv;
              gfr->writefunc(gfr->pData, numBytesRecv, gfr->pFile);    
              fprintf(stderr, "\nIn seconds Bytes Received: %d\tFile Len: %d", gfr->nBytesReceived, gfr->nFileLen);
              if(gfr->nBytesReceived == gfr->nFileLen)
              {
                fprintf(stderr, "\nnBytesReceived: %d\n\tnFileLen: %d\tbRuning is FALSE", 
                        gfr->nBytesReceived, gfr->nFileLen);
                bRunning = FALSE;    
              }
          }
      }
  }
  if((gfr->gfStatus == GF_INVALID) )
  {
     fprintf(stderr, "\nClosing Scoket in invalid");
     close(gfr->nSocket); 
     return ERROR;
  }   
  //close(gfr->nSocket);  
  fprintf(stderr, "\nBytes Received: %d\nFileLen: %d\n", gfr->nBytesReceived,  gfr->nFileLen);  

  //Make sure the server sent over the whole file before sending a success
  if( (gfr->nBytesReceived != gfr->nFileLen) && 
      (gfr->nFileLen != -1))
  {
      fprintf(stderr, "\nClosing Scoket File trans Fail");
      close(gfr->nSocket);
      return ERROR;  
  }
  
  fprintf(stderr, "\nClosing Scoket File trans SUCCESS");
  close(gfr->nSocket);
  return SUCCESS;
}




gfstatus_t gfc_get_status(gfcrequest_t *gfr)
{
    return gfr->gfStatus;  
}

char* gfc_strstatus(gfstatus_t status)
{
  switch(status)
  {
    case GF_OK:
    {
      return "OK";
    }
    case GF_FILE_NOT_FOUND:
    {
      return "FILE_NOT_FOUND";  
    }
    case GF_ERROR:
    {
      return "ERROR"; 
    }
    case GF_INVALID:
    {
      return "ERROR";
    }
    default: 
    {
      return "ERROR";
    }
  }
}

size_t gfc_get_filelen(gfcrequest_t *gfr)
{
    return gfr->nFileLen; 
}

size_t gfc_get_bytesreceived(gfcrequest_t *gfr)
{
    fprintf(stderr, "\nReturning a value of: %d", gfr->nBytesReceived);
    return gfr->nBytesReceived;
}

void gfc_cleanup(gfcrequest_t *gfr)
{
   gfr->gfStatus                    = GF_INVALID; 
   gfr->pFilePath                   = NULL;  

   gfr->pFilePath                   = NULL; 
   if(gfr->pHeaderResponse != NULL)
   {
      free(gfr->pHeaderResponse);
      gfr->pHeaderResponse          = NULL; 
   }  
   gfr->pHeaderResponse             = NULL; 
   free(gfr->pData); 
   gfr->pData                       = NULL; 
   gfr->pHeaderRequest              = NULL; 

   bzero((char*) &gfr->server_addr, sizeof(gfr->server_addr));    //A internet socket address struct to our server
   gfr->pServer                     = NULL;
   gfr->nSocket                     = -1;
   gfr->nFileLen                    = -1;  
   gfr->portno                      = -1;
}

void gfc_global_init()
{
  pFileTransReq = gfc_create(); 
}

void gfc_global_cleanup()
{
  gfc_cleanup(pFileTransReq); 
}
