    /* server-select.c */

    #include <dns_sd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdlib.h>
    #include <time.h>
    #include <math.h>

    #include <string.h>
    #include <errno.h>
    #include <time.h>
    #include <math.h>

    #include <sys/select.h>      /* <===== */


    #ifdef _WIN32
    #include <process.h>
    typedef	int	pid_t;
    #define	getpid	_getpid
    #define	strcasecmp	_stricmp
    #define snprintf _snprintf
    #else
    #include <sys/time.h>		// For struct timeval
    #include <unistd.h>         // For getopt() and optind
    #include <arpa/inet.h>		// For inet_addr()
    #endif

    #include <netdb.h>
    #include <sys/select.h>
    #include <fcntl.h>
    #include <err.h>

    #define BUFFER_SIZE 1024
    #define MAX_CLIENTS 100      /* <===== */


    /**
     * Creates a connection with a socket. It handles
     * socket(), bind().
     *
     * @name create_socket
     * @param server_addr the server address
     *
     */

    int create_socket(struct sockaddr_in* server) {

    	/* Create the listener socket as TCP socket */

    	int sock = socket(PF_INET, SOCK_STREAM, 0); //use protocol family, internet

    	if (sock < 0) {
    		perror("socket()");
    		exit(EXIT_FAILURE);
    	}


    	bzero(&*server, sizeof(*server)); // causing segault
    	//server->sin_family = PF_INET;
    	server->sin_family = AF_INET;
    	//server->sin_addr.s_addr = INADDR_ANY;
    	server->sin_addr.s_addr = htonl(INADDR_ANY);

    	unsigned short port = 0;
    	server->sin_port = htons(port); //the port is chosen as the first available.

    	int len = sizeof(*server);

    	if (bind(sock, (struct sockaddr*) &*server, len) < 0) {
    		perror("bind()");
    		exit(EXIT_FAILURE);
    	}

    	return sock;
    }

    /** MAIN **/

    int main()
    {

    	/** Guess variables **/

    	/* Socket Structures */
    	struct sockaddr_in server, client, temp;
    	fd_set readfds;
    	int client_sockets[ MAX_CLIENTS ]; /* client socket fd list */
    	int client_socket_index = 0;  /* next free spot */


    	/* Create the listener socket as TCP socket */
    	/*   (use SOCK_DGRAM for UDP)               */
    	int sock = create_socket(&server);


    	socklen_t sz = sizeof(temp);
    	int ret = getsockname(sock, (struct sockaddr*)&temp, &sz);
    	if (ret < 0) {
    		printf("Problem!\n");
    		exit(-1);
    	}

    	unsigned short local_port = ntohs(temp.sin_port); // gets port to register service

    	listen(sock, 5); /* 5 is number of waiting clients */
    	//printf("Listener bound to port %d\n", port);

    	printf("Started server; listening on port: %d\n", local_port);

    	/** Register the Service **/

    	DNSServiceErrorType	error;

    	fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);

    	/** Read from and Write to the Client **/

    	int fromlen = sizeof( client );

    	char buffer[ BUFFER_SIZE ];

    	int i;

    	while ( 1 )
    	{
    #if 1
    		struct timeval timeout;
    		timeout.tv_sec = 3;
    		timeout.tv_usec = 500;  /* 3 AND 500 microseconds */
    #endif

    		FD_ZERO( &readfds );
    		FD_SET( sock, &readfds ); // FD_SET to include listener fd, sock

    		for ( i = 0 ; i < client_socket_index ; i++ )
    		{
    			FD_SET( client_sockets[ i ], &readfds ); //Set FD_SET to include client socket fd
    		}

    #if 0
    		/* BLOCK */
    		int ready = select( FD_SETSIZE, &readfds, NULL, NULL, NULL );
    		/* ready is the number of ready file descriptors */
    		printf( "select() identified %d descriptor(s) with activity\n", ready );
    #endif

    #if 1
    		int ready = select( FD_SETSIZE, &readfds, NULL, NULL, &timeout );
    		if ( ready == 0 ) {
    		//printf( "No activity\n" );
    			continue;
    		}
    #endif

    		if ( FD_ISSET( sock, &readfds ) )
    		{
    			int newsock = accept( sock,
    					(struct sockaddr *)&client,
    					(socklen_t *)&fromlen );
    			/* this accept() call we know will not block */
    			printf( "Accepted client connection\n" );
    			client_sockets[ client_socket_index++ ] = newsock;
    		}

    		for ( i = 0 ; i < client_socket_index ; i++ )
    		{
    			int fd = client_sockets[ i ];

    			if ( FD_ISSET( fd, &readfds ) )
    			{
    				int n, guess, success;

    				/** Recieve user input, check for errors **/
    				n = recv( fd, buffer, BUFFER_SIZE - 1, 0 );

    				if ( n < 0 )
    				{
    					perror( "recv()" );
    				}
    				else if ( n == 0 )
    				{
    					int k;
    					printf( "Client on fd %d closed connection\n", fd );
    					close( fd );
    					/* remove fd from client_sockets[] array: */
    					for ( k = 0 ; k < client_socket_index ; k++ )
    					{
    						if ( fd == client_sockets[ k ] )
    						{
    							/* found it -- copy remaining elements over fd */
    							int m;
    							for ( m = k ; m < client_socket_index - 1 ; m++ )
    							{
    								client_sockets[ m ] = client_sockets[ m + 1 ];
    							}
    							client_socket_index--;
    							break;  /* all done */
    						}
    					}

    				} else {

    					buffer[n] = '\0';

    					printf("Recieved %s",buffer);

    					n = send(fd, buffer, strlen(buffer), 0);

    					/* Divide input into parts and get the guess */


    				}
    			}
    		}
    	}

    	return EXIT_SUCCESS; /* we never get here */
    }
