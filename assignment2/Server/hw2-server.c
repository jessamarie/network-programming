/**
Author: Jessica Marie Barre
Due Date: Sunday, March 19, 11:59:59 PM
Name: TFTP Server
Purpose: To create a simple TFTP Server

Implement a TFTP server according to RFC 1350.

Your server should:
X concurrent using fork()
- "octet" mode ONLY
- Upon not receiving data for 1 SECOND:
  - SENDER should retransmit its last packet.
- If you have not heard from the other party for 10 SECONDS:
   - abort the connection.
- AVOID Sorcerer’s Apprentice Syndrome  covered in Short Lecture 11.
- We will only be testing files SMALLER than 32 MB (512 bytes).
- TIMEOUTS: SIGALRM may be useful (CH 14)

- When TESTING using tftp Linux client (for example)
  - set the mode to binary.
 */



#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>


#define PORT      0

/** OP Codes **/
#define OPCODE_RRQ		1
#define OPCODE_WRQ		2
#define OPCODE_DATA	    3
#define OPCODE_ACK		4
#define OPCODE_ERROR	5

#define TIMEOUT_BREAK   1  // 1 sec
#define TIMEOUT_ABORT   10 // 10 sec

#define DEF_RETRIES	6
#define DEF_TIMEOUT_SEC	0
#define DEF_TIMEOUT_USEC	50000
#define BLOCKSIZE		512
#define MAX_MSGSIZE	(4 + BLOCKSIZE)

#define MODE_OCTET		"octet"

/** ERROR Codes **/

#define ERR_NOT_DEFINED	        0 //see error message if any
#define ERR_FILE_NOT_FOUND	    1
#define ERR_ACCESS_VIOLATION	2
#define ERR_DISK_FULL	        3 //or allocation exceeded
#define ERR_ILLEGAL_OP			4
#define ERR_UNKNOWN_TID			5
#define ERR_FILE_EXISTS			6
#define ERR_NO_SUCH_USER	    7

#define ERRMSG_BAD_PACKET        "BAD PACKET SIZE/FORMAT"
#define ERRMSG_INVALID_MODE		 "INVALID TRANSFER MODE"
#define ERRMSG_ERROR_RECEIVED    "ERROR RECEIVED"
#define ERRMSG_FILE_NOT_FOUND    "FILE NOT FOUND"


#define ERRMSG_ILLEGAL_OP        "ILLEGAL OPCODE"
#define ERRMSG_FILE_EXISTS       "FILE EXISTS"

/* tftp message structure */
typedef union {

	uint16_t opcode;

	struct {
		uint16_t opcode;
		uint8_t data[514];
	} request; /* RRQ or WRQ */

	struct {
		uint16_t opcode;
		uint16_t block_num;
		uint8_t data[BLOCKSIZE];
	} data;

	struct {
		uint16_t opcode;
		uint16_t block_num;
	} ack;

	struct {
		uint16_t opcode;
		uint16_t error_code;
		uint8_t error_message[BLOCKSIZE];
	} error;

} tftp_message;


/**
 * A function to create a socket and error check
 *
 * @return the socket descriptor
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
 * Bind a socket to an internet address and error check
 *
 * @param sd the socket descriptor
 * @param server the structure describing the address
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
 * Get information about the address of the socket and error check
 *
 * @param sd the socket descriptor
 * @param length the length of the address structure
 * @param server the structure describing the address
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

/**
 * A helper function that calls the Sock(), Bind() and
 * GetSockName() functions, and sets varous properties
 * of the address structure
 *
 * @param server the address structure
 * @returns the socket descriptor
 */
int create_and_bind_socket(struct sockaddr_in* server) {

	/* create the socket (endpoint) on the server side */
	int sd = Sock();

	int servlen = sizeof(*server);

	bzero((char*) server, servlen);
	server->sin_family = AF_INET; /* IPv4 */
	server->sin_addr.s_addr = htonl(INADDR_ANY);
	/* 0 - lets kernel assign a port number to listen on */
	server->sin_port = htons(PORT);

	/* bind to a specific (OS-assigned) port number */
	Bind(sd, server);

	servlen = sizeof(*server);
	GetSocketName(sd, servlen, server);

	return sd;
}


/** This function splits the contents of a string by the
 * null terminator
 * @param str
 * @param filename the filename to fill
 * @param mode the mode to fill
 * @return the number of tokens
 * **/

size_t get_tokens(char* str, char** filename, char** mode) {

	char *ptr = str;
	int numtokens = 0;

	if (*ptr) {
		*filename = strdup(ptr);
		numtokens++;
	} else {
		return -1;
	}

	ptr += strlen(ptr) + 1;

	if (*ptr) {
		*mode = strdup(ptr);
		numtokens++;
	} else {
		return -1;
	}

	return numtokens;
}


/*
 * Sends error code back to client
 *
 * 		  2by    2by        string     1by
 * ERROR:  ---------------------------------
 *       | 5 | ErrorCode |  ErrorMsg  | 0  |
 *       -----------------------------------
 *
 * @param sd the socket descriptor
 * @param block_num the block number
 * @param msg the message to send back to the client
 * @param client the client address
 * @param len the length of the client address
 * @return the number of bytes sent
 */
int send_error(int sd, uint16_t error_code, char* msg, struct sockaddr_in *client, socklen_t len) {

	// TODO:
	// ADD IF else for easier code
	// ADD # for exit condition ex: if 1 exit(1)

	perror(msg);
	tftp_message message;
	int bytes_sent;

	message.opcode = htons(OPCODE_ERROR);
	message.error.error_code = htons(error_code);
	memcpy(message.error.error_message, (uint8_t *) msg, sizeof(msg)/sizeof(uint8_t));

	bytes_sent = sendto(sd, &message, sizeof(message.error), 0, (struct sockaddr *) client, len);

	if(bytes_sent < 0) perror("sendto() failed: ERROR");

	return bytes_sent;
}


/*
 * Retrieves a request and error checks
 *
 * @param sd the socket descriptor
 * @param message the packet structure
 * @param client the client address structure
 * @param len the length of the client address
 * @effect fills the message structure with a packet
 *         from the client
 * @return the number of bytes_read
 */
int get_packet(int sd, tftp_message *message, struct sockaddr_in *client, socklen_t *len) {

	int bytes_read;

	bytes_read = recvfrom(sd, message, sizeof(*message), 0, (struct sockaddr *) client, len);

	if (bytes_read < 0) perror("recvfrom() failed");

	if(bytes_read < 4) {
		send_error(sd, ERR_NOT_DEFINED, ERRMSG_BAD_PACKET, client, *len);
	}

	return bytes_read;
}

static void packet_alarm(int signo){
	return;
}

/*
 * Waits for a request and handles timeout
 *
 * @param sd the socket descriptor
 * @param message the packet structure
 * @param client the client address structure
 * @param len the length of the client address
 * @effect fills the message structure with a packet
 *         from the client
 * @return the number of bytes_read
 */
int retrieve_packet(int sd, tftp_message *message, uint16_t opcode, struct sockaddr_in *client, socklen_t *len) {

	struct sigaction action;

	int bytes_recieved = 0;

	action.sa_handler = packet_alarm;

	if (sigfillset(&action.sa_mask) < 0) return -1;

	action.sa_flags = 0;

	if (sigaction(SIGALRM,&action,0) < 0) return -1;

	alarm(TIMEOUT_BREAK);

	errno = 0;

	while (1)
	{

		bytes_recieved = get_packet(sd, message, client, len);

		if (errno != 0) {
			if (errno == EINTR) printf("Timeout Occured.\n");
			else printf("Errno with value[%d]\n",errno);

			alarm(0);
			return -1;
		}

		if (bytes_recieved < 0)
		{
			alarm(0);
			return -1;
		}

		// redundant?
		if (message->opcode == opcode)
		{
			alarm(0);
			return bytes_recieved;
		}
		else if(message->opcode == OPCODE_ERROR)
		{
			alarm(0);
			return 0;
		}
	}
	//should never reach this
	alarm(0);
	return -1;
}

/*
 * Gets an ACK upon recieving a read request and error checks
 *
 *              2by             2by
 * ACK:  -----------------------------------
 *       |       4        |    Block #     |
 *       -----------------------------------
 *
 * @param sd the socket descriptor
 * @param block_num the block number it should be
 * @param client the client address
 * @param len the client address length
 * @return the number of bytes receieved
 */
int get_ack(int sd, uint8_t block_num, struct sockaddr_in *client, socklen_t len) {

	tftp_message message;
	int timeouts = 0;

  /* ISSUE with timeouts here **/
	//int bytes_received =  get_packet(sd, &message, client, &len);
	int bytes_received = retrieve_packet(sd, &message, OPCODE_ACK, client, &len);


	if (bytes_received < 0) {
		if (timeouts > TIMEOUT_ABORT) {
			// error timeout limit reached
			exit(EXIT_FAILURE);
		}

		timeouts++;

	} else {

		if (bytes_received < 4) exit(EXIT_FAILURE);

		uint16_t opcode = ntohs(message.opcode);

		if(opcode == OPCODE_ERROR) {
			send_error(sd, message.error.error_code, (char *) message.error.error_message, client, len);
			exit(EXIT_FAILURE);
		}

		if (opcode != OPCODE_ACK) {
			send_error(sd, ERR_ILLEGAL_OP, ERRMSG_ILLEGAL_OP, client, len);
			exit(EXIT_FAILURE);
		}

		if (block_num != ntohs(message.ack.block_num)) {
			send_error(sd, ERR_UNKNOWN_TID,ERRMSG_BAD_PACKET, client, len);
			exit(EXIT_FAILURE);
		}

	}

	return bytes_received;
}

/*
 * Gets data from the client
 *
 *		   2by     2by		  n bytes
 * DATA:  ----------------------------------
 *       |  3  | block # |      DATA       |
 *       -----------------------------------
 *
 * @param sd the socket descriptor
 * @param block_num the block number
 * @param data the file contents to send back to the client
 * @param client the client address
 * @param len the length of the client address
 * @return the number of bytes sent
 */
int get_data(int sd, uint8_t block_num, tftp_message* message, struct sockaddr_in *client, socklen_t len) {

	int timeouts = 0;

	int bytes_received = retrieve_packet(sd, message, OPCODE_DATA, client, &len);

	if (bytes_received < 0) {
		if (timeouts > TIMEOUT_ABORT) {
			// error timeout limit reached
			exit(EXIT_FAILURE);
		}

		timeouts++;

	} else {

		if (bytes_received < 4) exit(EXIT_FAILURE);

		printf("ACK recieved: %d\n", bytes_received); //debugging

		uint16_t opcode = ntohs(message->opcode);

		printf("opcode is %d\n", opcode); //debugging

		if(opcode == OPCODE_ERROR) {
			perror(ERRMSG_ERROR_RECEIVED);
			send_error(sd, message->error.error_code, (char *)message->error.error_message, client, len);
			exit(EXIT_FAILURE);
		}

		if (opcode != OPCODE_DATA) {
			send_error(sd, ERR_ILLEGAL_OP, ERRMSG_ILLEGAL_OP, client, len);
			exit(EXIT_FAILURE);
		}

		//debugging
		printf("blockum is %d\n", block_num);
		printf("ack blocknum is %d\n", ntohs(message->ack.block_num));

		if (block_num != ntohs(message->ack.block_num)) {
			send_error(sd, ERR_UNKNOWN_TID, ERRMSG_BAD_PACKET, client, len);
			exit(EXIT_FAILURE);
		}

	}

	return bytes_received;
}


/*
 * Sends ACK upon recieving a write request and error checks
 *
 *             2by             2by
 * ACK:  -----------------------------------
 *       |       4        |    Block #     |
 *       -----------------------------------
 *
 * @param sd the socket descriptor
 * @param block_num the block number
 * @param client the client address
 * @param len the client address length
 * @return the number of bytes sent
 */
int send_ack(int sd, uint16_t block_num, struct sockaddr_in *client, socklen_t len) {

	tftp_message message;
	int bytes_sent;

	message.opcode = htons(OPCODE_ACK);
	message.ack.block_num = htons(block_num);

	//debugging
	printf("blockum is %d\n", block_num);
	printf("ack blocknum is %d\n", ntohs(message.ack.block_num));

	bytes_sent = sendto(sd, &message, sizeof(message.ack), 0, (struct sockaddr *) client, len);

	if(bytes_sent < 0) perror("sendto() failed: ACK");

	return bytes_sent;
}


/*
 * Sends data back to client
 *
 *		   2by     2by		  n bytes
 * DATA:  ----------------------------------
 *       |  3  | block # |      DATA       |
 *       -----------------------------------
 *
 * @param sd the socket descriptor
 * @param block_num the block number
 * @param data the file contents to send back to the client
 * @param client the client address
 * @param len the length of the client address
 * @return the number of bytes sent
 */
int send_data(int sd, uint16_t block_num, uint8_t* data, ssize_t bytes_read, struct sockaddr_in *client, socklen_t len) {

	tftp_message message;
	int bytes_sent;

	message.opcode = htons(OPCODE_DATA);
	message.data.block_num = htons(block_num);
	memcpy(message.data.data, data, bytes_read);

	bytes_sent = sendto(sd, &message, bytes_read+4, 0, (struct sockaddr *) client, len);

	if(bytes_sent < 0) perror("sendto() failed: DATA");
	printf("BYTES SENT: %d\n", bytes_sent);

	return bytes_sent;
}


/*
 * Sends data from the requested file to the client
 *
 * @param sd the sock descriptor
 * @param filename the file to read from
 * @param client the client address
 * @param len the client length
 *
 * @return the number of bytes read
 * OR error/success code?
 */
int read_data(int sd, char * filename, struct sockaddr_in* client, socklen_t len) {

	printf("\nClient wants to recieve a file....\n");

	FILE *file;
	uint16_t block_num = 0;

	if( access( filename, F_OK | R_OK) != 0)
	{
		send_error(sd, ERR_FILE_NOT_FOUND, ERRMSG_FILE_NOT_FOUND, client, len);
		exit(EXIT_FAILURE);
	}

  file = fopen(filename, "r");
	if (file == NULL) { perror("Error: fopen() failed"); exit(EXIT_FAILURE); }

	ssize_t bytes_sent = 0;
	ssize_t bytes_read = 0;

	do {

		uint8_t* data = (uint8_t *) malloc(sizeof(char) * BLOCKSIZE);

		//uint8_t data[BLOCKSIZE];

		bytes_read = fread(data, sizeof(char), BLOCKSIZE, file);
		block_num++;

		printf("BYTES READ from file: %lu\n", bytes_read);

		if (bytes_read < 4) { exit(EXIT_FAILURE);}

		//data[bytes_read] = '\0';
		data = (uint8_t *) realloc(data, bytes_read * sizeof(uint8_t));

		// debugging
		printf("DATA READ from file: %s", data);

		/** send block to client **/
		bytes_sent = send_data(sd, block_num, data, bytes_read, client, len);

		printf("BYTES SENT to client: %lu\n", bytes_sent);
		if (bytes_sent < 0) exit(EXIT_FAILURE);

		get_ack(sd, block_num, client, len);

	} while (bytes_read == BLOCKSIZE);
	// Only last data contents < 512
	// MAybe add ifEnd?

	printf("%s.%u: File successfully Sent\n",
			inet_ntoa(client->sin_addr), ntohs(client->sin_port));

	fclose(file);

	return 0; //successe
}


/*
 * Writes file to requested file for the client. Sends
 * in packets of size 4 (opcode/blocknum) + 512 (data)
 *
 * @param sd the sock descriptor
 * @param filename the file to write to
 * @param client the client address
 * @param len the client length
 *
 * @return the number of bytes read
 * OR error/success code?
 */
int write_data(int sd, char* filename, struct sockaddr_in* client, socklen_t len) {

	printf("\nclient wants to transfer file to server...\n");

	FILE *file;
	uint16_t block_num = 0;

	if(access( filename, F_OK ) == 0)
	{
		send_error(sd, ERR_FILE_EXISTS, ERRMSG_FILE_EXISTS, client, len);
		exit(EXIT_FAILURE);
	}

	file = fopen(filename, "wb");
	if (file == NULL) { perror("Error: fopen() failed"); exit(EXIT_FAILURE);}

	int bytes_read = 0;

	do {

		tftp_message message;

		/* Send ACK to client BEFORE recieving DATA */
		send_ack(sd, block_num, client, len);

		/* ISSUE with timeouts here **/
		bytes_read = get_data(sd, block_num, &message, client, len);
		//bytes_read = get_packet(sd, &message, client, &len);

		if (bytes_read < 4) { perror("serious issues here\n"); exit(EXIT_FAILURE); }

		block_num++;

		int bytes_written = fwrite(message.data.data, sizeof(char), bytes_read - 4, file);

		printf("BYTES written to server: %d\n", bytes_written);
		printf("DATA Written: %s", message.data.data);

		if (bytes_written < 0) { perror("fwrite() failed"); exit(EXIT_SUCCESS); }

	} while (bytes_read == MAX_MSGSIZE);
	// Only last data contents < 512
	// MAybe add ifEnd?

   /* LAST ACK */
	send_ack(sd, block_num, client, len);


	printf("%s.%u: File successfully transfered!\n",
			inet_ntoa(client->sin_addr), ntohs(client->sin_port));

	fclose(file);

	return bytes_read;
}


/*
 * handles a valid client request by opening a new socket
 * for communication and appropriately responding
 *
 * 		       2by   string     1by   string  1by
 * RRQ/WRQ:  ------------------------------------
 *           | 1/2 | filename |  0  |  Mode  | 0 |
 *           ------------------------------------
 *
 * @param message
 * @param bytes_read
 * @param client
 * @param len
 */
void handle_request(tftp_message *message,
		struct sockaddr_in* client, socklen_t len) {

	struct timeval tv;
	int client_sd;

	char *filename;
	char *mode;
	uint16_t opcode;

	size_t numtokens;
	//char ** parts;

	client_sd = Sock();

	tv.tv_sec  = TIMEOUT_BREAK;
	tv.tv_usec = 0;

	/** Set the max time period a function waits til it completes **/
	if(setsockopt(client_sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt() failed");
		exit(EXIT_FAILURE);
	}

	numtokens = get_tokens((char *) message->request.data, &filename, &mode);

	if (numtokens  != 2) {
		send_error(client_sd, ERR_NOT_DEFINED, ERRMSG_BAD_PACKET, client, len);
		exit(EXIT_FAILURE);
	}

	if (strcasecmp(mode, MODE_OCTET) != 0) {
		send_error(client_sd, ERR_NOT_DEFINED, ERRMSG_INVALID_MODE, client, len);
		exit(EXIT_FAILURE);
	}

	opcode = ntohs(message->opcode);

	if(opcode == OPCODE_RRQ) {

		read_data(client_sd, filename, client, len);

	} else { /** WWQ  **/

		write_data(client_sd, filename, client, len);
	}

	printf("Closing client socket...\n");
	close(client_sd);

}

int main()
{
	int sd;
	struct sockaddr_in server;

	/* create the socket (endpoint) and bind on the server side */
	sd = create_and_bind_socket(&server);
	printf("UDP server at port number %d and address %s\n",
			ntohs(server.sin_port),
			inet_ntoa((struct in_addr)server.sin_addr));


	/* Implement the application protocol */

	while (1)
	{
		struct sockaddr_in client;
		int clilen = sizeof(client);
		int bytes_read;
		int pid;
		tftp_message message;
		uint16_t opcode;

		/* read a datagram from the remote client side (BLOCKING) */

		bytes_read = get_packet(sd, &message, &client, (socklen_t *) &clilen);

		if(bytes_read < 4) continue;

#if 0
		printf( "Rcvd datagram from %s port %d\n",
				inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("RCVD %d bytes\n", bytes_read);
#endif

		opcode = ntohs(message.opcode);

#if 0
		printf( "RCVD: Opcode:\"%d \n", opcode); //deubugging
#endif

		if (opcode == OPCODE_RRQ || opcode == OPCODE_WRQ) {

			pid = fork();

			if ( pid < 0 )
			{
				perror("fork() failed");
				return EXIT_FAILURE;
			}
			else if ( pid == 0 ) /* CHILD */
			{
				/* no longer need parent socket in the child */

				close(sd);

				/* Exchange ADDITIONAL datagrams w/ client on new socket */

				handle_request(&message, &client, clilen);

				exit(EXIT_SUCCESS); /* child terminates here! */

#if 0
				sleep( 5 );
#endif
			}
			else /* pid > 0   PARENT */
			{
				/* parent waits for child to terminate */

				wait(NULL);
			}

		} else {
			send_error(sd, ERR_ILLEGAL_OP, ERRMSG_ILLEGAL_OP, &client, clilen);
		}

	} // end while

	return EXIT_SUCCESS;
}

/* NOTES:
 * TFTP message: RRQ, WRQ, DATA, ACK, ERROR
   - SERVER RECIEVES a RRQ or WRQ w/ host TID
   - SERVER SENDS
     - DATA block 1 with matching Dest TID + it's Source TID
     - OR ERROR
   - CLIENT sends Dat1
   - REPEAT until done

 *
 * * TFTP Limitations:
 * - no retransmission
 * - send size limite
 * - ACK system only ensures no wasting of OUTBOUND network bandwith
 *
 * Reliability & Timeout:
 * T_R = reciever timeout
 * T_S = sender timeout
 * - if reciever gets no data recieved after T_R since last sent ACK, resend ACK
 * - if sender recieves no ACK after T_S resend last DATA or KILL connection?
 *
 * protocol handles lost packets by having the sender of the data to use timeout with retransmissions.
If a data packet is lost, the sender eventually times out when an acknowledgment
is not received, and retransmits the packet.
If
an acknowledgment packet is lost, the sender also times out and retransmits the data packet.
In this case the receiver notes from the block number in the data packet that it is a duplicate,
so it ignores the duplicate packet and retransmits the acknowledgment. Most errors other than
timeout cause termination of the packet exchange.
 *
 * Sorcerer's Apprentice BUG:
 * -
 * -
 *
 *
 *
 *
 */
