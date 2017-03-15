/**
Author: Jessica Marie Barre
Due Date: Sunday, March 19, 11:59:59 PM
Name: TFTP Server
Purpose:

Implement a TFTP server according to RFC 1350.

Your server should:
- be able to support multiple connections at the same time by calling the
  fork() system
- You MUST support the “octet” mode.
- You should not implement the “mail” mode or the “netascii” mode.
- Upon not receiving data for 1 second, your sender should
	- retransmit its last packet.
	- similarly, if you have not heard from the other party for 10 seconds,
	  you should abort the connection.
- Take care to not allow the Sorcerer’s Apprentice Syndrome which should
  have been covered in Short Lecture 11.
- Additionally, we will only be testing files smaller than 32 MB.

- Be sure when testing using the tftp Linux client (for example) to set the
  mode to binary.
- SIGALRM is discussed in our textbook (Chapter 14) which may be useful
  when implementing timeouts.
- Please include a README.txt file which should include your name,
  the name of your partner, and any helpful remarks for the grader.

 */

/* udp-server.c */

/* To test this server, you can use the following
   command-line netcat tool:

   bash$ netcat -u linux00.cs.rpi.edu 41234

   The hostname (e.g., linux00.cs.rpi.edu) cannot be
   localhost; and the port number must match what
   the server reports.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXBUFFER 8192

int main()
{
	int sd;
	struct sockaddr_in server;
	int length;

	/* create the socket (endpoint) on the server side */
	sd = socket( AF_INET, SOCK_DGRAM, 0 );
	/* UDP */

	if ( sd < 0 )   /* this will be part of the file descriptor table! */
	{
		perror( "socket() failed" );
		return EXIT_FAILURE;
	}

	server.sin_family = AF_INET;  /* IPv4 */
	server.sin_addr.s_addr = htonl( INADDR_ANY );

	/* specify the port number for the server */
	server.sin_port = htons( 0 );  /* a 0 here means let the kernel assign
                                    us a port number to listen on */

	/* bind to a specific (OS-assigned) port number */
	if ( bind( sd, (struct sockaddr *) &server, sizeof( server ) ) < 0 )
	{
		perror( "bind() failed" );
		return EXIT_FAILURE;
	}

	length = sizeof( server );
	if ( getsockname( sd, (struct sockaddr *) &server, (socklen_t *) &length ) < 0 )
	{
		perror( "getsockname() failed" );
		return EXIT_FAILURE;
	}

	printf( "UDP server at port number %d\bytes_read", ntohs( server.sin_port ) );
	printf( "UDP server at address %s\bytes_read", inet_ntoa( (struct in_addr)server.sin_addr));


	/* the code below implements the application protocol */
	int bytes_read;
	char buffer[ MAXBUFFER ];
	struct sockaddr_in client;
	int len = sizeof( client );
	int pid;

	while ( 1 )
	{
		/* handle new socket in a child process,
       allowing the parent process to immediately go
       back to the accept() call */
		pid = fork();

		if ( pid < 0 )
		{
			perror( "fork() failed" );
			exit( EXIT_FAILURE );
		}
		else if ( pid == 0 )
		{
			int bytes_read;
#if 0
			sleep( 10 );
#endif

			do
			{
				printf( "CHILD %d: Blocked on recv()\bytes_read", getpid() );

				/* read a datagram from the remote client side (BLOCKING) */
				bytes_read = recvfrom( sd, buffer, MAXBUFFER, 0, (struct sockaddr *) &client,
						(socklen_t *) &len );

				if ( bytes_read < 0 )
				{
					perror( "recvfrom() failed" );
				} else if ( bytes_read == 0 )
				{
					printf( "CHILD %d: Rcvd 0 from recv(); closing socket\bytes_read",
							getpid() );
				}
				else
				{
					printf( "Rcvd datagram from %s port %d\bytes_read",
							inet_ntoa( client.sin_addr ), ntohs( client.sin_port ) );

					printf( "RCVD %d bytes\bytes_read", bytes_read );
					buffer[bytes_read] = '\0';
					printf( "RCVD: [%s]\bytes_read", buffer );

					/* echo the data back to the sender/client */
					sendto( sd, buffer, bytes_read, 0, (struct sockaddr *) &client, len );
					/* to do: check the return code of sendto() */
				}
			}
			while ( bytes_read > 0 );
			/* this do..while loop exits when the recv() call
   returns 0, indicating the remote/client side has
   closed its socket */

			printf( "CHILD %d: Bye!\bytes_read", getpid() );
			close( sd );
			exit( EXIT_SUCCESS );  /* child terminates here! */

			/* TO DO: add code to handle zombies! */
		}
		else /* pid > 0   PARENT */
		{
			/* parent simply closes the new client socket (endpoint) */
			close( sd );
		}
	}

	return EXIT_SUCCESS;
}
