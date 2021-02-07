/* simple http server 
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

#define BUFFER_SIZE 4096

struct httpObject {
    /* Create some object 'struct' to keep track of all
        the components related to a HTTP message */
    char method[5];         // PUT, HEAD, GET
    char filename[28];      // File we are reading or creating
    char httpversion[9];    // http version
    char header[BUFFER_SIZE];  // server response
    char content[BUFFER_SIZE]; // temp char for content length 
    int content_length; 
    int status_code;
    __int32_t fd; 
    char buffer[BUFFER_SIZE];
};

// read the response from the client and parse out the data. 
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
            message[0].status_code = 400;
        }
    }
    return;
} 

// execute a GET request 
void execute_GET(struct httpObject* message) {
    // do some stuff 
    printf("[+]GET file...\n");

    // fd to get the file in question
    message[0].fd = open(message[0].filename, O_RDWR);
    
    // catch errors
    if(message[0].fd == -1){
        // 404
        if(errno == 2){
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 404;
        }
        // 403
        else if(errno == 13){
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 403;
        }
        // else 500
        else {
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 500;
        }
    } 

    // continue with the opening
    else {

        struct stat finfo;
        fstat(message[0].fd, &finfo);

        // set the content length 
        message[0].content_length = finfo.st_size;
        message[0].status_code = 200;
    }
    return;
}

// execute a PUT request 
void execute_PUT(ssize_t client_sockd, struct httpObject* message){
    // do some stuff 
    printf("[+]PUT file...\n");

    // fd to get the file in question
    __int32_t fd = open(message[0].filename, O_CREAT | O_TRUNC |  O_RDWR, S_IWUSR | S_IRUSR);

     // catch errors
    if(fd == -1){
        // 404
        if(errno == 2){
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 404;
        }
        // 403
        else if(errno == 13){
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 403;
        }
        // else 500
        else {
            // set the content length 
            message[0].content_length = 0;
            message[0].status_code = 500;
        }
    }

    // continue with the opening
    else {
        message[0].status_code = 201; 

        __int32_t arg_file = 0;   
        for(int byteCount = 0; byteCount < message[0].content_length; byteCount+=arg_file){
            arg_file = recv(client_sockd, message[0].buffer, BUFFER_SIZE, 0);
            if(arg_file == 0)
               { message[0].status_code = 500; message[0].content_length = byteCount; break;}
            write(fd, message[0].buffer, arg_file);
        }
    }

    close(fd);
    return;
}

// Figure out what we need to do based off of the parsed request from the client. 
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
            { message[0].status_code = 400; } 
    }
    return;
}

/*
    3. Construct some response based on the HTTP request we recieved
*/
void construct_http_response(ssize_t client_sockd, struct httpObject* message) {

    printf("[+]Constructing Response\n");

    // check for 200
    if(message[0].status_code == 200) { 
        // load up our message
        char const *msg_http ="HTTP/1.1 %d OK\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);

        send(client_sockd, message[0].header, strlen(message[0].header), 0);

        int byteCount = 0;
        __int32_t arg_file;
        while(( arg_file = read(message[0].fd, message[0].buffer, BUFFER_SIZE)) > 0){
            send(client_sockd, message[0].buffer, arg_file, 0);
            byteCount += arg_file;
            read(message[0].fd + byteCount, message[0].buffer, BUFFER_SIZE);
        } 

        close(message[0].fd);
    }
    
    // check for 201
    else if (message[0].status_code == 201){ 
        char const *msg_http ="\r\nHTTP/1.1 %d Created\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
        send(client_sockd, message[0].header, strlen(message[0].header), 0);
    } 
    
    else if (message[0].status_code == 400){
        char const *msg_http ="\r\nHTTP/1.1 %d Bad Request\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
        send(client_sockd, message[0].header, strlen(message[0].header), 0);
    }

    else if (message[0].status_code == 403){
        char const *msg_http ="\r\nHTTP/1.1 %d Forbidden\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
        send(client_sockd, message[0].header, strlen(message[0].header), 0);
    }
    
    else if (message[0].status_code == 404){
        char const *msg_http ="\r\nHTTP/1.1 %d Not Found\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
        send(client_sockd, message[0].header, strlen(message[0].header), 0);
    }

    else if(message[0].status_code == 500){
        char const *msg_http ="\r\nHTTP/1.1 %d Internal Server Error\r\nContent-Length: %zu\r\n\r\n";
        sprintf(message[0].header, msg_http, message[0].status_code, message[0].content_length);
        send(client_sockd, message[0].header, strlen(message[0].header), 0);
    }
    return;
}

/*
  getaddr returns the numerical representation of the address
  identified by *name* as required for an IPv4 address represented
  in a struct sockaddr_in.
 */
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


/*
  Start:
 */
int main(int argc, char* argv[]) {
    unsigned short port;

    if(argc == 1) {
        char msg[] = "getaddrinfo(): address identification error\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }

    // specifying the port number on the command line
    if(argv[2] != NULL)
        port = atoi(argv[2]);
    else
       port = 80;

    // Create sockaddr_in with server information 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = getaddr(argv[1]);
    server_addr.sin_port = htons(port);
    socklen_t addrlen = sizeof(server_addr);

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

    struct httpObject message;

    while (true) {
        printf("[+] server is waiting...\n");

        // 1. Accept Connection
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);

        // Check for errors
        if(client_sockd == -1){
            perror("accept");
            continue;
        }

        // Parse the HTTP Message
        read_http_response(client_sockd, &message);

        // Process the Request
        process_request(client_sockd, &message);

        // Construct Clinet Response
        construct_http_response(client_sockd, &message);

        // Send that shit off
        printf("Response Sent\n");

        close(client_sockd);
        }

    return EXIT_SUCCESS;
}
