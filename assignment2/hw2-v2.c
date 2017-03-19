/**
Author: Jessica Marie Barre
Due Date: Sunday, March 19, 11:59:59 PM
Name: TFTP Server
Purpose: To create a simple TFTP Server

Implement a TFTP server according to RFC 1350.

Your server should:
X concurrent using fork()
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


/* To test this server, you can use the following
   command-line netcat tool:

   bash$ netcat -u linux00.cs.rpi.edu 41234

   The hostname (e.g., linux00.cs.rpi.edu) cannot be
   localhost; and the port number must match what
   the server reports.

   Notes:

   TFTP message:
   - RRQ from client TransferModeFileName
   - WRQ to Client or from clients Transfer..
   - DATA to client
   - ACK to client
   - ERROR

   - Clients sends a RRQ or WRQ to server
   - server sends 0
   - client sends Dat1
   - repeat until done
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define MAXBUFFER 8192
#define TFTP_OPCODE_RRQ		1
#define TFTP_OPCODE_WRQ		2
#define TFTP_OPCODE_DATA	3
#define TFTP_OPCODE_ACK		4
#define TFTP_OPCODE_ERROR	5

#define TFTP_DEF_RETRIES	6
#define TFTP_DEF_TIMEOUT_SEC	0
#define TFTP_DEF_TIMEOUT_USEC	50000
#define TFTP_BLOCKSIZE		512
#define TFTP_MAX_MSGSIZE	(4 + TFTP_BLOCKSIZE)

#define TFTP_MODE_OCTET		"octet"

/**
 * Helper function sock()
 */
int Sock() {
	/* create the socket (endpoint) on the server side */
	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	/* UDP */
	if (sd < 0)/* this will be part of the file descriptor table! */
	{
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	return sd;
}

/**
 * Helper function bind()
 */
void Bind(int sd, const struct sockaddr_in* server) {
	/* bind to a specific (OS-assigned) port number */
	if (bind(sd, (struct sockaddr*) server, sizeof(*server)) < 0) {
		perror("bind() failed");
		exit(EXIT_FAILURE);
	} else {
		printf("bind() success\n");
	}
}

/**
 * Helper function getsocketname()
 */
void GetSocketName(int sd, int length, struct sockaddr_in* server) {
	if (getsockname(sd, (struct sockaddr*) server, (socklen_t*) &length)
			< 0) {
		perror("getsockname() failed");
		exit(EXIT_FAILURE);
	} else {
		printf("getsocketname() success\n");
	}
}

void Listen(int sd) {
	if (listen(sd, 5) == -1) {
		perror("Error listening.\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * connection() sends data to the client
 */
void handle_request(int bytes_read, char buffer[MAXBUFFER], int sd,
		const struct sockaddr_in* client, int len) {
	//sendFile here

	/* echo the data back to the sender/client */

	if (sendto(sd, buffer, bytes_read, 0, (struct sockaddr*) client, len)
			< 0) {
		perror("sendto() failed");
		exit(EXIT_FAILURE);
	}

}

int main()
{
	int sd;
	struct sockaddr_in server;
	int length;

	/* create the socket (endpoint) on the server side */

	sd = Sock();

	server.sin_family = AF_INET;  /* IPv4 */
	server.sin_addr.s_addr = htonl( INADDR_ANY );

	/* a 0 here means let the kernel assign
       us a port number to listen on */
	server.sin_port = htons( 0 );

	/* bind to a specific (OS-assigned) port number */
	Bind(sd, &server);

	length = sizeof( server );
	GetSocketName(sd, length, &server);

	printf( "UDP server at port number %d\n", ntohs( server.sin_port ) );
	printf( "UDP server at address %s \n", inet_ntoa( (struct in_addr)server.sin_addr));

	/* the code below implements the application protocol */
	int bytes_read;
	char buffer[ MAXBUFFER ];
	struct sockaddr_in client;
	int len = sizeof( client );

	while ( 1 )
	{
		/* read a datagram from the remote client side (BLOCKING) */
		bytes_read = recvfrom( sd, buffer, MAXBUFFER, 0, (struct sockaddr *) &client,
				(socklen_t *) &len );

		if ( bytes_read < 0 )
		{
			perror( "recvfrom() failed" );
		}
		else
		{
			buffer[bytes_read] = '\0';

			printf( "Rcvd datagram from %s port %d\n",
					inet_ntoa( client.sin_addr ), ntohs( client.sin_port ) );
			printf( "RCVD %d bytes\n", bytes_read );
			printf( "RCVD: \"%s]\"", buffer );

			int pid;

			pid = fork();

			if ( pid < 0 )
			{
				perror( "fork() failed" );
				exit( EXIT_FAILURE );
			}
			else if ( pid == 0 )
			{

				handle_request(bytes_read, buffer, sd, &client, len);

				/* echo the data back to the sender/client */
				sendto( sd, buffer, bytes_read, 0, (struct sockaddr *) &client, len );
				/* to do: check the return code of sendto() */
#if 0
				sleep( 5 );
#endif
			}
			else /* pid > 0   PARENT */
			{
				/* parent simply closes the new client socket (endpoint) */
				wait(NULL);
				close( sd );
				exit(EXIT_SUCCESS); /* child terminates here! */
			}

		}
	}

	return EXIT_SUCCESS;
}
