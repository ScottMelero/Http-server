A simple, single threaded HTTP server written in C.

How to Run: 
- Compile the program by running the make command
- execute the server binary file with the following command 
    - ./httpserver address port#
    - be sure to include a valid address. If no 
        port # is specified, it will default to 80
    - If the port# < 3000, you will need to preface the above run command with Sudo
        sudo ./httpserver address port#
- once the server is online, connet as a client via another terminal. 
- use the following curl commands 
    - curl -T file http://localhost:8080/filename, to PUT a file onto the server 
    - curl http://localhost:8080/filename, to GET a file from the server.  

Designed and written by: 
    - Scott Melero (Stmelero)
    - Korbie Sevilla (Ksevilla)

Sources:
	- Linux man   
	- How to use fstat
	- https://stackoverflow.com/questions/28288775/how-do-you-properly-use-the-fstat-function-and-what-are-its-limits
	- How to use strstr
	- https://stackoverflow.com/questions/20649390/searching-substrings-in-char-array
