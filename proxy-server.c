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


void createServer(int portnumber);
int createClient();
int getRequestHost(char* method);
char* createNewRequest(char* oldRequest);

char *msg = "Hello world";


int main(int argc, char *argv[])
{

  if ( argc <= 1)
  {
    fprintf(stderr, "use: ./proxy <port number> \n");
    return -1;
  }

  /* 1st argument is port number */
  int port_number;
  if( sscanf(argv[1], "%d", &port_number) < 1 )
  {
    perror("error parsing port number");
    exit (1);
  }
    /* handle socket in child process */
    int pid = fork();
    if ( pid == 0 )
    {
      //createClient();
      exit(0);
    }
    createServer(port_number);

  return 0; /* never executed */

}

void createServer(int portnumber){

  int newsock, len, n, pid;
  unsigned int fromlen;

  /* socket structs */
  struct sockaddr_in client;
  struct sockaddr_in server;

  char buffer[ BUFFER_SIZE ];
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
    //fflush( NULL );

    /* handle socket in child process */
    pid = fork();
    if ( pid == 0 )
    {
      /* can also use read() and write() */
      n = recv( newsock, buffer, BUFFER_SIZE - 1, 0 );   // BLOCK
      if ( n < 1 )
      {
        perror( "server-recv()" );
      }
      else
      {
        buffer[n] = '\0';
        printf( "server-Received message from %s: %s\n", inet_ntoa((struct in_addr)client.sin_addr), buffer );
        createNewRequest(buffer);

      }
      n = send( newsock, buffer, BUFFER_SIZE, 0 );
      if ( n < strlen( msg ) )
      {
        perror( "server-Write()" );
      }
      close( newsock);
      exit(0);
    }
    close (newsock);
  }
}


int createClient()
{

  char buffer[ BUFFER_SIZE ];

  int sock, n;
  unsigned short port;
  struct sockaddr_in server;
  struct hostent *hp;

  sock = socket( PF_INET, SOCK_STREAM, 0 );
  if ( sock < 0 ) {
    perror( "client-socket()" );
    exit( 1 );
  }

  server.sin_family = PF_INET;
  hp = gethostbyname( "localhost" );  // localhost
  if ( hp == NULL ) {
    perror( "client-Unknown host" );
    exit( 1 );
  }

  /* could also use memcpy */
  bcopy( (char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length );
  port = 8000;
  server.sin_port = htons( port );

  if ( connect( sock, (struct sockaddr *)&server, sizeof( server) ) < 0 ) {
    perror( "client-connect()" );
    exit( 1 );
  }

  while ( 1 )
  {
    sleep( 5 );

    n = write( sock, msg, strlen( msg ) );
    if ( n < strlen( msg ) )
    {
      perror( "client-write()" );
      exit( 1 );
    }

    n = read( sock, buffer, 1024 );   // BLOCK
    if ( n < 1 )
    {
      perror( "client-read()" );
      exit( 1 );
    }
    else
    {
      buffer[n] = '\0';
      printf( "client-Received message from server: %s\n", buffer );
    }
  }

  close( sock );

  return 0;
}
char* createNewRequest(char* oldRequest){
  char* firstLine = strtok(oldRequest, "\r\n");;
  printf("Firstline is: %s\n", firstLine);
  char* requestMethod = (char*)malloc(5*sizeof(char));
  sscanf(firstLine, "%s", requestMethod);
  if ((getRequestHost(requestMethod)) == 0)
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

int getRequestHost(char* method)
{
  if((strstr(method, "GET")) != NULL)
  {
    if((strstr(method, "POST")) != NULL)
    {
      if((strstr(method, "HEAD")) != NULL)
      {
        perror("unsupported request method");
        return -1;
      }
    }
  }
  return 0;
}




