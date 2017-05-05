/** Author: Jessica Marie Barre
 *  Name: A client-server
 *  Purpose: A basic client-server program
 * 
 *  Compile with:
 *  gcc -Wall test-connection.c
 *  Run with:
 *  ./a.out server [port] AND ./a.out client [port]
 */

#include<stdio.h>
#include<unistd.h> 
#include<stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdbool.h>
 
int is_server = 0;
int is_client = 0;

#define BUFFER_SIZE 	 1024
#define FILE_LIST		 ".4220_file_list.txt"
#define SERVER_FILE_LIST ".4220_file_list_server.txt"
#define COMMAND_CONTENTS "CONTENTS"
#define COMMAND_PUT 	 "PUT"
#define COMMAND_GET 	 "GET"
#define COMMAND_QUERY	 "QUERY"


/****************** CLIENT OR SERVER FUNCTIONS *****************************/

/** 
 * recv_data uses recv to get data over a connection
 * and checks that the size of the data is exactly
 * as asked for.
 * 
 * @paraam sock the sock fd
 * @param buffer
 * @param size the size to be received
*/
int recv_data( int sock, char * buffer, int size)
{
	int n = recv( sock, buffer, size, 0 );
	
	printf("received %d bytes of data\n", n);

	if ( n != size )
	{
		perror( "recv() data failed" );
		exit(EXIT_FAILURE);
	}
	
	return n;
}

void receive_and_echo( int sock, char * buffer )
{
	int n = recv( sock, buffer, BUFFER_SIZE, 0 );

	if ( n < 0 )
	{
		perror( "recv() failed" );
	}
	else if ( n == 0 )
	{
		printf( "Server disconnected\n" );
		fflush( stdout );
	}
	else
	{
		buffer[n] = '\0';
		printf( "Received [%s]\n", buffer );
		fflush( stdout );
	}
}


/** 
 * send_data uses send() to sends data over a connection
 * and checks that the size of the data is exactly
 * as asked for.
 * 
 * @paraam sock the sock fd
 * @param buffer
 * @param size the size to be sent
*/
int send_data( int sock, char * buffer, int bytes )
{
	int n = send(sock, buffer, bytes, 0 );
	fflush(NULL);

	printf("sent %d out of %d bytes of data\n", n, bytes);

	if ( n != bytes ) {
		perror( "send() failed" );
		exit( EXIT_FAILURE );
	}
	 
	return n;
}

/* start_client is called to start the client */
void start_client(int* sock, unsigned short port) {
    	/* create TCP client socket (endpoint) */
	*sock = socket( PF_INET, SOCK_STREAM, 0 );

	if ( *sock < 0 )
	{
		perror( "socket() failed" );
		exit( EXIT_FAILURE );
	}

	struct hostent * hp = gethostbyname( "localhost" );
	if ( hp == NULL )
	{
		perror( "gethostbyname() failed" );
		exit( EXIT_FAILURE );
	}

	struct sockaddr_in server;
	server.sin_family = PF_INET;
	memcpy( (void *)&server.sin_addr, (void *)hp->h_addr,
			hp->h_length );
	server.sin_port = htons( port );
	
	printf( "Server address is %s\n", inet_ntoa( server.sin_addr ) );
	fflush( stdout );

	if ( connect( *sock, (struct sockaddr *)&server,
			sizeof( server ) ) < 0 )
	{
		perror( "connect() failed" );
		exit( EXIT_FAILURE );
	}
	
	printf( "Connected to the server\n" );
	fflush( stdout );
	
}

void run_client(int sock) {
    
    char buffer[BUFFER_SIZE];
    
    sprintf(buffer, "HI\");
    
    send_data(sock, buffer, strlen(buffer));
    buffer[0] = '\0';
    receive_and_echo(sock, buffer);
   // recv_data(sock, buffer, 2);
    printf("%s\n", buffer);
    
    
}

void start_server(int * sock, unsigned short port) {
    
    /* socket structures */

	struct sockaddr_in server;

	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;


	/* Create the listener socket as TCP socket */

	*sock = socket(PF_INET, SOCK_STREAM, 0);

	if (*sock < 0) {
		perror("ERROR: Could not create socket.");
		exit(EXIT_FAILURE);
	}
	
	server.sin_port = htons(port);
	int len = sizeof(server);

	if (bind(*sock, (struct sockaddr*) &server, len) < 0) {

		perror("Error: bind() failed.");
		exit(EXIT_FAILURE);
	}

	listen(*sock, 1); /* 5 is the max number of waiting clients */

	printf("Started server; listening on port: %d\n", port);
	fflush(stdout);

}

/**
 * revieve_commands is a server function to go through
 * the process of receiving GET, PUT, QUERY, CONTENTS and 
 * request from the server and responding appropriately
 * 
 * @param sock
 * @param dirname
 */
void run_server(int sock, char * dirname) {

	struct sockaddr_in client;
	int fromlen = sizeof( client );
	
	while ( 1 )	{
		
		int newsock = accept( sock, (struct sockaddr *)&client, (socklen_t*)&fromlen );
		printf("Received incoming connection from: %s\n", inet_ntoa( (struct in_addr)client.sin_addr));
	    fflush(stdout);

	    int n;
	    char buffer[BUFFER_SIZE];
	    
	    
	    /* get_file instruction */
	    printf("Receiving an instruction....\n"); // debugging
	    fflush(stdout);

	    do {
	    	
		    n = recv(newsock, buffer, BUFFER_SIZE, 0);
		    fflush(NULL);
		    
		    if (n < 0) {
		    	
		    	perror("recv() failed");
		    	return exit(EXIT_FAILURE);
		    	
		    } else if (n == 0) {
		    	
		    	printf( "SERVER: Rcvd 0 from recv(); closing socket...\n" );
		    	
		    } else { /* n > 0 */
		    
		    	buffer[n] = '\0';
		    	
			    printf( "SERVER: Rcvd message from %s: %s",
			            inet_ntoa( (struct in_addr)client.sin_addr ),
			            buffer );
			    fflush(stdout);

				// do stuff here
				printf("%s\n", buffer);
			    fflush(stdout);
				sprintf(buffer, "YO\n");
				send_data(sock, buffer, strlen(buffer));
				
		    }
	    	
	    } while (n > 0);
	    
	    close( newsock );

	}
}

/************** MAIN *************/

int main( int argc, char **argv) {

	/** Check for correct number of arguments and exit
      if incorrect **/

	if ( argc != 3 ) {
		fprintf(stderr, "Error: Invalid arguments\nUsage: ./a.out <server/client> <port number>>");
		return EXIT_FAILURE;
	}
	
	int sock; 
	
	if (strcasecmp("server", argv[1]) == 0) {
	    
	    is_server = 1;

	    /* Create a temp dir for storage */
	   		
	    start_server(&sock, atoi(argv[2]));
      	
	    run_server(sock, "haha\n");
	  

	} else if (strcasecmp("client", argv[1]) == 0) {
	    
	    is_client = 1; 
	    
	    start_client(&sock, atoi(argv[2]));
	    
	    run_client(sock); 
	   
	} else {
	    perror("Incorrect role input, must be client OR server");
	}
	
	close( sock );
	
	return EXIT_SUCCESS;

}
