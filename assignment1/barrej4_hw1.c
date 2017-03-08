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


// Note: the select() implementation on Windows (Winsock2)
//fails with any timeout much larger than this
#define LONG_TIME 100000000

static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;

/**
 * callback for DNSServiceRegister which checks for errors.
 *
 * @param service
 * @param flags
 * @param errorCode
 * @param name
 * @param type
 * @param domain the localhost
 * @param void * context
 */
static void MyRegisterCallBack(DNSServiceRef service,
		DNSServiceFlags flags,
		DNSServiceErrorType errorCode,
		const char * name,
		const char * type,
		const char * domain,
		void * context)
{

	//	#pragma unused(flags)
	//	#pragma unused(context)

	if (errorCode != kDNSServiceErr_NoError) {
		fprintf(stderr, "MyRegisterCallBack returned %d\n", errorCode);
	}
	else {
		printf("%-15s %s.%s%s\n","REGISTER", name, type, domain);
	}
}


/**
 * This function Registers a Service. Namely, for this project:
 * "_gtn._tcp"
 *
 * @param port the port to register the service on
 */
static DNSServiceErrorType MyDNSServiceRegister(DNSServiceRef * serviceRef, unsigned int port)
{
	DNSServiceErrorType error;


	error = DNSServiceRegister(&*serviceRef,
			0,                  // no flags
			0,                  // all network interfaces
			"Guessing Game",  // name
			"_gtn._tcp",       // service type
			"local",                 // register in default domain(s)
			NULL,               // use default host name
			htons(port),        // port number
			0,                  // length of TXT record
			NULL,               // no TXT record
			MyRegisterCallBack, // call back function
			NULL);              // no context

	if (error == kDNSServiceErr_NoError)
	{
		DNSServiceRefDeallocate(*serviceRef);
	}

	return error;
}




/**
 * The following functions are are to handle the server and
 * guessing part of the game.
 *
 **/


/*
 * splits input by a specified delimiter
 *
 * @name strsplit
 * @param str the string to split
 * @demlim the delimeter to split by
 * @numtokens the number of tokens that results
 */
char **strsplit(const char* str, const char* delim, size_t* numtokens) {

	char *s = strdup(str);
	size_t tokens_alloc = 1;
	size_t tokens_used = 0;

	char **tokens = calloc(tokens_alloc, sizeof(char*));

	char *token, *rest = s;

	while ((token = strsep(&rest, delim)) != NULL) {


		if (tokens_used == tokens_alloc) {
			tokens_alloc *= 2;
			tokens = realloc(tokens, tokens_alloc * sizeof(char*));
		}


		/* Skip last delimeter */

		if(strcmp(token, "")) {
			tokens[tokens_used++] = strdup(token);
		}
	}


	if (tokens_used == 0) {
		free(tokens);
		tokens = NULL;
	} else {
		tokens = realloc(tokens, tokens_used * sizeof(char*));
	}

	*numtokens = tokens_used;

	free(s);

	return tokens;
}


/*
 * Checks to see if the client's guess equals the answer
 * and sends a response.
 *
 * @name checkGuess
 * @param fd the socket
 * @param guess the user's guess
 * @param answer
 * @param guessCount the number of current guesses
 * @returns 0 if successful, 1 if too high, -1 if too low
 */
int checkGuess(int fd, int guess, int answer, int guessCount) {

	char *message1;
	int n;

	if(guess < answer) {

		message1 = malloc( 40 * sizeof(char));

		sprintf(message1, "GREATER\n");
		n = send(fd, message1, strlen(message1), 0);
		if ( n != strlen(message1) ) {
			perror( "send() failed" );
		}

		free(message1);

		return 1;

	} else if (guess > answer) {

		message1 = malloc( 40 * sizeof(char));

		sprintf(message1, "SMALLER\n");
		n = send(fd, message1, strlen(message1), 0);

		if ( n != strlen(message1) ) {
			perror( "send() failed" );
		}

		free(message1);

		return -1;

	} else {

		/*Successful Guess */

		/* Send "CORRECT" to client */

		message1 = malloc( 40 * sizeof(char));
		char* message2 = malloc( 40 * sizeof(char) );

		sprintf(message1, "CORRECT\n");

		n = send(fd, message1, strlen(message1), 0);

		if ( n != strlen(message1) ) {
			perror( "send() failed" );
		}

		free(message1);


		/* Evalute client's performance */

		int log_of_hundred = log(100) / log(2);

		if (log_of_hundred - 1 > guessCount) {

			sprintf(message2, "GREAT GUESSING\n");

		} else if(log_of_hundred + 1 > guessCount) {

			sprintf(message2, "BETTER LUCK NEXT TIME\n");

		} else {

			sprintf(message2, "AVERAGE\n");

		}

		n = send(fd, message2, strlen(message2), 0);

		if ( n != strlen(message2) ) {
			perror( "send() failed" );
		}

		free(message2);

		return 0;

	}

}

/* This function extracts the guess from the user's input.
 *
 * @returns the user's guess or -1 if command is wrong
 */


int getGuess(int fd, char buffer[]) {

	size_t numTokens;

	char **parts = strsplit(buffer, " ", &numTokens);

	if (numTokens == 2) {

		if (strcasecmp(parts[0], "GUESS") == 0) {

		} else {

			int n = send(fd, "???\n", 4, 0);
			if ( n != 4 ) {
				perror( "send() failed" );
			}
			return -1;
		}

	} else {

		int n = send(fd, "???\n", 4, 0);
		if ( n != 4 ) {
			perror( "send() failed" );
		}
		return -1;
	}
	int guess = atoi(parts[1]);

	int i;
	for(i = 0; i < numTokens; i++){
		free(parts[i]);
	}
	free(parts);

	return guess;
}


/*
 * This function simply generates a random number
 * between 1-100.
 *
 * @name generateRandomNumber
 * @returns a random number between 1 and 100
 */
int generateRandomNumber() {

	int n;
	time_t t;

	/* Intializes random number generator */
	srand((unsigned) time(&t));

	n = rand() % 100 + 1;

	printf("random number: %d\n ", n);

	return n;

}


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

/**
 * kills socket
 *
 * @name die
 * @param msg the message to print
 *
 */

/** MAIN **/

int main()
{

	/** Guess variables **/

	int num;
	int guessCount = 0;


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


	/** Register the Service **/

	DNSServiceErrorType	error;
	DNSServiceRef serviceRef;


	error = MyDNSServiceRegister(&serviceRef, local_port);
	fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);

	int dns_sd_fd = DNSServiceRefSockFD(serviceRef);

	printf("Started server; listening on PORT: %d\n", local_port);


	/** Generate the random number **/

	num = generateRandomNumber();



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

		int maxfd = sock;

		FD_ZERO( &readfds );

		FD_SET(sock, &readfds); // FD_SET to include listener fd, sock

		for ( i = 0; i < client_socket_index ; i++ )
		{
			int sd = client_sockets[i];
			FD_SET( sd, &readfds ); //Set FD_SET to include client socket fd
			if(sd > maxfd)
				maxfd = sd;
		}

#if 0
		/* BLOCK */
		int ready = select( maxfd+1, &readfds, NULL, NULL, NULL );
		/* ready is the number of ready file descriptors */
		printf( "select() identified %d descriptor(s) with activity\n", ready );
#endif

#if 1
		int ready = select(maxfd+1, &readfds, NULL, NULL, &timeout );
		if ( ready == 0 ) {
			//printf( "No activity\n" );
			continue;
		}
#endif

		//HandleEvents(serviceRef, &dns_sd_fd);

		if ( FD_ISSET( sock, &readfds ) )
		{
			/* this accept() call we know will not block */
			int newsock = accept( sock,
					(struct sockaddr *)&client,
					(socklen_t *)&fromlen );

			printf( "Accepted client connection\n" );

			//add new socket to array of sockets

			client_sockets[ client_socket_index++ ] = newsock;
		}



		struct timeval tv;
		int result;
		tv.tv_sec = timeOut;
		tv.tv_usec = 0;
		int nfds = dns_sd_fd + 1;


		while (!stopNow)
		{
			FD_ZERO(&readfds);
			FD_SET(dns_sd_fd, &readfds);
			tv.tv_sec = timeOut;
			tv.tv_usec = 0;

			result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);

			if (result > 0)
			{
				DNSServiceErrorType err = kDNSServiceErr_NoError;
				if (FD_ISSET(dns_sd_fd, &readfds))
					err = DNSServiceProcessResult(serviceRef);
				if (err) { fprintf(stderr,
						"DNSServiceProcessResult returned %d\n", err);
				stopNow = 1; }
			}
			else
			{
				printf("select() returned %d errno %d %s\n",
						result, errno, strerror(errno));
				if (errno != EINTR) stopNow = 1;
			}
		}

		/* read and write to client */

		for ( i = 0 ; i < client_socket_index ; i++ )
		{
			int fd = client_sockets[ i ];

			if ( FD_ISSET( fd, &readfds ) )
			{
				int n, guess = -1, success = -1;

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

					/*	printf( "Received message from %s: %s\n",
							inet_ntoa( (struct in_addr)client.sin_addr ),
							buffer ); */

					printf("Recieved %s",buffer);

					/* Divide input into parts and get the guess */

					guess = getGuess(fd, buffer);

					if (guess != -1) {
						guessCount++;
						success = checkGuess(fd, guess, num, guessCount);
					}

					if (success == 0) {

						close(fd);
						guessCount = 0;
						continue;

					}


				}
			}
		}
	}

	return EXIT_SUCCESS; /* we never get here */
}
