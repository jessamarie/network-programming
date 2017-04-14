# TFTP Server
## Author: Jessica Marie Barre

Your name(s)
# Platforms you developed / tested on
Used Atom Text Editor inside an Ubuntu VM

# Difficulties you had while developing this code
- Main issue was in the beginning, I couldn't figure out when to fork
- I had some trouble syncing timeouts with Ack/Data packets.

# Time spent on this assignment
- I spent a few 8hr days over Spring Break. At this point I was about halfway done.
- I've spent a few hours on and off this past week. Finally finished yesterday.

# Anything else you think is worth writing up

## Client-Side Testing
tftp 0.0.0.0 port
mode octet
get filename.txt asfilename.txt
put filename.txt asfilename.txt

## Server-Output Example
UDP server at port number [50683] and address 0.0.0.0
Received 25 bytes
127.0.0.1 PORT 59905: File Transfer Success
Sent 70672 bytes
127.0.0.1 PORT 60818: File Transfer Success
