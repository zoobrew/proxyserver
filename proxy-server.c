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


#define BUFFER_SIZE 1024


void createServer(int portnumber, char* blacklist[]);
int createClient();
int checkRequestMethod(char* method);
char* getHostName(char* oldRequest);
char* forwardRequestToHost(char* host);
int handleConnection(int newsock, struct sockaddr_in client, char* blacklist[]);

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
    blacklistedHosts[arg] = argv[arg];
  }
  /* handle socket in child process */
  int pid = fork();
  if ( pid == 0 )
  {
    //createClient();
    exit(0);
  }
  createServer(port_number, blacklistedHosts);

  return 0; /* never executed */

}

void createServer(int portnumber, char* blacklist[])
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
  printf( "server-Listener socket created and bound to port %d\n", portnumber );
  while (1)
  {
    printf( "server-Blocked on accept()\n" );
    newsock = accept( socket_handle, (struct sockaddr *)&client, &fromlen );
    printf( "server-Accepted client connection\n" );
    fflush( NULL );
    /* handle socket in child process */
    pid = fork();
    if ( pid == 0 )
    {
      handleConnection(newsock, client, blacklist);
    }
    close (newsock);
  }
}

char* getHostName(char* oldRequest){
  char* firstLine = strtok(oldRequest, "\r\n");;
  printf("Firstline is: %s\n", firstLine);
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
    printf("hostname is: %s\n", hostname);
    char* newRequestLine = malloc(strlen(requestMethod)+ 1 + strlen(hostname) + strlen(" HTTP/1.1"));
    strncpy(newRequestLine, requestMethod, strlen(requestMethod));
    strncpy(newRequestLine+strlen(requestMethod), " ", 1);
    strncpy(newRequestLine+strlen(requestMethod)+1, hostname, strlen(hostname));
    strncpy(newRequestLine+strlen(requestMethod)+1+ strlen(hostname), " HTTP/1.1", strlen(" HTTP/1.1"));
    printf("newRequestLine = %s\n", newRequestLine);

    free(newRequestLine);
    return hostname;
  }
  free(requestMethod);
  char* secondLine = strtok(NULL, "\r\n");
  printf("Secondline is: %s", secondLine);
  return NULL;
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

char* forwardRequestToHost(char* host)
{
  return NULL;
}

int handleConnection(int newsock, struct sockaddr_in client, char* blacklist[])
{
  int n;
  char* hostname;
  /* message from client */
  char clientBuffer[ BUFFER_SIZE ];
  char serverBuffer[ BUFFER_SIZE ];
  /* can also use read() and write() */
  n = recv( newsock, clientBuffer, BUFFER_SIZE - 1, 0 );   // BLOCK
  if ( n < 1 )
  {
    perror( "server-recv()" );
  }
  else
  {
    clientBuffer[n] = '\0';
    printf( "server-Received message from %s: %s\n", inet_ntoa((struct in_addr)client.sin_addr), clientBuffer );
    hostname = getHostName(clientBuffer);
    int host = 0;
    for (; host < sizeof(blacklist); host++)
    {
      if( strcmp(blacklist[host], hostname) == 0)
      {
        perror("requested host is blacklisted");
        exit(1);
      }
    }
  }
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
  printf("Connected to Host");
  n = write( hostsock, clientBuffer, strlen( clientBuffer ) );
  if ( n < strlen( clientBuffer ) )
  {
    perror( "host-write()" );
    exit( 1 );
  }
  printf("Sent request to Host");
  n = read( hostsock, serverBuffer, BUFFER_SIZE-1);   // BLOCK
  if ( n < 1 )
  {
    perror( "host-read()" );
    exit( 1 );
  }
  else
  {
    serverBuffer[n] = '\0';
    printf( "host-Received message from server: %s\n", serverBuffer );
  }
  n = write( newsock, serverBuffer, strlen( serverBuffer ) );
  if ( n < strlen( serverBuffer ) )
  {
    perror( "host-write()" );
    exit( 1 );
  }
  
  close( hostsock);
  exit(0);
}
