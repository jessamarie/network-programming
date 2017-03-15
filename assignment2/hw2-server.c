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

  printf( "UDP server at port number %d\n", ntohs( server.sin_port ) );


  /* the code below implements the application protocol */
  int n;
  char buffer[ MAXBUFFER ];
  struct sockaddr_in client;
  int len = sizeof( client );

  while ( 1 )
  {
    /* read a datagram from the remote client side (BLOCKING) */
    n = recvfrom( sd, buffer, MAXBUFFER, 0, (struct sockaddr *) &client,
                  (socklen_t *) &len );

    if ( n < 0 )
    {
      perror( "recvfrom() failed" );
    }
    else
    {
      printf( "Rcvd datagram from %s port %d\n",
              inet_ntoa( client.sin_addr ), ntohs( client.sin_port ) );

      printf( "RCVD %d bytes\n", n );
      buffer[n] = '\0';
      printf( "RCVD: [%s]\n", buffer );

      /* echo the data back to the sender/client */
      sendto( sd, buffer, n, 0, (struct sockaddr *) &client, len );
        /* to do: check the return code of sendto() */
    }
  }

  return EXIT_SUCCESS;
}
