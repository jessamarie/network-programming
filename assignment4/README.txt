Author: Jessica Marie Barre
No Partner
Late days used: 2 + 1 extra from prof
Due: Saturday, May 6th


Helpful Remarks:

# compile and run with
$ gcc -g -Wall -o rsync barrej4_client_server.c -lssl -lcrypto
$ ./rsync client [port] or 
$ ./rsync server [port]

# IMPORTANT NOTES:
 - tested w/ .txt files (good)
 - I did not support .pdf files because they hang on the last
    transfer recv/send 
 - if all files fail to sync with the server, restart server and run client  
   again; it usually will succeed.
 - My implementation does not allow the server to know the hash of a file that 
   it doesn't have. It calculates upon updating the file list.
 - The following file/filetypes will be ignored as implemented in the 
   is_valid_file() method:
       *	".",
       *	"..",
       *	".4220_file.list.txt",
       *	".4220_file_list.server.txt",
       *	the [a.out] file, or
       *    .c files
       *    .pdf files

# testing:

I only used .txt files from size 0 to size 760000 to
test this program.

Assume at the beginning of each test run the server is started,
runs the test run and contains no files.

TC1: client A runs and all files get downloaded to the server.
TC2: TC1 + client B (empty directory) runs and downloads all files from server
TC3: TC2 + change some files in client B, run client B - file gets put to server,
     run client A, files get downloade from server