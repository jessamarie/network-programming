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


/** Function for handling events **/
void HandleEvents(DNSServiceRef serviceRef) {
	int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
	int nfds = dns_sd_fd + 1;
	fd_set readfds;
	struct timeval tv;
	int result;
	
//...
	while (!stopNow)
		{
		
		//... 
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

}// end HandleEvents


/**Functions for browsing **/
static void MyBrowseCallBack(DNSServiceRef service,
				DNSServiceFlags flags,
				uint32_t interfaceIndex,
				DNSServiceErrorType errorCode,
				const char * name,
				const char * type,
				const char * domain,
				void * context)
	{
	//#pragma unused(context)
	if (errorCode != kDNSServiceErr_NoError)
		fprintf(stderr, "MyBrowseCallBack returned %d\n", errorCode);
	else
		{
		char *addString  = (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE";
		char *moreString = (flags & kDNSServiceFlagsMoreComing) ? "MORE" : "    ";
		printf("%-7s%-5s %d%s.%s%s\n",
			addString, moreString, interfaceIndex, name, type, domain);
		}
	if (!(flags & kDNSServiceFlagsMoreComing)) fflush(stdout);
	
} // end MyBrowseCallBack


static DNSServiceErrorType MyDNSServiceBrowse()
	{

	DNSServiceErrorType error;
	DNSServiceRef  serviceRef;
	
	error = DNSServiceBrowse(&serviceRef,
							0,                // no flags
							0,                // all network interfaces
							"_gtn._tcp",     // TODO: service type _gtn.
							"local",               // TODO: default domains local!
							MyBrowseCallBack, // call back function
							NULL);            // no context
	if (error == kDNSServiceErr_NoError)
		{
		HandleEvents(serviceRef); // Add service to runloop to get callbacks
		DNSServiceRefDeallocate(serviceRef);
		}

	return error;

	} //End MyDNSServiceBrowse()


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

/* Register methods */
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

	if (errorCode != kDNSServiceErr_NoError)
		fprintf(stderr, "MyRegisterCallBack returned %d\n", errorCode);
	else
		printf("%-15s %s.%s%s\n","REGISTER", name, type, domain);

	}



static DNSServiceErrorType MyDNSServiceRegister(int port)
	{
	DNSServiceErrorType error;
	DNSServiceRef serviceRef;
	
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


/**Server/game functions**/

/* This function generates a random number

	Returns 
		a random number between 1 and 100
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



/* This function gets input from the user

	Returns 
		the user's guess
*/

int getGuess(int sock) {

	int guess = 0;
	int n;
	char bff1[BUFFER_SIZE];
	
	char* buffer = (char *) calloc (BUFFER_SIZE, sizeof(char));


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

	free(buffer);

	return guess;
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

int create_socket(struct sockaddr_in* server, unsigned short port) {

	/* Create the listener socket as TCP socket */
	int sd = socket(PF_INET, SOCK_STREAM, 0);

	if (sd < 0) {

		perror("ERROR: Could not create socket.");
		exit(EXIT_FAILURE);

	}

	server->sin_port = htons(port);
	int len = sizeof(*server);

	if (bind(sd, (struct sockaddr*) &*server, len) < 0) {

		perror("Error: bind() failed.");
		exit(EXIT_FAILURE);
	}

	listen(sd, 5); /* 5 is the max number of waiting clients */

	printf("Started server; listening on port: %d\n", port);
	fflush(NULL);

	return sd;
}


/** MAIN **/
int main (int argc, const char * argv[])

{

 /** Guess variables **/

	//int n;
	int guessCount = 0;


	/* socket structures */

	int					i, maxi, maxfd, listen_sock, connfd, sockfd;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[BUFFER_SIZE];
	socklen_t			clilen;
	struct sockaddr_in	client_addr, server_addr;

	DNSServiceRef	serviceRef;
	DNSServiceErrorType	error;

	
	/* set up the socket */

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_sock < 0) {

		perror("ERROR: Could not create socket.");
		exit(EXIT_FAILURE);
	}

	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port        = htons(0);

	if (bind(listen_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {

		perror("Error: bind() failed.");
		exit(EXIT_FAILURE);
	}

	socklen_t sz = sizeof(server_addr);
	int ret = getsockname(listen_sock, (struct sockaddr*)&server_addr, &sz);
	
	if (ret < 0) {
		printf("Problem!\n");
		exit(-1);
	}

	printf("Port is %d\n", ntohs(server_addr.sin_port));

	listen(listen_sock, 5); /* 5 is the max number of waiting clients */

	/** Register the Service **/
	error = MyDNSServiceRegister(ntohs(server_addr.sin_port));
	fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);

	

  /* Set up for clients*/

	maxfd = listen_sock;			/* initialize */
	maxi = -1;					/* index into client[] array */
	
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;			/* -1 indicates available entry */
	}

	FD_ZERO(&allset);
	FD_SET(listen_sock, &allset);

	for ( ; ; ) {

		rset = allset;		/* structure assignment */

		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listen_sock, &rset)) {	/* new client connection */

			clilen = sizeof(client_addr);
			connfd = accept(listen_sock, (struct sockaddr*) &client_addr, &clilen);

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
				} else
					write(sockfd, buf, n); // do stuff here

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}

	/** Generate a random number **/
	
/*
	n = generateRandomNumber();

	while ( 1 )
	{

	int guess = getGuess(sock);
	guessCount++;

	checkGuess(sock, guess, n);

	}

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
	

	send(sock, message, strlen(message), 0);
	fflush(NULL);


  close(sock);

	close( sd );

*/

	error = MyDNSServiceBrowse();
	if (error) fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);

	error = MyDNSServiceResolve();
	fprintf(stderr, "DNSServiceDiscovery returned %d\n", error);
									//if function returns print error
	
	return EXIT_SUCCESS;
}
