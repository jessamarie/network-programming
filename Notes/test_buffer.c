/* Author: Jessica Marie Barre
   Name: File Sync Utility - Server
   Purpose: A filesync utility that is similar to rsync
   Due: Wednesday, May 3rd
*/

/***** Goals ****

- implement a fle syncing utility similar in (reduced)
functionality to rsync. 

- implement both the server and client portions of this simplifed
protocol. 
    - client will be most of the bulk
    - server plays a largely passive role.
- Required commands: contents, query, get, and put. 
    - contents will: 
        - request the contents of the .4220_file_list.txt (if it exists) 
        - be sent from the server to the client. 
    - query will:
        - be called in the event that two fles exist on both the server and 
          client 
         but their MD5 hashes diﬀer; this call is required to determine which 
         file is newer (using stat() and mtime). 
    - get: 
        - required in the event the server has the more recently modifed file
    - put:
        - required in the event the client has the more recently modifed file.
- All fle names should be limited to 255 characters in length and only include 
  upper/lower case letters, numbers, periods, dashes, and underscores. 
  
- On the server, your program should: 
    - call mkdtemp() to create a temporary (and initially empty) directory. 
    - The fle .4220_file_list.txt will be populated over time as clients 
    connect and upload fles. 
    - Should the MD5 hash of two fles diﬀer, the most recently modifed file
    should be selected and copied in full to the out-of-date party.
    
- The format of each line in your .4220_file_list.txt file should be the hash, 
four spaces, then file name like so:

b468ef8dfcc96cc15de74496447d7b45 Assignment4.pdf
d41d8cd98f00b204e9800998ecf8427e foo.txt

- Add server to the command-line to run your code in the server confguration. 
- The client option implies that this should be run in the client confguration. 
- Running in the client confguration implicitly uses the current directory. 
- The second argument should be the port number. 
- Multiple simultaneous clients will not be tested.

Sample output given below:
./syncr server 12345
[server] Detected different & newer file: Assignment4.pdf
[server] Downloading Assignment4.pdf: a833e3c92bb5c1f03db8d9f771a2e1a2
./syncr client 12345
[client] Detected different & newer file: foobar.txt
[client] Downloading foobar.txt: d41d8cd98f00b204e9800998ecf8427e



*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

char * test_static() {
	char * buffer[BUFFER_SIZE];
	sprintf(*buffer, "Hi");
	return *buffer;
}

char * test_malloc() {
	char * buffer = (char *) malloc( 5 * sizeof(char));
	sprintf(buffer, "Hii");
	return buffer;
}

void test_change(char * buffer) {
	sprintf(buffer, "Hiii");
	sprintf(buffer, "Hii agains");
}


int main( int argc, char * argv[] )
{
	char * buffer1;
	char buffer2[BUFFER_SIZE];
	
    //printf("Buff1: %s Length: %zu Size: %lu\n", buffer1, strlen(buffer1), sizeof(buffer1));
    //buffer1 = test_static();
    //printf("Buff1: %s Length: %zu Size: %lu\n", buffer1, strlen(buffer1), sizeof(buffer1));
	
	printf("\n");

	printf("Buff2: %s Length: %zu Size: %lu\n", buffer2, strlen(buffer2), sizeof(buffer2));
	test_change(buffer2);
	printf("Buff2: %s Length: %zu Size: %lu\n", buffer2, strlen(buffer2), sizeof(buffer2));
	buffer2[0] = '\0';
	printf("Buff2: %s Length: %zu Size: %lu\n", buffer2, strlen(buffer2), sizeof(buffer2));

	

	return EXIT_SUCCESS;
}
