#include <dns_sd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

// Note: the select() implementation on Windows (Winsock2)
//fails with any timeout much larger than this
#define LONG_TIME 100000000
#define BUFFER_SIZE 20

static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;


/**
 * Function for Handling Events
 *
 * @param serviceRef the DNS service reference
 */
void HandleEvents(DNSServiceRef serviceRef) {

	int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
	int nfds = dns_sd_fd + 1;
	fd_set readfds;
	struct timeval tv;
	int result;

	while (!stopNow)
	{
		FD_ZERO(&readfds);
		FD_SET(dns_sd_fd, &readfds);
		tv.tv_sec = timeOut;
		tv.tv_usec = 0;

		result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);

		if (result > 0) {
			DNSServiceErrorType err = kDNSServiceErr_NoError;

			if (FD_ISSET(dns_sd_fd, &readfds)) {
				err = DNSServiceProcessResult(serviceRef);
			}

			if (err) {
				fprintf(stderr,"DNSServiceProcessResult returned %d\n", err);
				stopNow = 1;
			}

		} else {

			printf("select() returned %d errno %d %s\n", result, errno, strerror(errno));

			if (errno != EINTR) {
				stopNow = 1;
			}

		} // end else

	} // end while

}// end HandleEvents


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
static DNSServiceErrorType MyDNSServiceRegister(unsigned int port)
{
	DNSServiceRef serviceRef;
	DNSServiceErrorType error;


	error = DNSServiceRegister(&serviceRef,
			0,                  // no flags
			0,                  // all network interfaces
			"Not a real page",  // name
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
		HandleEvents(serviceRef);
		DNSServiceRefDeallocate(serviceRef);
	}

	return error;
}


/** Functions for resolving **/
static void MyResolveCallBack(DNSServiceRef serviceRef,
		DNSServiceFlags flags,
		uint32_t interface,
		DNSServiceErrorType errorCode,
		const char *fullname,
		const char *hosttarget,
		uint16_t port,
		uint16_t txtLen,
		const char *txtRecord,
		void *context)
{
	//	#pragma unused(flags)
	//	#pragma unused(fullname)

	if (errorCode != kDNSServiceErr_NoError)
		fprintf(stderr, "MyResolveCallBack returned %d\n", errorCode);
	else
		printf("RESOLVE: %s is at %s:%d\n", fullname, hosttarget, ntohs(port));
	if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);

} //End myResolveCallBack


static DNSServiceErrorType MyDNSServiceResolve()
{
	DNSServiceErrorType error;
	DNSServiceRef  serviceRef;

	error = DNSServiceResolve(&serviceRef,
			0,  // no flags
			0,  // all network interfaces
			"Not a real page", //name
			"_gtn._tcp", // service type
			"local", //domain
			MyResolveCallBack,
			NULL);	 // no context

	if (error == kDNSServiceErr_NoError)
	{
		HandleEvents(serviceRef);  // Add service to runloop to get callbacks
		DNSServiceRefDeallocate(serviceRef);
	}
	return error;
}


/**
 * The following functions are are to handle the server and
 * guessing part of the game.
 *
 **/


/*
 * This Function splits the users input
 *
 * Example "GUESS 50" becomes
 * ["Guess", "50"]
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




/* This function is to check the users guess

	Params
		guess the guess from the user
		answer the answer to the game

	Returns
		0 if successful, 1 if too high, -1 if too low
 */
int checkGuess(int sock, int guess, int answer) {

	char *response;

	response = malloc( 100 * sizeof(char));

	/** copy responses into this ptr **/

	if(guess < answer) {

		sprintf(response, "GREATER");
		send(sock, response, strlen(response), 0);
		fflush(NULL);

		return 1;

	} else if (guess > answer) {

		sprintf(response, "SMALLER");
		send(sock, response, strlen(response), 0);
		fflush(NULL);

		return -1;

	} else {

		return 0;

	}

}

/* This function gets input from the user
 *
 * @returns the user's guess
 */


int getGuess(int sock) {

	int guess = 0;
	int n;
	char buffer[BUFFER_SIZE];

	/* Receive instruction */

	do {

		n = recv(sock, buffer, BUFFER_SIZE, 0);
		fflush(stdout);

		printf("\nbuffer is %s\n", buffer);
		printf("n is %d\n", n);


		if (n < 0) {

			perror("recv() failed");
			fflush(NULL);

		} else if (n == 0) {

			fflush(NULL);

			printf("Rcvd 0 from recv(); closing socket\n");

		} else {

			buffer[n] = '\0';
			size_t numTokens;


			printf("made it here \n");
			char **parts = (char **) calloc (2, sizeof(char*));

			parts = strsplit(buffer, " ", &numTokens);

			int i;
			for (i =0; i < numTokens; i++)
				printf("\n token is %s and numtokens is %lu\n", parts[i], numTokens);

			if (numTokens != 2) {
				perror("wrong number of tokens");
			}

			// TODO: check if parts1 = guess and parts 2 isdigit

			guess = atoi(parts[1]);

			fflush(NULL);

		}

	} while (n > 0);

	return guess;
}


/* This function simply generates a random number

	Returns a random number between 1 and 100
 */
int generateRandomNumber() {

	int n;
	time_t t;

	/* Intializes random number generator */
	srand((unsigned) time(&t));

	n = rand() % 100;

	printf("\nrandom number: %d ", n);

	return n;

}


/**
 * Creates a connection with a socket. It handles
 * socket(), bind(), and listen().
 *
 * @name create_socket
 * @param server_addr the server address
 *
 */
int create_socket(struct sockaddr_in* server_addr) {
	return 0;
}

/**
 * kills socket
 *
 * @name die
 * @param msg the message to print
 *
 */
void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}


/** MAIN **/
int main (int argc, const char * argv[])

{
	/** Guess variables **/

	int num;
	int guessCount = 0;


	/* socket structures */

	int					i, maxi, maxfd, listen_sock, connfd, sockfd, local_port;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[BUFFER_SIZE];
	socklen_t			clilen;
	struct sockaddr_in	client_addr, server_addr, temp_addr;

	DNSServiceErrorType	error;

	unsigned int short port = 0;


	/* set up the socket */

	// Create socket
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_sock < 0) {

		perror("ERROR: Could not create socket.");
		exit(EXIT_FAILURE);
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);  //the port is chosen as the first available.

	// Bind socket
	if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {

		perror("Error: bind() failed.");
		exit(EXIT_FAILURE);
	}

	// Get port info to register the service
	socklen_t sz = sizeof(temp_addr);
	int ret = getsockname(listen_sock, (struct sockaddr*)&temp_addr, &sz);
	if (ret < 0) {
		printf("Problem!\n");
		exit(-1);
	}

	local_port = ntohs(temp_addr.sin_port);

	// Make socket listen
	listen(listen_sock, 32); /* 32 is the max number of waiting clients */

	printf("Started server; listening on port: %d\n", ntohs(server_addr.sin_port));
	printf("Address is %s\n", inet_ntoa( (struct in_addr)server_addr.sin_addr));


	/** Register the Service **/

	error = MyDNSServiceRegister(local_port);
	fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);


	/* Set up iterative server using select() */

	maxfd = listen_sock;			/* initialize */
	maxi = -1;					/* index into client[] array */

	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;			/* -1 indicates available entry */
	}

	FD_ZERO(&allset);
	FD_SET(listen_sock, &allset);

	/* Start the game */

	while (1) {

		num = generateRandomNumber();  //generate a random number

		(void)printf("running select()\n");

		rset = allset;		/* structure assignment */

		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listen_sock, &rset)) {	/* new client connection */

			clilen = sizeof(client_addr);
			connfd = accept(listen_sock, (struct sockaddr*) &client_addr, &clilen);

			printf("Received incoming connection from: %s\n", inet_ntoa( (struct in_addr)client_addr.sin_addr));
#ifdef	NOTDEF
			printf("new client: %s, port %d\n",
					Inet_ntop(AF_INET, &client_addr.sin_addr, 4, NULL),
					ntohs(client_addr.sin_port));
#endif

			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			}

			if (i == FD_SETSIZE)
				perror("too many clients");

			FD_SET(connfd, &allset);	/* add new descriptor to set */

			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = read(sockfd, buf, BUFFER_SIZE)) == 0) {
					/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					//write(sockfd, buf, n); // do stuff here

					while(1) {

						printf("%s",buf);
						int guess = getGuess(sockfd);
						guessCount++;

						checkGuess(sockfd, guess, num);

						char message[BUFFER_SIZE];

						sprintf(message, "CORRECT");

						int log_of_hundred = log(100) / log(2);

						if (log_of_hundred - 1 > guessCount) {

							sprintf(message, "GREAT GUESSING");

						} else if(log_of_hundred + 1 > guessCount) {

							sprintf(message, "BETTER LUCK NEXT TIME");

						} else {

							sprintf(message, "AVERAGE");

						}


						send(sockfd, message, strlen(message), 0);
						fflush(NULL);

						//close here?

					}
				}

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}

	return EXIT_SUCCESS;
}
