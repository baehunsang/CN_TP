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
    char* response;
    if(!strcmp(path, "index.html")){
        response = "HTTP/1.1 200 OK\r\n"
                    "Connection: Keep-Alive\r\n"
                    "keep-alive: timeout=5, max=30\r\n"
                    "Content-Type: text/html\r\n\r\n";
    }
    else if(!strcmp(path, "script.js")){
        response = "HTTP/1.1 200 OK\r\n"
                    "Connection: Keep-Alive\r\n"
                    "keep-alive: timeout=5, max=30\r\n"
                    "Content-Type: text/js\r\n\r\n";
    }
    else if(!strcmp(path, "gr-small.png")){
        response = "HTTP/1.1 200 OK\r\n"
                    "Connection: Keep-Alive\r\n"
                    "keep-alive: timeout=5, max=30\r\n"
                    "Content-Type: image/png\r\n\r\n";
    }
    else if(!strcmp(path, "gr-large.jpg")){
        response = "HTTP/1.1 200 OK\r\n"
                    "Connection: Keep-Alive\r\n"
                    "keep-alive: timeout=5, max=30\r\n"
                    "Content-Type: image/jpg\r\n\r\n";
    }
    

    

    char file_path[MAX_BUF];
    snprintf(file_path, sizeof(file_path), "%s/%s", resources_dir, path);

    FILE* file = fopen(file_path, "rb");
    char buffer[MAX_BUF];

    if (file != NULL) {
        send(new_sock, response, strlen(response), 0);
        size_t n;
        while((n = fread(buffer, 1, MAX_BUF, file)) > 0) {
            send(new_sock, buffer, n, 0);
        }
        fclose(file);
    } else {
        response = "HTTP/1.1 404 Not Found\r\n";
        send(new_sock, response, strlen(response), 0);
    }
}


int process_client_request(int new_sock) {
    char buffer[MAX_BUF];
    if(!recv(new_sock, buffer, sizeof(buffer) - 1, 0)){
        close(new_sock);
        return;
    };
    char method[5];
    char path[MAX_BUF];
    char connection_status[MAX_BUF];

    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") == 0) {
        if (path[0] == '/') {
            memmove(path, path + 1, strlen(path));
        }
        if (strlen(path) == 0) {
            strcpy(path, "index.html");
        }
        
        send_file(new_sock, path);

        
        
    } else {
        // Unsupported method
        char* response = "HTTP/1.1 400 Bad Request\r\n";
        send(new_sock, response, strlen(response), 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(1);
    }

    int portno = atoi(argv[1]);
    strncpy(resources_dir, argv[2], MAX_BUF - 1);
    resources_dir[MAX_BUF - 1] = '\0';

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset((char*)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(server_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        exit(1);
    }

    listen(server_sock, MAX_CLIENTS);

    fd_set readfds;
    int client_sockets[MAX_CLIENTS];
    int max_sd, sd;
    int new_socket;
    int i;
    struct timeval timeout;
    timeout.tv_sec = 0; 
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
            if(sd > 0){
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

            new_socket = accept(server_sock, (struct sockaddr*)&client_addr, &clilen);

            if (new_socket < 0) {
                continue;
            }


            for (i = 0; i < MAX_CLIENTS; i++) {
                if(client_sockets[i] == new_socket){
                    break;
                }
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    FD_SET(client_sockets[i], &readfds);
                    break;
                }
            }
        }
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                if(process_client_request(sd) == -1) {
                    client_sockets[i] = 0;
                }
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