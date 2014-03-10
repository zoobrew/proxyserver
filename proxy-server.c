  /* message from client */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>


#define BUFFER_SIZE 5120


void createServer(int portnumber, char* blacklist[], int blacklistSize);
int checkRequestMethod(char* method);
char* getHostName(char* oldRequest);
char* filterRequest(char* request);
void handleConnection(int newsock, struct sockaddr_in client, char* blacklist[], int blacklistSize);

char *msg = "Hello world";


int main(int argc, char *argv[])
{

  if ( argc <= 2)
  {
    fprintf(stderr, "use: ./proxy <port number> <blacklisted hosts> \n");
    return -1;
  }

  /* 1st argument is port number */
  int port_number;
  if( sscanf(argv[1], "%d", &port_number) < 1 )
  {
    perror("error parsing port number");
    exit (1);
  }
  char* blacklistedHosts[argc-2];
  int arg = 2;
  for(; arg < argc; arg++)
  {
    blacklistedHosts[arg-2] = argv[arg];
  }
  createServer(port_number, blacklistedHosts, argc-2);

  return 0; /* never executed */

}

void createServer(int portnumber, char *blacklist[], int blacklistSize)
{
  
  int newsock, len, pid;
  unsigned int fromlen;

  /* socket structs */
  struct sockaddr_in client;
  struct sockaddr_in server;
  /* create the socket for the server */
  int socket_handle = socket(AF_INET, SOCK_STREAM, 0);

  if(socket_handle< 0)
  {
    perror("server-socket()");
    exit( 1 );
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(portnumber);
  len = sizeof(server);

  /* bind to the port number given as argument */
  if ( bind( socket_handle, (struct sockaddr*)&server, len) < 0 )
  {
    perror("server-bind()");
    exit(1);
  }
  fromlen = sizeof( client);
  /* asssign the socket as a listener */
  listen(socket_handle, 5);
  while (1)
  {
    newsock = accept( socket_handle, (struct sockaddr *)&client, &fromlen );
    fflush( NULL );
    /* handle socket in child process */
    pid = fork();
    if ( pid == 0 )
    {
      handleConnection(newsock, client, blacklist, blacklistSize);
      close (newsock);
    }
  }
}

char* getHostName(char* oldRequest){
  int strsize = strlen(oldRequest) + 1;
  char* request = (char *)malloc(strsize);
  strcpy(request, oldRequest);
  char* firstLine = strtok(request, "\r\n");;
  printf("%s\n", firstLine);
  char* requestMethod = (char*)malloc(5*sizeof(char));
  sscanf(firstLine, "%s", requestMethod);
  if (checkRequestMethod(requestMethod))
  {
    /* Isolate the hostname from the URI on the Request-Line:*/
    char *hostnameBegin = strstr(firstLine, "://")+3;
    char *hostnameEnd = strstr(hostnameBegin, "/");
    int hostnameLength = hostnameEnd - hostnameBegin;
    char* hostname = (char *)malloc(hostnameLength+1);
    strncpy(hostname,hostnameBegin, hostnameLength);
    hostname[hostnameLength+1] = '\0';
    free(requestMethod);
    free(request);
    return hostname;
  }
  free(requestMethod);
  free(request);
  return NULL;
}

char* filterRequest(char* request)
{
  int strsize = strlen(request) + 1;
  char* requestCopy = (char *)malloc(strsize);
  char* newRequest = (char*)malloc(1);
  int newRequestSize = 3;
  strcpy(requestCopy, request);
  char* line = strtok(requestCopy, "\r\n");
  while ( line != NULL)
  {
    if ( strstr(line, "Accept-Encoding") != NULL)
    {
      line = strtok(NULL, "\r\n");
    } else
    {
      newRequest = realloc(newRequest, (newRequestSize + strlen(line)+2));
      strncpy((newRequest+newRequestSize-3), line, strlen(line));
      strncpy((newRequest+newRequestSize+strlen(line)-3), "\r\n", 2);
      newRequestSize += strlen(line)+2;
      line = strtok(NULL, "\r\n");
    }
  }
  strncpy((newRequest+newRequestSize-3), "\r\n\0", 3);
  return newRequest;
}

int checkRequestMethod(char* method)
{
  if((strstr(method, "GET")) != NULL)
  {
    if((strstr(method, "POST")) != NULL)
    {
      if((strstr(method, "HEAD")) != NULL)
      {
        perror("unsupported request method");
        return 0;
      }
    }
  }
  return 1;
}

void handleConnection(int newsock, struct sockaddr_in client, char* blacklist[], int blacklistSize)
{
  int n,m;
  char* hostname;
  /* message from client */
  char clientBuffer[ BUFFER_SIZE ];
  char serverBuffer[ BUFFER_SIZE ];
  char* filteredRequest;
  /* can also use read() and write() */
  n = recv( newsock, clientBuffer, BUFFER_SIZE - 1, 0 );   // BLOCK
  if ( n < 1 )
  {
    perror( "server-recv()" );
    exit(1);
  }
  else
  {
    clientBuffer[n] = '\0';
    printf("%s: ", inet_ntoa(client.sin_addr));
    hostname = getHostName(clientBuffer);
    if (hostname == NULL)
    {
      perror("HTTP/1.1 405 Method not allowed \r\n\r\n\0");
	  m = write( newsock, "HTTP/1.1 405 Method not allowed \r\n\r\n\0", strlen("HTTP/1.1 405 Method not allowed \r\n\r\n\0")+1 );
	  if ( m < strlen( serverBuffer ) )
	  {
	    perror( "405-write error" );
	    exit( 1 );
	  }
	    exit(1);
    } 
    else
    {
      int blacklistedHost = 0;
      for (; blacklistedHost < blacklistSize; blacklistedHost++)
      {
	char* blacklistEntry = malloc(strlen(blacklist[blacklistedHost]));
	strcpy(blacklistEntry, blacklist[blacklistedHost]);
	if( strtok(blacklistEntry, hostname) == NULL)
        {
	  perror("HTTP/1.1 HTTP 403 Forbidden \r\n\r\n\0");
	  m = write( newsock, "HTTP/1.1 403 Method not allowed \r\n\r\n\0", strlen("HTTP/1.1 403 Method not allowed \r\n\r\n\0")+1 );
	  if ( m < strlen( serverBuffer ) )
	  {
	    perror( "403-write error" );
	    exit( 1 );
	  }
	    exit(1);
        }
        free(blacklistEntry);
      }
      filteredRequest = filterRequest(clientBuffer);
      /* create connection to host */
      struct sockaddr_in host;
      struct hostent *hp;
      int hostsock = socket( PF_INET, SOCK_STREAM, 0 );
      hp = gethostbyname( hostname );
      bcopy( (char *)hp->h_addr, (char *)&host.sin_addr, hp->h_length );

      host.sin_family = AF_INET;
      host.sin_port = htons(80);
      if ( connect( hostsock, (struct sockaddr *)&host, sizeof( host) ) < 0 ) {
        perror( "host-connect()" );
        exit( 1 );
      }
      n = write( hostsock, filteredRequest, strlen(clientBuffer) );
      if ( n < strlen( clientBuffer ) )
      {
        perror( "host-write()" );
        exit( 1 );
      }
      n=1;
      while(n != 0)
      {
        n = read( hostsock, serverBuffer, BUFFER_SIZE-1);   // BLOCK
        if ( n < 0 )
        {
          perror( "host-read()" );
        }
        else
        {
          serverBuffer[n] = '\0'; 
        }
        m = write( newsock, serverBuffer, strnlen( serverBuffer, BUFFER_SIZE-1 ) );
        if ( m < strlen( serverBuffer ) )
        {
          perror( "host-write()" );
          exit( 1 );
        }
      } 
      free(filteredRequest);
      close( hostsock);
      exit(0);
    }
  }
}

