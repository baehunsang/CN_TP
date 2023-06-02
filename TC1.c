#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_BUF 1024
#define MAX_CLIENTS 1024

char resources_dir[MAX_BUF];

void send_file(int new_sock, char* path) {
    char response[MAX_BUF];
    char file_path[MAX_BUF];
    snprintf(file_path, sizeof(file_path), "%s/%s", resources_dir, path);

    FILE* file = fopen(file_path, "rb");
    char buffer[MAX_BUF];

    if (file != NULL) {
         // Get the file size
        fseek(file, 0L, SEEK_END);
        long int file_size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        if(!strcmp(path, "index.html")){
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n"
                "keep-alive: timeout=5, max=30\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n\r\n", file_size);
        }
        if(!strcmp(path, "script.js")){
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n"
                "keep-alive: timeout=5, max=30\r\n"
                "Content-Type: text/js\r\n"
                "Content-Length: %ld\r\n\r\n", file_size);
        }
        if(!strcmp(path, "gr-small.png")){
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n"
                "keep-alive: timeout=5, max=30\r\n"
                "Content-Type: img/png\r\n"
                "Content-Length: %ld\r\n\r\n", file_size);
        }
        if(!strcmp(path, "gr-large.jpg")){
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n"
                "keep-alive: timeout=5, max=30\r\n"
                "Content-Type: img/jpg\r\n"
                "Content-Length: %ld\r\n\r\n", file_size);
        }
        send(new_sock, response, strlen(response), 0);
        size_t n;
        while((n = fread(buffer, 1, MAX_BUF, file)) > 0) {
            send(new_sock, buffer, n, 0);
        }
        fclose(file);
    } else {
        snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\n");
        send(new_sock, response, strlen(response), 0);
        perror("File open error");
    }
}
    

void process_client_request(int new_sock) {
    char buffer[MAX_BUF];
    if(!recv(new_sock, buffer, sizeof(buffer) - 1, 0)){
        close(new_sock);
        return;
    };
    char method[5];
    char path[MAX_BUF];

    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") == 0) {
        if (path[0] == '/') {
            memmove(path, path + 1, strlen(path));
        }
        if (strlen(path) == 0) {
            strcpy(path, "index.html");
        }
        
        send_file(new_sock, path);
        //shutdown(new_sock, SHUT_WR);
    } else {
        // Unsupported method
        char* response = "HTTP/1.1 400 Bad Request\r\n";
        send(new_sock, response, strlen(response), 0);
    }
}

int main(int argc, char *argv[]) {
    /*if (argc != 3) {
        fprintf(stderr,"usage: %s <port> <resources_directory>\n", argv[0]);
        exit(1);
    }*/

    int portno = 8080; //atoi(argv[1]);
    strncpy(resources_dir, "resources"/*argv[2]*/, MAX_BUF - 1);
    resources_dir[MAX_BUF - 1] = '\0';

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset((char*)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(server_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(server_sock, MAX_CLIENTS);

    fd_set readfds;
    int client_sockets[MAX_CLIENTS];
    int max_sd, sd;
    int new_socket;
    int i;
    struct timeval timeout;
    timeout.tv_sec = 0; // timeout after 5 seconds
    timeout.tv_usec = 0;
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(server_sock, &readfds);
        max_sd = server_sock;
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if (FD_ISSET(server_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t clilen = sizeof(client_addr);

            printf("waiting for new client...\n");
            new_socket = accept(server_sock, (struct sockaddr*)&client_addr, &clilen);

            if (new_socket < 0) {
                perror("ERROR on accept");
                continue;
            }

            printf("client connected\n");

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    FD_SET(new_socket, &readfds);
                    break;
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                process_client_request(sd);
            }
            else if(sd > 0){
                close(sd);
                client_sockets[i] = 0;
            }
            else{
                continue;
            }
        }
    }

    close(server_sock);
    return 0;
}