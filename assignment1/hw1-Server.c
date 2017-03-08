#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>


#define BUFFER_SIZE 20


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


/** main **/

int main() {


  /** Guess variables **/

	int n;
	int guessCount = 0;


	/* socket structures */

	struct sockaddr_in server;

	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	unsigned short port =  8127; /* Get from zeroconf */


	/* Create the listener socket as TCP socket */

	int sd = create_socket(&server, port);

	struct sockaddr_in client;
	int fromlen = sizeof( client );

	int sock = select( sd, (struct sockaddr *)&client, (socklen_t*)&fromlen );

	printf("Trying %s\n", inet_ntoa( (struct in_addr)client.sin_addr));
	fflush(NULL);


	printf("Connect to localhost.");

	/** Generate a random number **/
	
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




	return EXIT_SUCCESS;


}
