Author: Jessica Marie Barre
No Partner
Late days used: 2
Due: Friday, May 5th


Helpful Remarks:

# compile and run with
gcc -g -Wall -o rsync barrej4_client_server.c -lssl -lcrypto
./rsync client/server [port]

# IMPORTANT NOTES:
 - My implementation does not allow the server to know
 the hash of a file that it doesn't have. It will calculate
 this upon the next update of the file list.
 - The following file/filetypes will be ignored as
implemented in the is_valid_file() method:
       *	".",
       *	"..",
       *	".4220_file.list.txt",
       *	".4220_file_list.server.txt",
       *	the [a.out] file, or
       *    .pdf files
       *    .c files



# testing:

I only used .txt files from size 0 to size 760000 to
test this program.

Assume at the beginning of each test run the server is started,
runs the test run and contains no files.

clients:
      O : client with old files only
      N : client with new files only
      E : client with an empty directory
      M : client with mixed only/new files

Test runs: (run these clients sequentially)
TC1: O (puts all to server) -> N (puts all to server) -> O (gets all from server) -> server exits
TC2:

# Sample output given below:

./syncr server [port]
[server] Detected different & newer file: Assignment4.pdf
[server] Downloading Assignment4.pdf: a833e3c92bb5c1f03db8d9f771a2e1a2
./syncr client [port]
[client] Detected different & newer file: foobar.txt
[client] Downloading foobar.txt: d41d8cd98f00b204e9800998ecf8427e
