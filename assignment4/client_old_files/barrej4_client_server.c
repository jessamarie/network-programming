/* Author: Jessica Marie Barre
   Name: File Sync Utility
   Purpose: A filesync utility that is similar to rsync
   Due: Wednesday, May 3rd
*/

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define BUFFER_SIZE 1024

#define BUFFER_SIZE 		 1024
#define FILENAME_LENGTH 	 255
#define LINE_LENGTH     	 MD5_DIGEST_LENGTH + FILENAME_LENGTH + 4
#define FILE_LIST			 ".4220_file_list.txt"
#define SERVER_FILE_LIST	 ".4220_file_list_server.txt"
#define CMD_CONTENTS		 "CONTENTS"
#define CMD_PUT 			 "PUT"
#define CMD_GET 			 "GET"
#define CMD_QUERY			 "QUERY"

#define ERR_MSG_PARAMS  	 "ERROR INVALID PARAMETERS\n"
#define ERR_MSG_BYTES          "ERROR INVALID BYTE RANGE\n"
#define ERR_MSG_NOFILE		 "ERROR NO SUCH FILE\n"
#define ERR_MSG_REQUEST        "ERROR INVALID REQUEST\n"

/** These are for debugging, uncomment to debug **/
//#define DEBUG_SERVER
//#define DEBUG_CLIENT
#define DEBUG_PARSING
#define DEBUG_LIST
//#define DEBUG_SEND
//#define DEBUG_RECV
//#define DEBUG_READ
//#define DEBUG_WRITE
//#define DEBUG_FILE
//#define DEBUG_POPULATE
//#define DEBUG_SYNC
#define DEBUG_GETLINE
#define DEBUG_QUERY

char * tmp_dirname = "";
char * asker = "";
char * responder = "";
char * outfile = "";




/****************** HELPER FUNCTIONS *****************************/

/**
 * remove_file removes a file from the current working directory
 *
 * @param filename the file to remove
 * @effect deletes filename from the cwd
 */
void remove_file(char * filename) {

	int n = remove(filename);

	if(n == 0) {

#ifdef DEBUG_FILE
		printf("[%s] deleted [%s] successfully\n", asker, filename);
		fflush( stdout );
#endif

	} else {
		fprintf(stderr, "[%s] unable to delete [%s]: %s\n", asker, filename, strerror(errno));
	}
}


/**
 * file_exists makes sure a file exists in the CWD
 *
 * @param filename
*/
void file_exists(char * filename) {

	if ( access( filename, F_OK ) != -1 ) {

#ifdef DEBUG_FILE
		printf("[%s] %s exists\n", asker, filename);
		fflush( stdout );
#endif

	} else {

		fprintf(stderr, "[%s] File [%s] does not exist: %s\n", asker, filename, strerror(errno));

	}
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

		fprintf(stderr, "[%s] fopen() failed for [%s] : %s\n", asker, filename, strerror(errno));

		exit(EXIT_FAILURE);
	}

	return file;

}


/**
 * is_valid_file tests that the given parameter does not equal:
 *	".",
 *	"..",
 *	".4220_file.list.txt",
 *	".4220_file_list.server.txt",
 *	the [a.out] file,
 *  .pdf files
 *  .c files
 *
 * @param name, the file name to check
 * @return 1 if the file is ok, and 0 o.w
 *
*/
int is_valid_file(char const * name) {

	return strcmp(name, ".") * strcmp(name, "..") * strcmp(name, FILE_LIST)
	* strcmp(name, SERVER_FILE_LIST) * strcmp(name, outfile) *
	strncmp(name + strlen(name) - 2,  ".c", 2) * strncmp(name + strlen(name) - 4,  ".pdf", 4);
}


/**
 * get_file_list scans the requested directory and populates an
 * array of character pointers with the names of those files.
 * It does not include any of the files listed in the documentation
 * of is_valid_file()
 *
 * The list will be sorted in alphabetical order to facilitate
 * the file_sync. It will also be used to populate the .4220_file_list.
 *
 * @param dirname the directory to scan
 * @param num_files a pointer to keep track of the number of files
 *		  that are found
 * @return file_list the double char array of file names
 */
char ** get_file_list(char * dirname, size_t * num_files) {

	struct dirent **dir;
	char ** file_list;
	int n, i;
	size_t size = 0;


	/* peek into the directory */

	if ((n = scandir(dirname, &dir, 0, alphasort)) < 0) {

		fprintf(stderr, "[%s] scandir() failed: %s\n", asker, strerror(errno));

		exit(EXIT_FAILURE);

	}

	fflush(NULL);

	file_list = (char **) calloc (n - 2, sizeof(char *)); // ignore the '.' and '..'

	char * name = (char *) malloc((FILENAME_LENGTH + 1) * sizeof(char));


	/* scan through the directory and save valid files*/

	i = 0;

	while (i < n) {

		sprintf(name, "%s", dir[i]->d_name );

		int n = is_valid_file(name);

		if (n != 0) {

			file_list[size] = (char *) malloc ((strlen(name) + 1) * sizeof(char));

			sprintf(file_list[size], "%s", name );

			size++;

			fflush( stdout );
		}

		i++;
	}

	free(name);


	*num_files = size;


#ifdef DEBUG_LIST

	 i = 0;

	 while (i < size) {

		printf("[%s] File %d: %s\n", asker, i + 1, file_list[i]);
	    	fflush( stdout );

	    	i++;
	 }

#endif

	return file_list;
}


/**
 * compare_hashes compares two hex strings for
 * equality
 *
 * @param hash1 the first hash to compare
 * @param hash2 the second hash to compare
 * @return 0 if they are equal, o.w a non-zero number
 */
int compare_hashes(char * hash1, char * hash2) {

      int isEqual = memcmp(hash1, hash2, MD5_DIGEST_LENGTH);


#ifdef DEBUG_SYNC
      if (isEqual == 0) {
            printf("They are equal!\n");
      } else {
            printf("They are not equal :( \n");
      }
     fflush(stdout);
#endif


      return isEqual;
}

/**
 * convert_hex_to_string converts a string in
 * hexidecimal representation to a simple
 * character string
 *
 * @param hex the hex to convert
 * @param string the empty string that gets populated
 * @effect convert hex to string representation
 */
void convert_hex_to_string(unsigned char * hex, char * string) {

  int i;

  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {

		char tmp[2];
		tmp[0] = '\0';
		tmp[1] = '\0';

		sprintf(tmp, "%x", hex[i]);
		
		char c = tmp[0];
		int k = 0;

		while ( k < 2 && c != '\0' ) {
		      k++;
		      c = tmp[k];
		}

		sprintf(string + strlen(string), "%s", tmp);
    }
}

/**
 * get_MD5 returns the MD5 checksum of a file
 *
 * @param filename the file to be hashed
 * @return the hash of the file
*/
unsigned char * get_MD5(char * filename) {

    FILE * file = open_file(filename, "rb");

    MD5_CTX context;

    unsigned char * hash = (unsigned char *) malloc (MD5_DIGEST_LENGTH * sizeof(unsigned char));

    int bytes;

    unsigned char data[1024];

    MD5_Init (&context);

    while ((bytes = fread (data, sizeof(char), BUFFER_SIZE, file)) != 0)
        MD5_Update (&context, data, bytes);

    MD5_Final (hash,&context);

#ifdef DEBUG_POPULATE
    int i;
    for(i = 0; i < MD5_DIGEST_LENGTH; i++)
    	printf("%02x", hash[i]);
    printf (" %s\n", filename);
    fflush(stdout);
#endif

    fclose (file);

    return hash;

}


/** put_line puts a line into the .4220_file_list.
 *
 * The format of each line in your .4220_file_list.txt file should be the hash,
 * four spaces, then file name like so:
 *
 * b468ef8dfcc96cc15de74496447d7b45 Assignment4.pdf
 * d41d8cd98f00b204e9800998ecf8427e foo.txt
 *
 * @param file the file fd
 * @param hash the hash string
 * @param filename the filename
 * @effect [HASH]    [Filename] is added to a file
 */
void put_line(FILE * file, unsigned char * hash, char * filename) {

     int i;

      for(i = 0; i < MD5_DIGEST_LENGTH; i++) {

            char tmp[2];
            tmp[0] = '\0';
            tmp[1] = '\0';

            sprintf(tmp, "%x", hash[i]);

            char c = tmp[0];
            int k = 0;
            while ( k < 2 && c != '\0' ) {
                  fputc(c, file); k++;
                  c = tmp[k];
            }

      }

      fwrite("    ", 4, 1, file);

      for(i = 0; i < strlen(filename); i++)
            fputc(filename[i], file);

      fputc('\n', file);
}

/**
 * feed_files is called by populate_file_list() to iterate
 * through an array of files and send them to put_line()
 * one by one.
 *
 * @param filenames the array of filenames
 * @param numfiles
 * @effect the file_list.txt file is generated
*/
void feed_files(char ** filenames, size_t numfiles) {

      int k = 0;

      fflush(stdout);

      FILE * file = open_file(FILE_LIST, "w");

      while (k < numfiles) {

            unsigned char * hash = get_MD5(filenames[k]);

            put_line(file, hash, filenames[k]);

            k++;
      }

      fclose(file);
}


/**
 * populate_file_list is called to populate the .4220_file_list
 * files with the correct md5 hashes. It will be .4220_file_list.txt will
 * be populated over time as clients connect and upload fles.
 *
 * @effect populates the .4220_file_list in the cwd
 */
void populate_file_list() {

	size_t num_files;

	char ** list = get_file_list(".", &num_files);

#ifdef DEBUG_POPULATE
printf("Populating file...\n");
fflush (stdout);
#endif

	feed_files(list, num_files);

	int i;
	for (i = 0; i < num_files; i++) {
		free(list[i]);
	}

	free(list);


#ifdef DEBUG_POPULATE
      printf("[%s] populated .4220_file_list...\n", asker);
      fflush( stdout );
#endif
}


/**
 * get_file_size returns the length of the file
 *
 * @param filename
 * @return the file length
 */
long int get_file_length(char * filename) {

	FILE * file;

	long int file_length;

	file = open_file(filename, "r+");

	fseek(file,0,SEEK_END);

	file_length = ftell(file);

	rewind(file);

	fclose(file);

	return file_length;
}


/**
 * get_mtime determines when a file was last modified.
 *
 *  this call is required to determine which file is newer
 * (using stat() and mtime).
 *
 * @param filename
 * @return the last modified time
 */
time_t get_mtime(char * filename) {

	struct stat attr;

	if (stat(filename, &attr) == 0) {

#ifdef DEBUG_QUERY
	printf("[%s] %s last modified time was %s", asker, filename, ctime(&attr.st_mtime));
	fflush( stdout );
#endif

	return attr.st_mtime;
    }

    return 0;
}


/**
 * compare_mtimes compares the two last modified times.
 *
 * @param t1 the client's time
 * @param t2 the server's time
 * @return -1 if the client is older, 1 is the client is younger,
 *		and 0 if they are both the same age
 */
int compare_mtimes(time_t t1, time_t t2) {
	
#ifdef DEBUG_QUERY
	
	char buff1[20], buff2[20];

	strftime(buff1, 20, "%Y-%m-%d %H:%M:%S", localtime(&t1));
	strftime(buff2, 20, "%Y-%m-%d %H:%M:%S", localtime(&t2));

	printf("[%s] %s - %s\n", asker, buff1, buff2);
	fflush( stdout );
#endif

	int t = difftime(t1,t2);

#ifdef DEBUG_QUERY
	printf("[%s] mtime comparison returned %d\n", asker, t);
	fflush( stdout );
#endif

    return t;
}


/**
 * is_digit checks whether the current digit is a number.
 *
 * @param s the char to check
 * @return 1 if digit, o.w 0
 */
int is_digit (const char * s) {

	if (s == NULL || *s == '\0' || isspace(*s))
		return 0;

	char * p;

	strtod (s, &p);

	return *p == '\0';
}


/**
 * read_file reads from a file and takes care of error
 * checking as well.
 *
 * @param buffer
 * @param size the size/length of the data to read
 * @param file the file pointer
 * @return the number of bytes read
 */
int read_file(char * buffer, int size, FILE * file) {

	int bytes_read = fread(buffer, sizeof(char), size, file);

	if (bytes_read !=  size) {
		fprintf(stderr, "[%s] failed to fread(): %s\n", asker, strerror(errno));
		exit(EXIT_FAILURE);
	}

	buffer[bytes_read] = '\0';

	return bytes_read;
}


/**
 * write file writes to a file and takes care of error
 * checking as well.
 *
 * @param buffer
 * @param size the size/length of the data to write
 * @param file the file pointer
 * @return the number of bytes written
 */
int write_file(char * buffer, int size, FILE * file) {

	int bytes_written = fwrite(buffer, sizeof(char), size, file);

	if (bytes_written !=  size) {
		fprintf(stderr, "[%s] failed to fwrite(): %s\n", asker, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return bytes_written;
}


/**
 * create_tempdir creates a temporary directory on the
 * server.
 *
 * @effect creates a temperary dir named /tmp/tmpdir.XXXXXX
 * @return the dirname
 */
char* create_tempdir() {

	char temp[] = "/tmp/tmpdir.XXXXXX";

	char * dirname = mkdtemp (temp);

	if(dirname == NULL) {
		fprintf(stderr, "[%s] could not create the temp directory: %s\n", asker, strerror(errno));
		exit (EXIT_FAILURE);
	}

	/* Change directory */
	if (chdir (dirname) == -1) {
		fprintf(stderr, "[%s] could not move to the temp directory: %s\n", asker, strerror(errno));
		exit (EXIT_FAILURE);
	}

	return dirname;
}


/**
 * remove_tempdir removes a temporary directory on the
 * server.
 *
 * @effect removes the /tmp/tmpdir.XXXXXX directory
 */
void remove_tempdir() {

	/* Delete the temporary directory */

	char rm_command[26];

	strncpy (rm_command, "rm -rf ", 7 + 1);

	strncat (rm_command, tmp_dirname, strlen (tmp_dirname) + 1);

	if (system (rm_command) == -1) {
		fprintf(stderr, "[%s] could not remove the temp directory: %s\n", asker, strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/**
 * split_at_delim splits a string once at the first occurance of the
 * desired delimeter.
 *
 * @param str the string to separate
 * @param delim the delimeter to separate by
 * @param numtokens the number of tokens (<=2)
 * @effect numtokens equals 1 or 2 if data occurs after the newline
 * @return the str separated at the first occurance of the delimeter
 **/
char **split_at_delim(const char* str, const char* delim, size_t* numtokens) {

	char *s = strdup(str);
	size_t tokens_alloc = 2;
	size_t tokens_used = 0;

	char **tokens = calloc(tokens_alloc, sizeof(char*));

	char *token, *rest = s;


	/* Seperate the string @ the first delim */

	if ((token = strsep(&rest, delim)) != NULL) {

		if(strcmp(token, "")) {

			tokens[tokens_used++] = strdup(token);

#ifdef DEBUG_PARSING
printf("[%s] Line: (%.40s)\n", asker, tokens[tokens_used - 1]);
fflush( stdout );
#endif

			if(strcmp(rest, "") != 0) {

				tokens[tokens_used++] = strdup(rest);

#ifdef DEBUG_PARSING
    printf("[%s] Line: (%.40s)\n", asker,
    tokens[tokens_used - 1]);
    fflush( stdout );
#endif

			}
		}
	}

#ifdef DEBUG_PARSING
      printf("[%s] Numtokens: %zu\n", asker, tokens_used);
      fflush( stdout );
#endif

	/* Reallocate based on the # of tokens */

	if (tokens_used == 0) {

		free(tokens);

		tokens = NULL;

	} else if (tokens_used < tokens_alloc) {

		tokens = realloc(tokens, tokens_used * sizeof(char*));
	}


	*numtokens = tokens_used;

	free(s);

	return tokens;
}


/********************** FILE/MESSAGE RECEIVE/SEND **************************/

/**
 * recv_msg uses recv() to get a message from over a client/server
 * connection and checks for errors.
 *
 * @param sock the sock fd
 * @param buffer
 * @return the number of bytes receieved
 */
int recv_msg( int sock, char * buffer ) {

	int n = recv( sock, buffer, BUFFER_SIZE, 0 );
	fflush (NULL);

	if ( n < 0 ) {

		fprintf(stderr, "[%s] recv() failed: %s\n", asker, strerror(errno));

	} else if ( n == 0 ) {

      #ifdef DEBUG_RECV
      		printf( "[%s] disconnected: Rcvd 0 from recv()\n", asker);
      		fflush( stdout );
      #endif

	} else {

		buffer[n] = '\0';

#ifdef DEBUG_RECV
		printf("[%s] received %d bytes of data\n", asker, n);
		fflush( stdout );
#endif

#ifdef DEBUG_RECV
		printf("[%s] RCVD [%.40s]...\n", asker, buffer);
		fflush( stdout );
#endif

	}

	return n;
}


/**
 * send_msg uses send() to send data over a connection
 * and checks that the size of the data is exactly
 * as given.
 *
 * @paraam sock the sock fd
 * @param buffer
 * @param size the size to be sent
 * @return the number of bytes sent
*/
int send_msg( int sock, char * buffer, int bytes ) {

	int n = send(sock, buffer, bytes, 0 );
	fflush(NULL);

	if ( n != bytes ) {

		fprintf(stderr, "[%s] send() failed:%s\n:", asker, strerror(errno));

		exit( EXIT_FAILURE );

	} else {

#ifdef DEBUG_SEND
      	printf("[%s] Sent %d of %d bytes of data\n", asker, n, bytes);
	      fflush( stdout );
#endif

#ifdef DEBUG_SEND
		printf("[%s] SENT [%.40s]...\n", asker, buffer);
		fflush( stdout );
#endif

	}

	return n;
}


/**
 * send_err_msg uses send() to send an error message over a connection
 * and checks that the size of the data is exactly as given.
 *
 * @paraam sock the sock fd
 * @param buffer
 */
void send_err_msg( int sock, char * err ) {

	send_msg(sock, err, strlen(err));

}


/**
 * recv_file receives the contents of a file into the
 * current working directory.
 *
 * @param sock the sock fd
 * @param buffer
 * @param filename
 * @param length the length of the data to recv
 * @effect a file is updated/create in the cwd
 */
 void recv_file(int sock, char * data, char * filename, int length) {

	/* store any data after the '\n' character currently
	 * in the buffer */

	FILE * file = open_file( filename , "wb");

	/* If the length is less than 1 then there is nothing to store
		so, leave the file empty */

	if (length < 1) {

	      fclose(file);

		send_msg(sock, "ACK\n", 4);

		return;
	}

	write_file(data, strlen(data), file);

#ifdef DEBUG_WRITE
	printf("[%s] initially wrote (%.40s)...\n", asker, data);
	fflush( stdout );
#endif

	/* while bytes rcvd/stored < expected length */

	int bytes_written = strlen(data);

	while (bytes_written < length) {

	    	char buffer[BUFFER_SIZE];

		/* read more bytes */

		int n = recv_msg(sock, buffer);

		if (n > 0) {

			/* write more bytes */

			write_file(buffer, n, file);

			bytes_written = bytes_written + n;

		} else {
			break; /* don't get stuck if recv 0 */
		}

	} /* done */

#ifdef DEBUG_WRITE
	printf("[%s] finally wrote %d bytes of %d\n", asker, bytes_written, length);
	fflush( stdout );
#endif

	if(bytes_written != length) {

		fclose(file);

		send_err_msg(sock, ERR_MSG_REQUEST);

		return;
	}


	fclose(file);

#ifdef DEBUG_WRITE

	printf( "[%s] Stored file \"%s\" (%d bytes)\n", asker, filename, bytes_written);
	fflush( stdout );

#endif

	send_msg(sock, "ACK\n", 4);

 }


/**
 * send_file sends a file to the cwd of the intended party
 *
 * If the file is empty an ACK is sent
 *
 * @param sock the sock fd
 * @param filename the filename
 * @param buffer
 */
void send_file(int newsock, char * filename, long int file_length, char * buffer) {

	file_exists(filename);

	FILE * fd = open_file(filename, "rb");

	long int length = file_length;
	int buff_len = strlen( buffer );
	int bytes_read, bytes_sent;
	int bytes = buff_len + length;

	if ( bytes <= BUFFER_SIZE )
	{
		bytes_read = fread(buffer + buff_len, sizeof(char), length, fd);

		if ( bytes_read == -1 )
		{
			perror( "read() failed" );
			exit( EXIT_FAILURE );
		}

		fclose( fd );

		bytes_sent = send_msg(newsock, buffer, bytes);

	} else {

		bytes_sent = send_msg(newsock, buffer, buff_len);

		while ( length > 0 ) {

			bytes_read = fread(buffer, sizeof(char), ( length < BUFFER_SIZE ? length : BUFFER_SIZE) , fd);

			if ( bytes_read == -1 )
			{
				perror( "read() failed" );
				exit( EXIT_FAILURE );
			}

			length -= bytes_read;

			bytes_sent += send_msg(newsock, buffer, bytes_read);
		}

		fclose( fd );
	}


#ifdef DEBUG_READ
	printf("[%s] Finally sent %d out of %li bytes of \"%s\"\n",
	asker, bytes_sent - buff_len, file_length, filename);
	fflush(stdout);
#endif

}


/********************** SERVER RESPONSES **************************/

/**
 * send_query sends the mtime and stat of a file to the client
 *
 * @param sock the sockfd
 * @param buffer
 * @param mtime the last modified time in time_t
 * @effect ACK [mtime] is sent to the client
*/
void respond_query( int sock, time_t mtime) {

	char * buffer = (char *) malloc ((BUFFER_SIZE + 1) * sizeof(char));

	char time[20];
	strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&mtime));
	printf("[server] sending %s\n", time);
	//sprintf( buffer, "ACK %s\n", ctime(&mtime));
    sprintf(buffer, "ACK %s\n", time);


	send_msg(sock, buffer, strlen(buffer));

	free(buffer);
}


/********************** CLIENT REQUESTS **************************/

/**
 * send_put sends the contents of a file to the server.
 *
 * put is required in the event that the:
 * - CLIENT HAS the more recently modifed file,
 * - server does not have the file
 *
 * @param sock
 * @param filename
 * @param length the length of the file to send
 * @param buffer
 * @effect PUT [filename] [length] [file contents] is sent to server
 */
void send_put( int sock, char * filename, long int length, char * unused ) {
      char buffer[BUFFER_SIZE];

	sprintf( buffer, "PUT %s %li\n", filename, length );

	send_file(sock, filename, length, buffer);
}

/**
 * send_get sends a request for a file to the server
 *
 * get is required in the event that the:
 * - SERVER HAS a more recently modifed file
 * - client does not have the file
 *
 * The client sends a get request for a file, then the server responds
 * by sending the file. Once the file is sent, the client sends an
 * acknowledge letting the server know the transfer is complete.
 *
 * @param sock
 * @param filename
 * @param buffer
 * @effect GET [filename] is sent to the server
 */
void send_get( int sock, char * filename, char * buffer ) {

	sprintf( buffer, "GET %s\n", filename);

	int bytes = strlen( buffer );

	send_msg(sock, buffer, bytes);
}


/**
 * recv_status recvs the mtime and stat of a file
 * from the server.
 *
 * Format: ACK [mtime] [stat]
 *
 * @param sock
 * @param buffer
 */
void recv_status( int sock, char * buffer, time_t * mtime ) {

      int n = recv_msg(sock, buffer);

      if (n > 0) {

		int hh, mm, ss, y, m, d;

            struct tm when = {0};
            
            sscanf(buffer, "ACK %d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss);

			when.tm_year = y - 1900;
			when.tm_mon = m - 1;
			when.tm_mday = d;
            when.tm_hour = hh;
            when.tm_min = mm;
            when.tm_sec = ss;
            
            *mtime = mktime(&when);
      }
}


/**
 * recv_size recvs a message and breaks it into the file length and the first
 * part of the contents to read.
 *
 * Receive: ACK [file_length] [file_contents]
 *
 * @param sock
 * @param buffer
 * @return the number of bytes receieved
 */
int recv_size( int sock, char * data ) {

      char buffer[BUFFER_SIZE];
      int length = 0;

      int n = recv_msg(sock, buffer);

      if (n > 0) {

            size_t numlines;
            char ** lines = split_at_delim(buffer, "\n", &numlines);

		/* Get the length of the file */

		char tmp[n - 3];

		sscanf(buffer, "ACK %s", tmp);

		length = atoi(tmp);


		/* if length < 0 there is no data to receive */

		if (length <= 0) {
			return 0;
		}

		/* if we make it here, then numlines should equal 1 or 2
		depending on recv */

		if(numlines == 2) {
			strcpy(data, lines[1]);
		} else {
		      data[0] = '\0';
		}

	}

	return length;
}


/**
 * send_query sends a query request to the server
 *
 * query is required in the event that two files exist on both the server
 * and client but their MD5 hashes diﬀer
 *
 * @param sock
 * @param buffer
 * @param filename
 * @effect QUERY [filename] is sent to the server
 */
void send_query(int sock, char * buffer, char * filename) {

	sprintf( buffer, "QUERY %s\n", filename);

	int bytes = strlen( buffer );

	send_msg(sock, buffer, bytes);
}


/**
 * send_contents sends a request for the server's .4220_file_list.txt file
 *
 * (if it exists) it will be sent from the server to the client.
 *
 * @param sock
 * @param buffer
 * @effect CONTENTS is sent to the server
 */
void send_contents( int sock, char * buffer )
{
	sprintf( buffer, "CONTENTS\n" );

	int bytes = strlen( buffer );

	send_msg(sock, buffer, bytes);
}


/**
 * request_contents sends for and retrieves the .4220_file_list
 *
 * @param sock
 * @param buffer
 * @effect .4220_file_list_server.txt file is rcvd
 */
void request_contents(int sock, char * unused) {

 	char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

	send_contents(sock, buffer);

	int length = recv_size(sock, buffer);

	recv_file(sock, buffer, SERVER_FILE_LIST, length);

	file_exists(SERVER_FILE_LIST);

	free(buffer);
}


 /**
 * request_get sends for and retrieves a file from the server
 *
 * @param sock
 * @param buffer
 * @param filename
 * @effect filename is rcvd
 */
 void request_get(int sock, char * unused, char * filename) {

 	char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

	send_get(sock, filename, buffer);

	int length = recv_size(sock, buffer);

	recv_file(sock, buffer, filename, length);

	file_exists(filename);

	free(buffer);
 }


 /**
 * request_put sends a file to the server and receives an ACK
 *
 * (if it exists) it will be sent from the server to the client.
 *
 * @param sock
 * @param buffer
 * @param filename
 * @effect filename is sent and ACK is rcvd
 */
 void request_put(int sock, char * unused, char * filename) {

 	char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

 	long int length = get_file_length(filename);

 	send_put(sock, filename, length, buffer);

 	recv_msg(sock, buffer);  /* ACK\n */

 	free(buffer);
 }


/**
 * request_query requests a query and recieves the answer
 *
 * @param sock
 * @param buffer
 * @param filename
 * @effect the result of the server and clients [filename] mtimes compared
 */
 int request_query(int sock, char * unused, char * filename) {

  	char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

	time_t server_mtime;
	time_t client_mtime = get_mtime(filename);

 	send_query(sock, buffer, filename);

 	recv_status(sock, buffer, &server_mtime);

 	free(buffer);

 	return compare_mtimes(client_mtime, server_mtime);

 }


int get_line(FILE * fd, char * hash, char * filename) {

	char c;
    unsigned char u;
    int i = 0, w;

    /* Check for end of file */

	if ((w = fgetc(fd)) == EOF) {

 #ifdef DEBUG_GETLINE
 printf("[%s] No more lines to read.\n", asker);
 fflush( stdout );
 #endif
      	return w;

	}

	/* If we made it here, there is/are still line(s) to read */
	hash[i] = (char) w;

#ifdef DEBUG_GETLINE
printf("Reading: %c", hash[i]);
fflush( stdout );
#endif

	/* get the hex string */

	while ((u = fgetc(fd)) != ' ') {

		hash[i] = u;

		i++;

#ifdef DEBUG_GETLINE
printf("%c", u);
fflush( stdout );
#endif

	}

	hash[i] = '\0';


	/* skip spaces */

	for(i = 0; i < 3; i++) {

	    c = fgetc(fd);

#ifdef DEBUG_GETLINE
printf("%c", c);
fflush( stdout );
#endif

	}

	/* get the filename */

	for(i = 0; i < 255; i++) {

	    if ((c = fgetc(fd)) == '\n') break;

	    filename[i] = c;


#ifdef DEBUG_GETLINE
printf("%c", c);
fflush( stdout );
#endif

	}

#ifdef DEBUG_GETLINE
printf("\n");
fflush( stdout );
#endif


	filename[i] = '\0';

	return c;
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
void file_sync(int sock) {

    char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

	/* Define variables for client/server files */

	FILE * client_file = open_file(FILE_LIST, "r+");
	FILE * server_file = open_file(SERVER_FILE_LIST, "r+");

	char * c_hex = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));
	char * s_hex = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));
	char * client_filename = (char * ) malloc((FILENAME_LENGTH + 1) * sizeof(char));
 	char * server_filename = (char * ) malloc((FILENAME_LENGTH + 1) * sizeof(char));

	/* Get the first lines in each file */

	char cli_n = get_line(client_file, c_hex, client_filename);

	char ser_n = get_line(server_file, s_hex, server_filename);


	/* Run a "merge-sort" algorithm to the server and client lists */

	while(cli_n != EOF && ser_n != EOF) {

		if(strcmp(client_filename, server_filename) < 0) {

			/* The file doesn't exit on the server, PUT and
			get the next line for the client */

			request_put(sock, buffer, client_filename);

			cli_n = get_line(client_file, c_hex, client_filename);

		} else if (strcmp(client_filename, server_filename) > 0) {

			/* The file doesn't exit on the client, GET and get the next line
			for the server */
			
			printf("[%s] Detected different & newer file: %s\n", asker, server_filename);

			request_get(sock, buffer, server_filename);
			
			printf("[%s] Downloading %s: %s\n", asker, server_filename, s_hex);

			ser_n = get_line(server_file, s_hex, server_filename);

		} else { // else (client_filename == server_filename)

			/* Since both files exist, check the hashes */

			int d = compare_hashes(c_hex, s_hex);

			if (d != 0) {

				/* Send query for last modified file */

				int last = request_query(sock, buffer, client_filename);

				if (last < 0 ) {

					/* server has newer */
					
					printf("[%s] Detected different & newer file: %s\n", asker, server_filename);

					request_get(sock, buffer, server_filename);
					
					printf("[%s] Downloading %s: %s\n", asker, server_filename, s_hex);


				} else {

					/* client has newer */

					request_put(sock, buffer, client_filename);
				}
			}


			/* Get the next lines in each file */

		cli_n = get_line(client_file, c_hex, client_filename);

		ser_n = get_line(server_file, s_hex, server_filename);

		}
	}


  /* Since the client still has files that the server doesn't,
	put them on the server */

  while(cli_n != EOF) {

	request_put(sock, buffer, client_filename);

	cli_n = get_line(client_file, c_hex, client_filename);

  }


  /* Since the client doesn't have files that the server has,
     get them from the server */

  while(ser_n != EOF) {
  	
  	printf("[%s] Detected different & newer file: %s\n", asker, server_filename);

  	request_get(sock, buffer, server_filename);
  
  	printf("[%s] Downloading %s: %s\n", asker, server_filename, s_hex);

	ser_n = get_line(server_file, s_hex, server_filename);

  }

#ifdef DEBUG_SYNC
	printf("[CLIENT] Sync finished....\n");
	fflush( stdout );
#endif

  /* Repopulate the client-side file */

  populate_file_list();

  /* free memory */

  fclose(client_file);
  fclose(server_file);

  free(c_hex);
  free(s_hex);
  free(client_filename);
  free(server_filename);
  free(buffer);

}


/****************** Run Server/Client  ********************/

/**
 * run_client runs the job of the client, which is to:
 * - populate it's file_list
 * - request contents from the server
 * - sync with the server
 *
 * @param newsock
 * @param lines
 * @param numparts
 */
void run_client(int sock) {

    char * buffer = (char * ) malloc((BUFFER_SIZE + 1) * sizeof(char));

	populate_file_list();

	request_contents(sock, buffer);

	file_sync(sock);

	remove_file(FILE_LIST);
	remove_file(SERVER_FILE_LIST);
}


/**
 * start_client starts the client
 *
 * @param sock
 * @param port
 */
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

#ifdef DEBUG_CLIENT
	printf( "Server address is %s connected to the server \n", inet_ntoa( server.sin_addr ) );
	fflush( stdout );
#endif

}


/**
 * do_command carries out the client's request.
 *
 * A request is valid if it begins with PUT, GET,
 * QUERY, or, CONTENTS
 *
 * @param newsock the client sock
 * @param lines the command line [and possibly some data attached]
 */
void do_command(int newsock, char ** lines) {

	char * filename = (char *) malloc ((FILENAME_LENGTH + 1) * sizeof(char));
	char * buffer = (char *) malloc ((BUFFER_SIZE + 1) * sizeof(char));
	long int length = 0;

	if (strncmp(lines[0], CMD_PUT, 3) == 0) {

		char tmp_length[10];

		if(sscanf(lines[0], "PUT %s %s", filename, tmp_length) != 2 || !is_digit(tmp_length)) {
			send_err_msg(newsock, ERR_MSG_PARAMS);
			return;
		}

		printf("[%s] Detected different & newer file: %s\n", asker, filename);
		fflush(stdout);

		/* Recv the file  */

		length = atoi(tmp_length);

		if (length > 0) {
			recv_file(newsock, lines[1], filename, length);
		} else {
			recv_file(newsock, "", filename, length);
		}

		unsigned char * hex = get_MD5(filename);
		convert_hex_to_string(hex, buffer);


	  	
		printf("[%s] Downloaded %s: ", asker, filename);
		
		// TODO: FIX BUFFER, AFTER RETURN, IT IS BAD - mby return from convert?
		
		int i;
		for(i=0; i < strlen(buffer); i++)
	    	printf("%c", buffer[i]);
	
	  	printf("\n");
		fflush(stdout);

	} else if (strncmp(lines[0], CMD_GET, 3) == 0) {

		if(sscanf(lines[0], "GET %s", filename) != 1) {
			send_err_msg(newsock, ERR_MSG_PARAMS);
			return;
		}

		/* Send the requested file: ACK [length]\n[data] */

		length = get_file_length(filename);

		sprintf(buffer, "ACK %li\n", length);

		send_file(newsock, filename, length, buffer);

		recv_msg(newsock, buffer); /* ACK */

	} else if (strncmp(lines[0], CMD_QUERY, 5) == 0) {

		if(sscanf(lines[0], "QUERY %s", filename) != 1) {
			send_err_msg(newsock, ERR_MSG_PARAMS);
			return;
		}

		 /* Send the last mtime: ACK [mtime] */

		 time_t mtime;

		 mtime = get_mtime(filename);

		 respond_query(newsock, mtime);

	} else if (strncmp(lines[0], CMD_CONTENTS, 8) == 0) {

		if(strlen(lines[0]) != 8) {
			send_err_msg(newsock, ERR_MSG_PARAMS);
			return;
		}

		/* Send the file_list: ACK [length]\n[data] */

		length = get_file_length(FILE_LIST);

		sprintf(buffer, "ACK %li\n", length);

		send_file(newsock, FILE_LIST, length, buffer);

		/* recv an ACK if the client rcved it.  */

		recv_msg(newsock, buffer); /* ACK */


	} else if (strncmp(lines[0], "ACK", 3) == 0) {
		// do nothing just an ack
	} else {

		send_err_msg(newsock, ERR_MSG_PARAMS);

		fprintf(stderr, "%s\n", ERR_MSG_PARAMS);

		exit(EXIT_FAILURE);
	}


	free(filename);
	free(buffer);
}


/**
 * runs_server runs the server for the individual client
 *
 * @param newsock
 * @param client
 */
void run_server(int newsock, const struct sockaddr_in* client) {

	int n;
	char buffer[BUFFER_SIZE];

	/* Receive instruction */

	do {

		buffer[0] = '\0';

		n = recv_msg(newsock, buffer);

		if (n > 0) {

			buffer[n] = '\0';

			/* Split at newline */

			char ** lines;
			size_t numlines;

			lines = split_at_delim(buffer, "\n", &numlines);

#ifdef DEBUG_SERVER
			printf("[%s] Command Received: \"%s\"\n", asker, lines[0]);
			fflush( stdout );
#endif

			/* Carry out instruction **/

			do_command(newsock, lines);

			/* free space */

			int i;
			for (i = 0; i < numlines; i++)
			  free(lines[i]);

			if (lines != NULL)
				free(lines);

		}


	} while (n > 0);

	close(newsock);
#ifdef DEBUG_SERVER
	printf("[%s] Client disconnected\n", asker);
	fflush( stdout );
#endif
	exit(EXIT_SUCCESS); /* client terminates here! */
}


/**
 * start_server starts the server
 *
 * @param port
 */
int start_server(unsigned short port) {

    	/* socket structures */

    	struct sockaddr_in server;

    	server.sin_family = PF_INET;
    	server.sin_addr.s_addr = INADDR_ANY;


	/* Create the listener socket as TCP socket */

	int sd = socket(PF_INET, SOCK_STREAM, 0);

	if (sd < 0) {

		perror("ERROR: Could not create socket.");
		exit(EXIT_FAILURE);

	}

	server.sin_port = htons(port);
	int len = sizeof( server );

	if (bind(sd, (struct sockaddr*) &server, len) < 0) {

		perror("Error: bind() failed.");
		exit(EXIT_FAILURE);
	}

	listen(sd, 5); /* 5 is the max number of waiting clients */

#ifdef DEBUG_SERVER
	printf("Started server; listening on port: %d\n", port);
	fflush( stdout );
#endif

	return sd;
}



/**
 * MAIN ENTRY POINT TO PROGRAM
 */
int main( int argc, char **argv){

	/** Check for correct number of arguments and exit
      if incorrect **/

	if ( argc != 3 )
	{
		fprintf( stderr, "ERROR: Invalid arguments\n" );
		fprintf( stderr, "USAGE: %s <client/server> <listener-port>\n", argv[0] );
		return EXIT_FAILURE;
	}

	outfile = (char *) malloc ((strlen(argv[0])) * sizeof(char));

	strncpy(outfile, argv[0] + 2, strlen(argv[0]) - 2);

	asker = argv[1];


	/* start the client or server */

	if (strcasecmp("server", asker) == 0) {

		responder = "client";

    	unsigned short port =  atoi(argv[2]); /* use port = 8127 for testing */


    	/* Create the listener socket as TCP socket */

    	int sd = start_server(port);

    	tmp_dirname = create_tempdir(); 	/* Create the directory for storage */

        /* create an empty file to keep track of MD5 hashes - filename list */
    	populate_file_list();

    	struct sockaddr_in client;
    	int fromlen = sizeof( client );

    	int pid;

    	while ( 1 ) {

    		int newsock = accept( sd, (struct sockaddr *)&client,
    				(socklen_t*)&fromlen );
#ifdef DEBUG_SERVER
    		printf("Received incoming connection from: %s\n", inet_ntoa( (struct in_addr)client.sin_addr));
    		fflush( stdout );
#endif

    		/* handle new socket in a client process,
    	       allowing the parent process to immediately go
    	       back to the accept() call */
    		pid = fork();

    		if ( pid < 0 )
    		{
    			perror( "ERROR:fork() failed" );
    			exit( EXIT_FAILURE );
    		}
    		else if ( pid == 0 )
    		{
    			run_server(newsock, &client); /* Child code */
    		}
    		else /* pid > 0   PARENT */
    		{
    			/* parent simply closes the new client socket (endpoint) */

    			wait(NULL);

    			close( newsock );

    			populate_file_list(); /* Repopulates the .4220.file_list */
    		}
    	}

        remove_tempdir(tmp_dirname);
        close( sd );

	} else if (strcasecmp("client", asker) == 0) {

		responder = "server";

		int sock;

		start_client(&sock, atoi(argv[2]));

		run_client( sock );

		close( sock );

	} else {
	    perror("Incorrect argument: enter 'client' OR 'server' Only");
	}

	return EXIT_SUCCESS;

}
