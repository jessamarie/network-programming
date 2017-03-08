/** 
Author: Jessica Marie Barre
Due Date: Sunday, March 19, 11:59:59 PM
Name: TFTP Server
Purpose:

Implement a TFTP server according to RFC 1350.

Your server should:
- be able to support multiple connections at the same time by calling the 
  fork() system
- You MUST support the “octet” mode. 
- You should not implement the “mail” mode or the “netascii” mode.
- Upon not receiving data for 1 second, your sender should 
	- retransmit its last packet. 
	- similarly, if you have not heard from the other party for 10 seconds, 
	  you should abort the connection.
- Take care to not allow the Sorcerer’s Apprentice Syndrome which should 
  have been covered in Short Lecture 11. 
- Additionally, we will only be testing files smaller than 32 MB.

- Be sure when testing using the tftp Linux client (for example) to set the 
  mode to binary.
- SIGALRM is discussed in our textbook (Chapter 14) which may be useful 
  when implementing timeouts.
- Please include a README.txt file which should include your name, 
  the name of your partner, and any helpful remarks for the grader.
- Please make sure to get started early!


*/