/* Multi-threaded httpserver
─────────▄──────────────▄
────────▌▒█───────────▄▀▒▌
────────▌▒▒▀▄───────▄▀▒▒▒▐
───────▐▄▀▒▒▀▀▀▀▄▄▄▀▒▒▒▒▒▐
─────▄▄▀▒▒▒▒▒▒▒▒▒▒▒█▒▒▄█▒▐
───▄▀▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▀██▀▒▌
▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄                                                              
▀█▄▀▄▀██████░▀█▄▀▄▀████▀░▌
─▌▀█▄█▄███▀░░░▀██▄█▄█▀░░░░ ▌
─▌▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▌
─▌▒▀▄██▄▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▌
─▌▀▐▄█▄█▌▄▒▀▒▒▒▒▒▒░░░░░░▒▒▒▐
▐▒▀▐▀▐▀▒▒▄▄▒▄▒▒▒▒▒░░░░░░▒▒▒▒▌
▐▒▒▒▀▀▄▄▒▒▒▄▒▒▒▒▒▒░░░░░░▒▒▒▐
─▌▒▒▒▒▒▒▀▀▀▒▒▒▒▒▒▒▒░░░░▒▒▒▒▌
─▐▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▐
──▀▄▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▄▒▒▒▒▌
────▀▄▒▒▒▒▒▒▒▒▒▒▄▄▄▀▒▒▒▒▄▀
*/

#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <netdb.h>
#include <err.h>
#include <pthread.h>
#include <iostream> 
#include <unordered_map> 
#include <dirent.h>
#include "myqueue.h"

#define BUFFER_SIZE 4096 // data buffer 

bool redundancy = false; // check is red flag is present 
using namespace std; 
int lock_count = 0;
unordered_map<string, pthread_mutex_t*> lock_map; 


pthread_t thread_pool[BUFFER_SIZE]; // pool of worker threads 
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // global mutex lock for threads 
pthread_mutex_t newfile_mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t dir_mutex[500]; // mutex lock pool for each of our files

pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER; // conditional to ensure thread safety

/* struct for parse data */
struct httpObject {
    char method[5];            // PUT, HEAD, GET
    char filename[28];         // File we are reading or creating
    char httpversion[9];       // http version
    char header[BUFFER_SIZE];  // server response
    char content[BUFFER_SIZE]; // temp char for content length 
    int content_length;        // content length
    int status_code;           // response status code
   __int32_t fd[BUFFER_SIZE];  // fd for get 
    char buffer[BUFFER_SIZE];  // file data buffer 
};

/* 1: read the response from the client and 
   parse out the data. */
void read_http_response(ssize_t client_sockd, struct httpObject* message) {
    printf("[+]Reading Request...\n");
    message[0].status_code = 0;

    // create a buff to hold our client request
    char buff[BUFFER_SIZE];

    // keep calling recv in a loop
    while(1){
        recv(client_sockd, buff, BUFFER_SIZE, 0);

         // when the last 4 bytes of buff are a blank newline, that is the end of the header
        if(strstr(buff, "\r\n\r\n")) {
            // now we should do sscanf to parse the header 
            sscanf(buff, "%s %s %s", message[0].method, message[0].filename, 
                        message[0].httpversion);    

            if(char* pnt = strstr(buff, "Content-Length:")){ 
                sscanf(pnt, "%s %s", message[0].content, message[0].content);
                message[0].content_length = atoi(message[0].content);
            }
            break;
        }
    }

    // removes the '/' from the file path
    for(int x = 0;message[0].filename[x] != '\0'; x++) {
        message[0].filename[x] = message[0].filename[x+1]; 
        if(isalnum(message[0].filename[x+1]) == 0 && message[0].filename[x+1] != '\0'){
            message[0].status_code = 500;
        }
    }
    return;
} 

// execute a GET request 
void execute_GET(struct httpObject* message) {
    // do some stuff 
    printf("[+]Get file...\n");

    // lock the filename before we open it. 
    if((lock_map[message[0].filename]) != NULL)
        pthread_mutex_lock(lock_map[message[0].filename]); 
    else{
         pthread_mutex_lock(&newfile_mutex);
         lock_map[message[0].filename] = &dir_mutex[lock_count];
         lock_count++;
         pthread_mutex_unlock(&newfile_mutex);
         pthread_mutex_lock(lock_map[message[0].filename]); 
    }

    char file[28];
    char const *filePath ="./copy%d/%s";
    int errCnt = 0,
        forbid = 0, 
        dne    = 0,
        r      = 1;

    if(redundancy)
        r = 3; 

    // open all 3 copies and record any errors that we find 
    for(int i = 1; i <= r; i++){

        if(r == 1){
            message[0].fd[i] = open(message[0].filename, O_RDWR);
        } else {
            snprintf(file, 28, filePath, i, message[0].filename);
            message[0].fd[i] = open(file, O_RDWR);
        }

        if(message[0].fd[i] == -1){
            errCnt++;
            switch(errno){
                case 2:
                    dne++;
                    break;
                case 13:
                    forbid++;
                    break; 
            }
        } 
    }

    // check if a majority of the opens failes
    if((errCnt >= 2) || (errCnt == r)){
        message[0].content_length = 0;
        if((forbid >= 2) || (forbid == r))
            // lots of 403's
            message[0].status_code = 403;
        else if((dne >= 2) || (dne == r))
            // lots of 404's
            message[0].status_code = 404;
        else
            // lots of 500's
            message[0].status_code = 500;
    } 

    // otherwise, check if at least 2 copies are equal. 
    else {
        char  i_tmp[BUFFER_SIZE],
              j_tmp[BUFFER_SIZE];
        __int32_t i_file,
                  j_file;

        int i, j;
        for(i = 1; i<=r; i++){

            if(!redundancy)
                goto noRedundancy;

            for(j = i+1; j<=r; j++){

                // there could be one last erroneous file
                if((message[0].fd[i] != -1) && (message[0].fd[j] != -1)){

                    // while both files are not empty...
                    while(((i_file = read(message[0].fd[i], i_tmp,BUFFER_SIZE)) > 0)&&
                          ((j_file = read(message[0].fd[j], j_tmp,BUFFER_SIZE)) > 0)){

                        // ... read a piece of them into a buffer and compare the buffs...
                        if(strncmp(i_tmp, j_tmp, i_file) != 0){
                            printf("files are not equal\r\n");
                            lseek(message[0].fd[i], 0, SEEK_SET);
                            lseek(message[0].fd[j], 0, SEEK_SET);
                            goto breakout; 
                        }

                    }

                    // if we finish the loop, we found 2 equal files
                    printf("Found 2 equal files\r\n");

                    lseek(message[0].fd[i], 0, SEEK_SET);

                    // i know its ugly, but it may work
                    noRedundancy:
                    message[0].fd[0] = message[0].fd[i];
                    struct stat finfo;
                    fstat(message[0].fd[i], &finfo);

                    // set the content length 
                    message[0].content_length = finfo.st_size;
                    message[0].status_code = 200;
                    goto exitGET;

                    // break label 
                    breakout:
                    printf("trying other files\r\n");
                }
            }
        } 

        // if we get to this part, then none of the files are equal
        printf("all files are different.\r\n");
        message[0].status_code = 500;
    }
    exitGET:
    pthread_mutex_unlock(lock_map[message[0].filename]); 
    return;
}

// execute a PUT request 
void execute_PUT(ssize_t client_sockd, struct httpObject* message){
    // do some stuff 
    printf("[+]PUT file...\n"); 

    // lock the filename before we open it. 
    if((lock_map[message[0].filename]) != NULL)
        pthread_mutex_lock(lock_map[message[0].filename]); 
    else{
         pthread_mutex_lock(&newfile_mutex);
         lock_map[message[0].filename] = &dir_mutex[lock_count];
         lock_count++;
         pthread_mutex_unlock(&newfile_mutex);
         pthread_mutex_lock(lock_map[message[0].filename]); 
    }
    

    char file[28];
    char const *filePath ="./copy%d/%s";
    int errCnt = 0,
        forbid = 0, 
        dne    = 0,
        r      = 1;

    if(redundancy)
        r = 3; 

    // open all 3 copies and record any errors that we find 
    for(int i = 1; i <= r; i++){

        if(r == 1){
            message[0].fd[i] = open(message[0].filename, O_CREAT | O_TRUNC |  O_RDWR, S_IWUSR | S_IRUSR);
        } else {
            snprintf(file, 28, filePath, i, message[0].filename);
            message[0].fd[i] = open(file, O_CREAT | O_TRUNC |  O_RDWR, S_IWUSR | S_IRUSR);
        }
        
        if(message[0].fd[i] == -1){
            errCnt++;
            switch(errno){
                case 2:
                    dne++;
                    break;
                case 13:
                    forbid++;
                    break; 
            }
        } 
    }
    

    // check if a majority of the opens failes
    if((errCnt >= 2) || (errCnt == r)){
        printf("error found\r\n");
        message[0].content_length = 0;
        if((forbid >= 2) || (forbid == r))
            // lots of 403's
            message[0].status_code = 403;
        else if((dne >= 2) || (dne == r))
            // lots of 404's
            message[0].status_code = 404;
        else
            // lots of 500's
            message[0].status_code = 500;
    } 

    // continue with the opening
    else {
        message[0].status_code = 201;

        __int32_t arg_file = 0;   
        for(int byteCount = 0; byteCount < message[0].content_length; byteCount+=arg_file){
            arg_file = recv(client_sockd, message[0].buffer, BUFFER_SIZE, 0);
            
            if(arg_file == 0)
               { message[0].status_code = 500; message[0].content_length = byteCount; goto exitPut;}

             for(int i = 1; i <= r; i++){
                 write(message[0].fd[i], message[0].buffer, arg_file);
            }

        }

        message[0].content_length = 0;
    }

    for(int i = 1; i <= r; i++)
        { close(message[0].fd[i]); }
    exitPut: 
    // unlock the file once the PUT has been completed. 
    pthread_mutex_unlock(lock_map[message[0].filename]); 
    return;
}

/* 2: Figure out what we need to do based off of the 
   parsed request from the client. */
void process_request(ssize_t client_sockd, struct httpObject* message) {
    printf("[+]Processing Request...\n");

    // if weve got a bad file, respond accordingly 
    if(strlen(message[0].filename) != 10 || message[0].status_code == 400){ 
        message[0].status_code = 400;
        message[0].content_length = 0; 
    }

    else {
        // check for get
        if(strncmp(message[0].method, "GET", 5) == 0) 
            { execute_GET(message); }
        // check for put
        else if(strncmp(message[0].method, "PUT", 5) == 0)
            { execute_PUT(client_sockd, message); } 
        else
            { message[0].status_code = 500; } 
    }
    return;
}

/* 3: Construct some response based on the 
   HTTP request we recieved */
void construct_http_response(ssize_t client_sockd, struct httpObject* message) {

    printf("[+]Constructing Response\n");
    char const *msg_http; 
    switch(message[0].status_code){
        case 200: 
            msg_http ="HTTP/1.1 %d OK\r\nContent-Length: %zu\r\n\r\n";
            break;
        case 201: // CREATED
            msg_http ="HTTP/1.1 %d Created\r\nContent-Length: %zu\r\n\r\n";
            break;
        case 400: // BAD REQUEST 
            msg_http ="HTTP/1.1 %d Bad Request\r\nContent-Length: %zu\r\n\r\n";
            break;
        case 403: // FORBIDDEN
            msg_http ="HTTP/1.1 %d Forbidden\r\nContent-Length: %zu\r\n\r\n";
            break;
        case 404: // FILE NOT FOUND
            msg_http ="HTTP/1.1 %d Not Found\r\nContent-Length: %zu\r\n\r\n";
            break;
        case 500: // SERVER DID A NAUGHTY
            msg_http ="HTTP/1.1 %d Internal Server Error\r\nContent-Length: %zu\r\n\r\n";
            break;
    }

    sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
    send(client_sockd, message[0].header, strlen(message[0].header), 0);

    // dont forget to send the client their file 
    if(message[0].status_code == 200) { 

        __int32_t arg_file;
        while(( arg_file = read(message[0].fd[0], message[0].buffer, BUFFER_SIZE)) > 0)
            { send(client_sockd, message[0].buffer, arg_file, 0); } 

        for(int i = 1; i <=3; i++)
            {close(message[0].fd[i]);}
    }

    return;
}

/* start the request handling */
void * handle_request(void* p_client_sockd){
        ssize_t client_sockd = *((int*)p_client_sockd);
        free(p_client_sockd);

        struct httpObject message;

        // 1: Parse the HTTP Message
        read_http_response(client_sockd, &message);

        // 2: Process the Request
        process_request(client_sockd, &message);

        // 3: Construct Clinet Response
        construct_http_response(client_sockd, &message);

        // Send that shit off
        printf("Response Sent\n");

        close(client_sockd);
    return NULL;
}

/* thread function to give each worker thread */
void * thread_function(void *arg){
    while(true){
        int *pclient; 
        pthread_mutex_lock(&queue_mutex); // lock the thread before we enter the critical region 
        if(( pclient = dequeue()) == NULL){
            pthread_cond_wait(&condition_var, &queue_mutex); // thread waiting for signal from other thread

            pclient = dequeue();
        } 
        pthread_mutex_unlock(&queue_mutex); // unlock the thread
        if(pclient != NULL) {
            // we have a connection 
            handle_request(pclient);
        }
    }
}

/* define some new function to create a mutex lock for each file the dir. 
 add those shits to a hash map with key: filename value: mutex lock*/
void make_locks(){

    DIR *d;
    struct dirent *dir;

    char file[28];
    char const *filePath ="./copy%d";

    if(redundancy){
        for(int i = 1; i <= 3; i++){
            snprintf(file, 28, filePath, i);
            d = opendir(file);
            if (d){
                while ((dir = readdir(d)) != NULL){
                    dir_mutex[lock_count] = PTHREAD_MUTEX_INITIALIZER;  
                    lock_map[dir->d_name] = &dir_mutex[lock_count];
                    lock_count++;
                }
                closedir(d);
            }
        }
    } 

    else{
        d = opendir(".");
        if (d){
            while ((dir = readdir(d)) != NULL){
                dir_mutex[lock_count] = PTHREAD_MUTEX_INITIALIZER;  
                lock_map[dir->d_name] = &dir_mutex[lock_count];
                lock_count++;
            }
            closedir(d);
        }
    }

    return;
}

/* getaddr returns the numerical representation of the address
   identified by *name* as required for an IPv4 address represented
   in a struct sockaddr_in. */
unsigned long getaddr(char *name) {
  unsigned long res;
  struct addrinfo hints;
  struct addrinfo* info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  if (getaddrinfo(name, NULL, &hints, &info) != 0 || info == NULL) {
    char msg[] = "getaddrinfo(): address identification error\n";
    write(STDERR_FILENO, msg, strlen(msg));
    exit(1);
  }
  res = ((struct sockaddr_in*) info->ai_addr)->sin_addr.s_addr;
  freeaddrinfo(info);
  return res;
}

/* Start */
int main(int argc, char* argv[]) {
    make_locks();
    unsigned long address = getaddr(argv[1]);
    unsigned short port;
    int thread_pool_size = 4;

    // specifying the port number on the command line
    if(atoi(argv[2]) > 80)
        port = atoi(argv[2]);
    else
       port = 80;

    int option;
    while((option = getopt(argc, argv, "rN")) != -1){
        switch(option) {
            case 'r':
                redundancy = true; 
                break; 
            case 'N':
                thread_pool_size = atoi(argv[optind]);
                break;
        }
    }

    // Create sockaddr_in with server information 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = address;
    server_addr.sin_port = htons(port);
    socklen_t addrlen = sizeof(server_addr);

    // create N new threads to use for later
    for(int i = 0; i < thread_pool_size; i++){
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    //Create server socket
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);

    // Need to check if server_sockd < 0, meaning an error
    if (server_sockd < 0) 
        perror("socket");

    // Configure server socket
    int enable = 1;

    // This allows you to avoid: 'Bind: Address Already in Use' error
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // Bind server address to socket that is open
    ret = bind(server_sockd, (struct sockaddr *) &server_addr, addrlen);

    // Listen for incoming connections
    ret = listen(server_sockd, 5); 

    if (ret < 0) 
        return EXIT_FAILURE;

    // Connecting with a client
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    // struct httpObject message;

    // this is where we want to start adding in MTh code 
    while (true) {
        printf("[+] server is waiting...\n");

        // 1. Accept Connection
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);

        // Check for errors
        if(client_sockd == -1){
            perror("accept");
            continue;
        }

       /* store the connection in a data structure so that 
            our threads can find them*/
    
        // create a thread for each new request 
        int *pclient = (int*) malloc(sizeof(int));
        *pclient = client_sockd;

        /* lock tht thread right before we add the request the queue so that 
        two threads dont mess with it at the same time */
        pthread_mutex_lock(&queue_mutex); // lock
        enqueue(pclient); // queue up the request 
        pthread_cond_signal(&condition_var); // send waiting thread(s) signal to proceed
        pthread_mutex_unlock(&queue_mutex); // unlock the thread. 
    }

    return EXIT_SUCCESS;
}