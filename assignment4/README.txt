Author: Jessica Marie Barre
No Partner 
Late days used: 2
Due: Friday, May 5th 


Helpful Remarks:

# compile and run with
gcc -g -Wall -o rsync barrej4_client_server.c -lssl -lcrypto
./rsync client/server [port]

# IMPORTANT NOTES:
 - 
 - The following file/filetypes will be ignored as 
implemented in the is_valid_file() method:
       *	".", 
       *	"..",
       *	".4220_file.list.txt",
       *	".4220_file_list.server.txt",
       *	the [a.out] file, or
       *    .pdf files
       *    .c files
 


# test cases passed:

Assume server starts with no files and is running

TC1: run client that contains all old versions of files

1. run client from /server_dir
      a. sync /client_old files
      b. sync /client_new files
      c. sync /client_none files
      d. sync /client_old files (should have the new ones)

# Sample output given below:

./syncr server [port]
[server] Detected different & newer file: Assignment4.pdf
[server] Downloading Assignment4.pdf: a833e3c92bb5c1f03db8d9f771a2e1a2
./syncr client [port]
[client] Detected different & newer file: foobar.txt
[client] Downloading foobar.txt: d41d8cd98f00b204e9800998ecf8427e