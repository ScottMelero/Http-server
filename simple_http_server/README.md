A multi-threaded HTTP server, with redundancay, written in C.

How to Run: 
Server terminal: 
    - compile all of the source file using the make command 
    - launch the server using the following command: 
        ./httpserver address port# -r -N #
    - the address is required
    - port# and the -r and -N flags are optional 
    - -N should be followed by a number, and will tell the server how many threads to 
        make 

client: 
    - the best way to test this server is to run a shell script that contains 
        multiple curl commands, and direct the output to separate files. 
    - make a new shell file "shell.sh". fill it with curl commands as follows: 
        curl -T t1 https://localhost:8080/filename01 > out01 &
        curl -T t1 https://localhost:8080/filename02 > out02 &
        curl -T t1 https://localhost:8080/filename03 > out03 ....
    - run the command "chmod +x shell.sh" to make the script executable, then run ./shell.sh
    - check the contents of the output files. 

Designed and written by: 
    - Scott Melero (Stmelero)
    - Korbie Sevilla (Ksevilla)
