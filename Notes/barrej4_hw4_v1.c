/* Author: Jessica Marie Barre
   Name: File Sync Utility
   Purpose: A filesync utility that is similar to rsync
   Due: Wednesday, May 3rd
*/

/***** Goals ****

- implement both the server and client portions of this simplifed protocol. 
- All file names should be limited to 255 characters in length and only include 
  upper/lower case letters, numbers, periods, dashes, and underscores. 
  
- On the server, your program should: 
    - call mkdtemp() to create a temporary (and initially empty) directory. 
    - The file .4220_file_list.txt will be populated over time as clients 
    connect and upload fles. 
    - Should the MD5 hash of two fles diﬀer, the most recently modifed file
    should be selected and copied in full to the out-of-date party.
    
- The format of each line in your .4220_file_list.txt file should be the hash, 
four spaces, then file name like so:

b468ef8dfcc96cc15de74496447d7b45 Assignment4.pdf
d41d8cd98f00b204e9800998ecf8427e foo.txt

- Running in the client confguration implicitly uses the current directory. 
- Multiple simultaneous clients will not be tested.

Sample output given below:
./syncr server [port]
[server] Detected different & newer file: Assignment4.pdf
[server] Downloading Assignment4.pdf: a833e3c92bb5c1f03db8d9f771a2e1a2
./syncr client [port]
[client] Detected different & newer file: foobar.txt
[client] Downloading foobar.txt: d41d8cd98f00b204e9800998ecf8427e

Compile with:
gcc -g -Wall -o rsync barrej4_hw4.c -lssl -lcrypto
gcc -g -Wall -o rsync barrej4_hw4.c -lssl -lcrypto

Run with:
./rsync server [port]
AND
./rsync client [port]

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

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif
 
int is_server = 0;
int is_client = 0;

#define BUFFER_SIZE 	 1024
#define FILENAME_LENGTH  255
#define LINE_LENGTH      MD5_DIGEST_LENGTH + FILENAME_LENGTH + 4
#define FILE_LIST		 ".4220_file_list.txt"
#define SERVER_FILE_LIST ".4220_file_list_server.txt"
#define COMMAND_CONTENTS "CONTENTS"
#define COMMAND_PUT 	 "PUT"
#define COMMAND_GET 	 "GET"
#define COMMAND_QUERY	 "QUERY"


/****************** HELPER FUNCTIONS *****************************/

/**
 * remove_file removes a file from the current working directory
 * 
 * @param filename the file to remove
 * @effect deletes filename from the cwd
 */
void remove_file(char * filename) {
	
	int n = remove(filename);
	   
	if(n == 0) printf("File deleted successfully\n");
	else perror("Error: unable to delete the file");
}

/**
 * get_file_list scans the requested directory and populates an
 * array of character pointers with the names of those files.
 * It does not include ".", "..", or any of the .4220_file_list
 * files. 
 * 
 * The list will be sorted in alphabetical order and be
 * used to populate the .4220_file_list, and to compare 
 * between the server/client's lists.
 * 
 * @param dirname the directory to scan 
 * @param num_files a pointer to keep track of the number of files
 *		  that are found
 * @return file_list the list of file names
 */
char ** get_file_list(char * dirname, size_t * num_files) {
	
	struct dirent **dir;
	char ** file_list = (char **) calloc(0, sizeof(char *));
	size_t size = 0;
	
	int n = scandir(dirname, &dir, 0, alphasort);
	fflush(NULL);
	
	if (n < 0) {
		perror("scandir() failed.");
		exit(EXIT_FAILURE);
	}

	if (dir) {
		
		int i = 0;
		
		while (i < n) {

			char * name = dir[i]->d_name;
			
			if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
				//don't add to filelist
			} else if ((strncmp(name, FILE_LIST, strlen(FILE_LIST) - 4)) == 0) {
				// don't add the .4220_file_list to the list either
			} else {
				//add file to return list
				size++;
				file_list = (char **) realloc(file_list, sizeof(char *) * size);
				file_list[size - 1] = (char *) malloc ((strlen(name) + 1) * sizeof(char));
				sprintf(file_list[size-1], "%s", name);
			}
			
			i++;
			
		}
	}
	
	*num_files = size;
	
	return file_list;
}

/**
 * get_MD5 returns the MD% checksum of a file
 * 
 * @param filename the file to be hashed
 * @return the hash of the file
*/
unsigned char * get_MD5(char * filename) {

    FILE * file = fopen(filename, "rb");
    MD5_CTX context;
    unsigned char * hash = (unsigned char *) malloc (MD5_DIGEST_LENGTH * sizeof(unsigned char));
    int bytes;
    unsigned char data[1024];

    if (file == NULL) {
        perror ("file can't be opened.");
        return 0;
    }

    MD5_Init (&context);
    
    while ((bytes = fread (data, sizeof(char), BUFFER_SIZE, file)) != 0)
        MD5_Update (&context, data, bytes);
        
    MD5_Final (hash,&context);

#if 0
    int i;

    for(i = 0; i < MD5_DIGEST_LENGTH; i++) 
    	printf("%02x", hash[i]);
    	
    printf (" %s\n", filename);
#endif
    
    fclose (file);
    
    return hash;
	
}


/** 
 * populate_file_list is called to populate the .4220_file_list
 * files with the correct md5 hashes and
 * 
 * The .4220_file_list.txt will be populated over time as clients 
 * connect and upload fles. 
 * 
 * The format of each line in your .4220_file_list.txt file should be the hash, 
 * four spaces, then file name like so:
 * 
 * b468ef8dfcc96cc15de74496447d7b45 Assignment4.pdf
 * d41d8cd98f00b204e9800998ecf8427e foo.txt   
 */
void populate_file_list(char * filename, char ** list, size_t num_files) {
	
    /* create an empty file to keep track of MD5 hashes - filename list */

	FILE * file = fopen(filename, "w+");
	
	if( file == NULL) {
		perror("fopen() failed");
	}
	

	int i = 0;

	while (i < num_files) {
		
		// CALCULATE HASHCODE OF FILE
		unsigned char * hash = get_MD5(list[i]);

		int len = strlen(list[i]) + MD5_DIGEST_LENGTH + 5;
		char * line = malloc(len * sizeof(char));
	
		sprintf(line, "%s    %s\n", hash, list[i]);
		fputs(line, file);
		
		free(line);
		i++;
	}
	
	fclose(file);
}


/**
 * get_file_size returns the length of the file
 * 
 * @param filename
 * @param file
 * @return the file length
 */
long int get_file_length(char * filename, FILE * file) {
	
	long int file_length;

	fseek(file,0,SEEK_END);
	
	file_length = ftell(file);
	
	rewind(file);
	
	return file_length;
}

/** 
 * open_file opens a file and takes care of error
 * checking as well
 * 
 * @param filename
 * @param mode
 * @return the file pointer
 */
FILE * open_file(char * filename, char * mode) {
	
	FILE * file = fopen(filename , mode);
	
	if( file == NULL) {
		perror("file open() failed.");
		exit(EXIT_FAILURE);
	}
	
	return file;
	
}


/** 
 * read file reads from a file and takes care of error
 * checking as well
 * 
 * @param buffer
 * @param size the size/length of the data to read 
 * @return the file pointer
 */
int read_data(char * buffer, int size, FILE * file) {
	
	int bytes_read = fread(buffer, sizeof(char), size, file);

	if (bytes_read !=  size) {
		perror("fread () failed.");
		exit(EXIT_FAILURE);
	}
	
	buffer[bytes_read] = '\0';
	
	return bytes_read;
}

/** 
 * write file writes to a file and takes care of error
 * checking as well
 * 
 * @param buffer
 * @param size the size/length of the data to write 
 * @return the file pointer
 */
int write_data(char * buffer, int size, FILE * file) {
	
	int bytes_written = fwrite(buffer, sizeof(char), size, file);

	if (bytes_written !=  size) {
		perror("fwrite() failed.");
		exit(EXIT_FAILURE);
	}
	
	return bytes_written;
}

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

/** send_file sends a file to the current working directory of the
 * intended party
 * 
 * If the file is empty an ACK is sent
 * 
 * @param sock the sock fd
 * @param filename the filename
 * @param buffer
 */
void send_file(int sock, char * filename, char * buffer) {
	
	printf("%s sending\n", filename);
	fflush(stdout);

	
	FILE * file = recv_data(filename , "r+");
	long int file_length = get_file_length(filename, file);
	int bytes_read = 0;
	int bytes_remaining = file_length;
	
	sprintf(buffer, "%li", file_length);
	
	/* send the file length */
	send_data(sock, buffer, strlen(buffer));

	/* For file length >  0, send data */
	while (bytes_remaining - BUFFER_SIZE > 0) {
		
		bytes_read = read_data(buffer, BUFFER_SIZE, file);
	
		send_data(sock, buffer, strlen(buffer));
		
		bytes_remaining = bytes_remaining - bytes_read;
	}
	
	if (bytes_remaining > 0) {
		
		bytes_read = read_data(buffer, bytes_remaining, file);
		
		send_data(sock, buffer, strlen(buffer));
		
		bytes_remaining = bytes_remaining - bytes_read;
	}

	fclose(file);
	
	printf("%s sent\n", filename);
		
}

/**
 * recv_file receives the contents of a file into the
 * current working directory.
 * 
 * @param sock
 * @param filename
 * @effect
 * 
 */
void recv_file(int sock, char * buffer, char* filename) {
	
	printf("receiving %s\n", filename);
	FILE * file = recv_data( filename , "w+");
	
	int file_length;
	int bytes_received = 0;
	int bytes_remaining;
	
	/* Get the file length */
	int n = recv( sock, buffer, 10, 0 );
	if ( n < 0 ) {
		printf("n is %d/n", n);
		exit(EXIT_FAILURE);
	}
	
	buffer[n] = '\0';
	
	file_length = atoi(buffer);
	bytes_remaining = file_length;
	
	if (file_length > 0) {
	
		while (bytes_remaining - BUFFER_SIZE > 0) {
			
			/* Receive file contents */
			bytes_received = recv_data(sock, buffer, BUFFER_SIZE);
	
			write_data(buffer, bytes_received, file);
			
		    bytes_remaining = bytes_remaining - bytes_received;
			
		} /* END WHILE */
		
		while (bytes_remaining > 0) {
			
			/* Receive file contents */
			bytes_received = recv_data(sock, buffer, bytes_remaining);
	
			write_data(buffer, bytes_received, file);

		    bytes_remaining = bytes_remaining - bytes_received;

			
		} /* END WHILE */
		
	}
		
	fclose(file);
}


/****************** CLIENT ONLY FUNCTIONS *****************************/

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

	if ( connect( *sock, (struct sockaddr *)&server,
			sizeof( server ) ) < 0 )
	{
		perror( "connect() failed" );
		exit( EXIT_FAILURE );
	}

	printf( "Server address is %s connected to the server \n", inet_ntoa( server.sin_addr ) );
	
}

/** The client calls request_contents to request the contents of the 
   .4220_file_list.txt from the server.
   
   (if it exists) it will be sent from the server to the client. 
*/
void request_contents(int sock, char * buffer) {
	
	sprintf( buffer, "%s\n", COMMAND_CONTENTS); // should i send w/ newline?
	
	int bytes = strlen( buffer );

	int n = send( sock, buffer, bytes, 0 );
	fflush(NULL);

	if ( n != bytes )
	{
		perror( "send() failed" );
		exit( EXIT_FAILURE );
	}
}


/**
 * send_query will be called in the event that two files exist on both the server 
 * and client but their MD5 hashes diﬀer; this call is required to determine 
 * which file is newer (using stat() and mtime). 
 * 
 */
void send_query(char * buffer, char * filename) {
	
	sprintf( buffer, "QUERY %s\n", filename);

	int bytes = strlen( buffer );

	int n = send( sock, buffer, bytes, 0 );
	fflush( NULL );

	if ( n != bytes )
	{
		perror( "send() failed" );
		exit( EXIT_FAILURE );
	}
}

/** 
 * get is required in the event the SERVER HAS a more recently modifed file
 * or a file on the server is not present on the client side.
 * 
 * The client sends a get request for a file, then the server responds
 * by sending the file. Once the file is sent, the client sends an
 * acknowledge letting the server know the transfer is complete.
 * 
 * @param sock 
 * @param filename
 * @effect
 * 
 */
void get_file(int sock, char * filename, char * buffer) {
	
	sprintf(buffer, "%s %s\n", COMMAND_GET, filename);
	
	int bytes = strlen( buffer );

	int n = send( sock, buffer, bytes, 0 );
	fflush(NULL);

	if ( n != bytes )
	{
		perror( "send() failed" );
		exit( EXIT_FAILURE );
	}	
	
	// retrieve the file from the server.
	recv_file(sock, filename, buffer);

}

/**
 * put is required in the event the CLIENT HAS the more recently modifed file,
 * or the server does not have a file on the client side.
 * 
 * The client will send a put request, retrieve an ACK, and then follow by
 * sending the file.
 * 
 * @param sock 
 * @param filename
 * @effect
 */
void put_file(int sock, char * filename, char * buffer) {
	
	sprintf(buffer, "%s %s\n", COMMAND_PUT, filename);
	
	int bytes = strlen(buffer);
	
	send_data(sock, buffer, bytes);
	
   //recv_data(sock, buffer, 3);


	// send the file to the server
	send_file(sock, filename, buffer);	
}

/** 
 * file_sync compares both the server's and client's lists of 
 * hash - filename files and appropriately syncs both sides.
 *  
 * Should the MD5 hash of two fles diﬀer, the most recently modifed file
 * should be selected and copied in FULL to the out-of-date party.
 * 
 * @param client_list the client's file_list
 * @param server_list the server's file_list
 * @param sock the sock fd
 * @param buffer an empty buffer 
 * @effect both the client and server are synced
 */
void file_sync(char * client_list, char * server_list, int sock, char * buffer) {
	
	FILE * client_file = recv_data(client_list, "r+");
	FILE * server_file = recv_data(server_list, "r+");
 
	char * c_line = malloc((LINE_LENGTH+1) * sizeof(char));
	char * s_line = malloc((LINE_LENGTH+1) * sizeof(char));
	
	char * c_hash = malloc((MD5_DIGEST_LENGTH + 1) * sizeof(char));
	char * s_hash = malloc((MD5_DIGEST_LENGTH + 1) * sizeof(char));
	
	char * client_filename = malloc((FILENAME_LENGTH + 1) * sizeof(char));
 	char * server_filename = malloc((FILENAME_LENGTH + 1) * sizeof(char));
	
	char* cli_n = fgets(c_line,LINE_LENGTH, client_file);
	
	sscanf(c_line,"%s    %s",c_hash, client_filename);

	char* ser_n = fgets(s_line,LINE_LENGTH, server_file);
	
	sscanf(s_line,"%s    %s",s_hash,server_filename);

	while(cli_n != NULL && ser_n != NULL) {

		//	printf("Client Hash: \"%s\" name: \"%s\"\n", c_hash, client_filename);
		//	printf("Server Hash: \"%s\" name: \"%s\"\n", s_hash, server_filename);
		
		if(strcmp(client_filename, server_filename) < 0) {

			put_file(sock, client_filename, buffer);

			cli_n = fgets(c_line,LINE_LENGTH, client_file);
			
			sscanf(c_line,"%s    %s",c_hash, client_filename);
	
		} else if (strcmp(client_filename, server_filename) > 0) {

			get_file(sock, server_filename, buffer);

			ser_n = fgets(s_line, LINE_LENGTH, server_file);	
			
			sscanf(s_line,"%s    %s",s_hash,server_filename);
		
		} else { // else (client_filename == server_filename)
		
			if (c_hash != s_hash) {
				// else query for last modified
				// if server last modified get file
				// else put file
			}

			// Move on to the next files
			cli_n = fgets(c_line,LINE_LENGTH, client_file);
			
			sscanf(c_line,"%s    %s",c_hash, client_filename);
			
			ser_n = fgets(s_line,LINE_LENGTH, server_file);
			
			sscanf(s_line,"%s    %s",s_hash,server_filename);
			
		}
	}
  
  /* Since the client still has files that the server has not,
	put them on the server */
  while(cli_n != NULL) {
  	
  	put_file(sock, client_filename, buffer);
  	
  	cli_n = fgets(c_line,LINE_LENGTH, client_file);
  	
  	sscanf(c_line,"%s    %s",c_hash, client_filename);
  }
  
  /* Since the client does not have files that the server has,
     get them from the server */
  while(ser_n != NULL) {
  	
  	get_file(sock, server_filename, buffer);
  	
  	ser_n = fgets(s_line,LINE_LENGTH,server_file);
  	
	sscanf(s_line,"%s    %s",s_hash,server_filename);

  }
  
  free(c_line);
  free(s_line);
  
  free(c_hash);
  free(s_hash);
  
  free(client_filename);
  free(server_filename);
  
  fclose(client_file);
  fclose(server_file);
	
}

/****************** SERVER FUNCTIONS *****************************/

char* create_tempdir(char * temp) {
    
     /* Create the temporary directory */
  char *tmp_dirname = mkdtemp (temp);

  if(tmp_dirname == NULL)
  {
     perror ("tempdir: error: Could not create tmp directory");
     exit (EXIT_FAILURE);
  }

  /* Change directory */
  if (chdir (tmp_dirname) == -1)
  {
     perror ("tempdir: error: ");
     exit (EXIT_FAILURE);
  }
  
    return tmp_dirname;
}

void remove_tempdir(char * tmp_dirname) {
    
      /* Delete the temporary directory */
  char rm_command[26];

  strncpy (rm_command, "rm -rf ", 7 + 1);
  strncat (rm_command, tmp_dirname, strlen (tmp_dirname) + 1);

  if (system (rm_command) == -1)
  {
     perror ("tempdir: error: ");
     exit (EXIT_FAILURE);
  }
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
		

}

/**
 * revieve_commands is a server function to go through
 * the process of receiving GET, PUT, QUERY, CONTENTS and 
 * request from the server and responding appropriately
 * 
 * @param sock
 * @param dirname
 */
void receive_commands(int sock, char * dirname) {

	struct sockaddr_in client;
	int fromlen = sizeof( client );
	
	while ( 1 )	{
		
		int newsock = accept( sock, (struct sockaddr *)&client, (socklen_t*)&fromlen );
		printf("Received incoming connection from: %s\n", inet_ntoa( (struct in_addr)client.sin_addr));

	    int n;
	    char buffer[BUFFER_SIZE];
	    
	    
	    /* get_file instruction */
	    printf("Receiving an instruction....\n"); // debugging

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

				size_t cmd_len = strlen(buffer);
				
		    	if (strncmp(buffer, COMMAND_CONTENTS, 
		    	strlen(COMMAND_CONTENTS)) == 0) {
		    		
		    	    send_file(newsock, FILE_LIST, buffer);

		        } else if (strncmp(buffer, COMMAND_QUERY, 
		    	strlen(COMMAND_QUERY)) == 0) {
		    		
		    		printf("Responding to Query....\n"); //debugging
		    		
		    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_QUERY)) * sizeof(char));
		        	char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));

		    		buffer[0] = '\0';

		    		sscanf(buffer,"%s %s", temp, filename);
		    		
		    		// check timestap (using stat() and mtime). 
		    		// return timestamp
		    		
		    		free(filename);
		    		free(temp);

		    	} else if (strncmp(buffer, COMMAND_GET, 
		    	strlen(COMMAND_GET)) == 0) {
		    		
		    		printf("%s", buffer); //debugging

		    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_PUT)) * sizeof(char));
		    		
		    		char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));
		    		
		    		sscanf(buffer,"%s %s", temp, filename);
		    		
		    		free(temp);
		    		
		    	    buffer[0] = '\0';
		    		
		    		send_file(sock, filename, buffer);
		    		
		    		free(filename);
		    				    	
		    	} else if (strncmp(buffer, COMMAND_PUT, 
		    	strlen(COMMAND_PUT)) == 0) {
		    		
		    		//send_data(sock, "ACK", 3);
		    		
		    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_QUERY)) * sizeof(char));
		    		
		    		char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));
		    		
		    		sscanf(buffer,"%s %s", temp, filename);

		    		printf("%s", buffer);
		    		recv_file(sock, buffer, filename);
		    		printf("%s", buffer);

		    		free(temp);
		    		free(filename);

		    	}
		    	else { 
		    		perror("Received Incorrect Command");
		    	}
		    }
	    	
	    } while (n > 0);
	    
	    close( newsock );

	}
	
}

void receive_commands3(int sock, char * dirname) {

	struct sockaddr_in client;
	int fromlen = sizeof( client );

	int pid;

	while ( 1 )
	{
		int newsock = accept( sock, (struct sockaddr *)&client,
				(socklen_t*)&fromlen );

		printf("Received incoming connection from: %s\n", inet_ntoa( (struct in_addr)client.sin_addr));
     	
		pid = fork();

		if ( pid < 0 )
		{
			perror( "ERROR:fork() failed" );
			exit( EXIT_FAILURE );
		}
		else if ( pid == 0 )
		{
		    
		    int n;
		    char buffer[BUFFER_SIZE];
		    
		    
		    /* Receive instruction */
		    printf("Receiving an instruction....\n"); // debugging
		    	

		    do {
		    	
			    n = recv(newsock, buffer, BUFFER_SIZE, 0);
			    fflush(NULL);
			    
			    if (n < 0) {
			    	perror("recv() failed");
			        return exit(EXIT_FAILURE);

			    } else if (n == 0) {
			    	printf( "SERVER: Rcvd 0 from recv(); closing socket...\n" );

			    } else {
			    	buffer[n] = '\0';
			    	
				    printf("[child %d] Received %s", getpid(), buffer); // debugging
				    		

					size_t cmd_len = strlen(buffer);
					
			    	if (strncmp(buffer, COMMAND_CONTENTS, 
			    	strlen(COMMAND_CONTENTS)) == 0) {
			    		
			    		printf("Sending contents....\n"); //debugging
			    			
			    		buffer[0] = '\0';
			    		
			    	    send_file(newsock, FILE_LIST, buffer);
			    	    	
			        } else if (strncmp(buffer, COMMAND_QUERY, 
			    	strlen(COMMAND_QUERY)) == 0) {
			    		
			    		printf("Responding to Query....\n"); //debugging
			    		
			    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_QUERY)) * sizeof(char));
			        	char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));
	
			    		buffer[0] = '\0';
	
			    		sscanf(buffer,"%s %s", temp, filename);
			    		
			    		// check timestap (using stat() and mtime). 
			    		// return timestamp
			    		
			    		free(filename);
			    		free(temp);
	
			    	} else if (strncmp(buffer, COMMAND_GET, 
			    	strlen(COMMAND_GET)) == 0) {
			    		
			    		printf("%s", buffer); //debugging
	
			    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_PUT)) * sizeof(char));
			    		
			    		char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));
			    		
			    		sscanf(buffer,"%s %s", temp, filename);
			    		
			    		free(temp);
			    		
			    	    buffer[0] = '\0';
			    		
			    		send_file(sock, filename, buffer);
			    		
			    		free(filename);
			    				    	
			    	} else if (strncmp(buffer, COMMAND_PUT, 
			    	strlen(COMMAND_PUT)) == 0) {
			    		
			    		printf("%s", buffer); //debugging
	
			    		char * filename = (char *) malloc( (cmd_len - strlen(COMMAND_QUERY)) * sizeof(char));
			    		
			    		char * temp = (char *) malloc ((cmd_len + 1) * sizeof(char));
			    		
			    		sscanf(buffer,"%s %s", temp, filename);
			    		
			    	//	buffer[0] = '\0';
			    		printf("%s", buffer);
			    		recv_file(sock, buffer, filename);
			    		printf("%s", buffer);
			    		
			    		free(temp);
			    		free(filename);
	
			    	}
			    	else { 
			    		perror("Received Incorrect Command");
			    	}
			    }
		    	
		    } while (n > 0);
		    
		} else /* pid > 0   PARENT */
		{			/* parent simply closes the new client socket (endpoint) */
			wait(NULL);
			close( newsock );
		}
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
	    char temp[] = "/tmp/tmpdir.XXXXXX";
	    char * tmp_dirname = create_tempdir(temp); 
	    printf("the temp dir name is %s\n", tmp_dirname);
	   		

	    start_server(&sock, atoi(argv[2]));
	    
	    /* create an empty file to keep track of MD5 hashes - filename list */
      	fopen(FILE_LIST ,"a");
      	
	    receive_commands(sock, tmp_dirname);
	    
	    remove_tempdir(tmp_dirname);

	} else if (strcasecmp("client", argv[1]) == 0) {
	    
	    is_client = 1;
	    
	    size_t num_files;
	    
	    start_client(&sock, atoi(argv[2]));
	    
	    char ** list = get_file_list(".", &num_files);

#if 0    
	    int i = 0;
	    while (i < num_files) {
	    	printf("File %d is %s\n", i, list[i]);
	    	i++;
	    }
#endif
	   
	    // populate .4220_file_list.txt with the file_list hashes
	    printf("populating file list...\n");
	    populate_file_list(FILE_LIST, list, num_files);
	    printf("populated file list...\n");
	    
	    run_client(sock);
	   
	   remove_file(FILE_LIST);
	   remove_file(SERVER_FILE_LIST);
	   
	} else {
	    perror("Incorrect role input, must be client OR server");
	}
	
	close( sock );
	
	return EXIT_SUCCESS;

}
