Author: Jessica Marie Barre
No Partner

Helpful remarks:

# IMPORTANT NOTES:
- Occasionally RECV or SEND will fail, it make take a few tries of 
      re running the client to make all the files sync

# compile and run with
gcc -g -Wall -o rsync barrej4_hw4.c -lssl -lcrypto
./rsync client/server [port]

# remarks

# testing methods:


1. run client from /server_dir
      a. sync /client_old files
      b. sync /client_new files
      c. sync /client_none files
      d. sync /client_old files (should have the new ones)