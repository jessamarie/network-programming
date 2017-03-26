# TFTP Server
## Author: Jessica Marie Barre

### Client-Side Testing
tftp 0.0.0.0 port
mode octet
get filename.txt asfilename.txt
put filename.txt asfilename.txt

### Server-Output Example
UDP server at port number [50683] and address 0.0.0.0
Retrieving...
Received 25 bytes
127.0.0.1 PORT 59905: File Transfer Success
Sending...
Sent 70672 bytes
127.0.0.1 PORT 60818: File Transfer Success
