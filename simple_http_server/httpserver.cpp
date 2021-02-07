/* httpserver w/ Backup & Recovery 
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
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 4096

/* Create some object 'struct' to keep track of all
    the components related to a HTTP message */
struct httpObject {
    char method[5];            // PUT, HEAD, GET
    char filename[28];         // File we are reading or creating
    char timestamp[33];        // store the timestamp if one is given
    char httpversion[9];       // http version
    char header[BUFFER_SIZE];  // server response
    char content[BUFFER_SIZE]; // temp char for content length 
    int content_length;        // amount of data sent back to the client 
    int status_code;           // status of the request 
    __int32_t fd;              // fd for the file the client is getting
    char buffer[BUFFER_SIZE];  // buffer tht holds temp data to write/send
};

/* 1) read the response from the client and parse out the data. */
void read_http_response(ssize_t client_sockd, struct httpObject* message) {
    printf("[+]Reading Request...\n");

    // clear the status code 
    message[0].status_code = 0;

    // clear the previous timestamp. 
    memset(message[0].timestamp, 0, sizeof message[0].timestamp);

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

    // first check to see if we are doing a recovert with a given timestamp
    if((message[0].filename[1] == 'r') && (message[0].filename[2] == '/')){
        int i = 0;
        char *token[4];

        // pares the filename 
        for (char * p = strtok(message[0].filename, "/"); p; p = strtok(NULL, "/")){
            printf("p = %s\r\n", p);
            token[i] = p;
            i++;
        }

        // set the timestamp to the number given past the r
        strncpy(message[0].timestamp, token[1], 33);
        // set the filename back to r
        strncpy(message[0].filename, token[0], 28);
    } 

    // else, parse the file as normal. 
    else{
        // removes the '/' from the file path
        for(int x = 0;message[0].filename[x] != '\0'; x++) {
            message[0].filename[x] = message[0].filename[x+1];

            // file can only be alphanumeric characters 
            if((isalnum(message[0].filename[x+1]) == 0) && (message[0].filename[x+1] != '\0')){
                message[0].status_code = 400;
            }
        }
    }
    return;
} 

/* execute a GET request */
void execute_GET(struct httpObject* message) {
    // do some stuff 
    printf("[+]GET file...\n");

    // fd to get the file in question
    message[0].fd = open(message[0].filename, O_RDWR);
    
    // catch errors
    if(message[0].fd == -1){

        switch(errno){
            case 2:
                message[0].status_code = 404; // DNE
                break;
            case 13: 
                 message[0].status_code = 403; // FORBID
                 break; 
            default: 
                 message[0].status_code = 500; // SERVER ERROR
                 break;
        }

        // set the content length 
        message[0].content_length = 0;
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

/* execute a PUT request */
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
               { message[0].status_code = 500; break;}
            write(fd, message[0].buffer, arg_file);
        }
    }

    message[0].content_length = 0;
    close(fd);
    return;
}

/* backup the server files */
void perform_backup(struct httpObject* message){ 
    printf("[+]backing up...\r\n");

    // Stores time seconds
    time_t seconds;
    time(&seconds);

    // or opening the directory. 
    DIR *d;
    struct dirent *dir;

    // we need to format the filepath into a string. 
    char file[28];
    char const *backup_folder ="./backup-%d";
    snprintf(file, 28, backup_folder, seconds);

    // make the backup-timestamp directory. 
    int check = mkdir(file, O_CREAT | O_TRUNC |  O_RDWR | S_IWUSR | S_IRUSR);
        if(check == -1)
            printf("error with mkdir");

    // open the directory
    d = opendir(".");
    if (d){
        while ((dir = readdir(d)) != NULL){
            // be sure to skip past the folders
            struct stat buffer;
            lstat(dir->d_name, &buffer);
            if((S_ISDIR(buffer.st_mode)) == false) { 

                // we need to start by opening the og file and creating a backup file
                char backup_file[50];
                char const *backup_path ="%s/%s";
                snprintf(backup_file, 50, backup_path, file, dir->d_name);

                int og_fd = open(dir->d_name, O_RDWR);

                // if we coudlnt open the file, skip and close it. 
                if(og_fd != -1){
                    int backup_fd = open(backup_file, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
                
                    message[0].status_code = 201; 

                    // write the contents of the og into the backup. 
                    __int32_t arg_file;
                    while(( arg_file = read(og_fd, message[0].buffer, BUFFER_SIZE)) > 0)
                        { write(backup_fd, message[0].buffer, arg_file); } 

                    // close thise mf's
                    close(backup_fd);
                }

                close(og_fd);
            }
        }

        // and close the directory. 
        closedir(d);
    }

    // nothing to return, so the content lenght is 0
    message[0].status_code = 200;
    message[0].content_length = 0;
    return;
}

/* recover to a stored backup */
void perform_recovery(struct httpObject* message){
    printf("[+]recovering...\r\n");

    // for opening the directory. 
    DIR *d;
    struct dirent *dir;
    int most_recent = 0; 

    // set up the folder path
    char file[28];
    char const *backup_folder ="./backup-%d";

    // if there is no timestamp given, then we need to find the most recent backup. 
    if(atoi(message[0].timestamp) == 0){
        d = opendir(".");
        if (d){
            while ((dir = readdir(d)) != NULL){
                // only open folders 
                struct stat buffer;
                lstat(dir->d_name, &buffer);
                if(S_ISDIR(buffer.st_mode)) { 

                    // parse the folder name
                    char *time_stamp = strtok(dir->d_name, "-");

                    // if it is a backup folder, then get the timestamp. 
                    if(strcmp(time_stamp, "backup") == 0){
                        int curr_time = atoi(strtok(NULL, ""));
                        // if the curr timestamp is the most recent, then use it. 
                        if(curr_time > most_recent)
                            most_recent = curr_time;  
                    }
                }
            }
            closedir(d);
        }
    }

    // if there is a timestamp, use that one. 
    else
        most_recent = atoi(message[0].timestamp);
    snprintf(file, 28, backup_folder, most_recent);

    // start the recoevry process. 
    d = opendir(file);
    // if we cant open the folder, return 404
    if(d == NULL)
        message[0].status_code = 404;
    else if (d){
        while ((dir = readdir(d)) != NULL){
            // open the backup file and create/truncate the og file. 
            char backup_file[50];
            char const *backup_path ="%s/%s";
            snprintf(backup_file, 50, backup_path, file, dir->d_name);

            int og_fd = open(dir->d_name, O_CREAT | O_TRUNC |  O_RDWR, S_IWUSR | S_IRUSR);
            int backup_fd = open(backup_file, O_RDWR);
        
            message[0].status_code = 201; 

            // replace the og file with the contents of the backup. 
            __int32_t arg_file;
            while(( arg_file = read(backup_fd, message[0].buffer, BUFFER_SIZE)) > 0)
                { write(og_fd, message[0].buffer, arg_file); } 

            // close thos mf's
            close(og_fd);
            close(backup_fd);

        }

        // ...and the directory.  
        closedir(d);
        message[0].status_code = 200;
    }

    // nothing to send back, so cotent length is 0
    message[0].content_length = 0;
    return;
}

/* list available backups*/
void list_backups(struct httpObject* message){
    printf("[+] listing times...\r\n");

    // clear the buffer
    memset(message[0].buffer,0,sizeof message[0].buffer);

    DIR *d;
    struct dirent *dir;
    
    // formatted time stamp. 
    char const *timestamp_f ="%s\n";

    d = opendir(".");
    if (d){
        while ((dir = readdir(d)) != NULL){
            // only read folders
            struct stat buffer;
            lstat(dir->d_name, &buffer);
            if(S_ISDIR(buffer.st_mode)) { 
                char *time_stamp = strtok(dir->d_name, "-");

                // we are only interested in the backup folders. 
                if(strcmp(time_stamp, "backup") == 0){
                    char* curr_time = strtok(NULL, "-");
                    // get the timestamp and add it to the buffer to create a list. 
                    sprintf(message[0].buffer + strlen(message[0].buffer), timestamp_f, curr_time);
                    
                }
            }
        }
        closedir(d);
    }

    // send back how ever many bytes worth of timestamps we are sending to the client. 
    message[0].status_code = 200;
    message[0].content_length = strlen(message[0].buffer);
    return;
}
    
/* 2) Figure out what we need to do based off of the parsed request from the client. */ 
void process_request(ssize_t client_sockd, struct httpObject* message) {
    printf("[+]Processing Request...\n");
   
    // check for get
    if(strncmp(message[0].method, "GET", 5) == 0) { 

        // special op: backup
        if(strcmp(message[0].filename, "b") == 0)
            perform_backup(message);
        
        // special op: recover
        else if(strcmp(message[0].filename, "r") == 0)
            perform_recovery(message);
        
        // special op: list backups
        else if(strcmp(message[0].filename, "l") == 0)
            list_backups(message);
        
        // operate on the file as normal. 
        else {
            // if it is a bad file, dont bother with it. 
            if(strlen(message[0].filename) != 10) 
                message[0].status_code = 400;
            else 
                execute_GET(message);
        }           
    }

    // check for put
    else if(strncmp(message[0].method, "PUT", 5) == 0){ 
        // if it is a bad file, dont bother with it. 
        if(strlen(message[0].filename) != 10) 
            message[0].status_code = 400;
        else 
            execute_PUT(client_sockd, message); 
    } 

    // unsupported operation has been requested. 
    else
        message[0].status_code = 500; 

    // bad request from faulty file name. 
    if(message[0].status_code == 400)
        message[0].content_length = 0; 
    
    return;
}

/* 3) Construct some response. */
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
    if((message[0].status_code == 200) && (strlen(message[0].filename) > 1)) { 

        __int32_t arg_file;
        while(( arg_file = read(message[0].fd, message[0].buffer, BUFFER_SIZE)) > 0)
            { send(client_sockd, message[0].buffer, arg_file, 0); } 

        close(message[0].fd);
    }

    // if it is a list request, send the list back to the client. 
    else if((message[0].status_code == 200) && (strcmp(message[0].filename, "l") == 0)) 
        send(client_sockd, message[0].buffer, strlen(message[0].buffer), 0);

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

/* Start: */ 
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
