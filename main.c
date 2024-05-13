#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CONNECTIONS 30

#define THREAD_POOL_SIZE 50
#define BUFFER_SIZE 1024

// Function to handle client request
void *handle_client(void *arg);

// Function to handle GET request
void handle_get(int client_fd, char *path);

// Function to handle POST request
void handle_post(int client_fd, char *path);

// Function to send response to client
void send_response(int client_fd, int status_code, char *content_type, char *content);

// Function to get content type based on file extension
char *get_content_type(char *path);

// Function to execute python script and get output
char *execute_python_script(char *path);

int main(int argc, char *argv[]) {
    int server_fd, client_fd, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // Check command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Initialize socket structure
    bzero((char *) &server_addr, sizeof(server_addr));
    port = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the host address
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Start listening for the clients
    listen(server_fd, MAX_CONNECTIONS);
    printf("Server is running on port %d\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
        if (client_fd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Create a new thread for each connection
        if (pthread_create(&thread_id, NULL, handle_client, (void *) &client_fd) < 0) {
            perror("ERROR on pthread_create");
            continue;
        }

        // Detach the thread
        pthread_detach(thread_id);
    }

    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *(int *) arg;
    char buffer[BUFFER_SIZE], method[8], path[256], protocol[16];
    ssize_t bytes_read;

    // Read the client request
    bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        pthread_exit(NULL);
    }

    // Null terminate the buffer
    buffer[bytes_read] = '\0';

    // Parse the method, path and protocol from the request
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Handle GET request
    if (strcmp(method, "GET") == 0) {
        handle_get(client_fd, path);
    }
    // Handle POST request
    else if (strcmp(method, "POST") == 0) {
        handle_post(client_fd, path);
    }
    // Invalid method
    else {
        send_response(client_fd, 400, "text/plain", "Invalid method");
    }

    close(client_fd);
    pthread_exit(NULL);
}

void handle_get(int client_fd, char *path) {
    char *content, buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytes_read, content_length = 0;

    // If path is root, return server information
    if (strcmp(path, "/") == 0) {
        send_response(client_fd, 200, "text/plain", "Server: MyHTTPServer/1.0");
        return;
    }

    // Open the file
    file = fopen(path + 1, "r");
    if (file == NULL) {
        send_response(client_fd, 404, "text/plain", "File not found");
        return;
    }

    // Read the file content
    content = malloc(BUFFER_SIZE);
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        content = realloc(content, content_length + bytes_read);
        memcpy(content + content_length, buffer, bytes_read);
        content_length += bytes_read;
    }

    // Null terminate the content
    content = realloc(content, content_length + 1);
    content[content_length] = '\0';

    fclose(file);

    // Send response
    send_response(client_fd, 200, get_content_type(path), content);
    free(content);
}

void handle_post(int client_fd, char *path) {
    char *content;

    // Check if path is a python script
    if (strstr(path, ".py") == NULL) {
        send_response(client_fd, 400, "text/plain", "Invalid path");
        return;
    }

    // Execute python script and get output
    content = execute_python_script(path + 1);
    if (content == NULL) {
        send_response(client_fd, 500, "text/plain", "Internal Server Error");
        return;
    }

    // Send response
    send_response(client_fd, 200, "text/plain", content);
    free(content);
}

void send_response(int client_fd, int status_code, char *content_type, char *content) {
    char *status_text, response[BUFFER_SIZE];

    // Get status text based on status code
    switch (status_code) {
        case 200:
            status_text = "OK";
            break;
        case 400:
            status_text = "Bad Request";
            break;
        case 404:
            status_text = "Not Found";
            break;
        case 500:
            status_text = "Internal Server Error";
            break;
        default:
            status_text = "Unknown";
    }

    // Write response
    sprintf(response, "HTTP/1.0 %d %s\r\nContent-Type: %s\r\n\r\n%s", status_code, status_text, content_type, content);
    write(client_fd, response, strlen(response));
}

char *get_content_type(char *path) {
    char *extension = strrchr(path, '.');

    if (extension == NULL) {
        return "application/octet-stream";
    }
    else if (strcmp(extension, ".html") == 0) {
        return "text/html";
    }
    else if (strcmp(extension, ".css") == 0) {
        return "text/css";
    }
    else if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    }
    else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        return "image/jpeg";
    }
    else if (strcmp(extension, ".png") == 0) {
        return "image/png";
    }
    else if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    }
    else {
        return "application/octet-stream";
    }
}

char *execute_python_script(char *path) {
    char buffer[BUFFER_SIZE], *content = NULL;
    FILE *file;
    size_t bytes_read, content_length = 0;

    // Execute python script
    sprintf(buffer, "python %s", path);
    file = popen(buffer, "r");
    if (file == NULL) {
        return NULL;
    }

    // Read the output
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        content = realloc(content, content_length + bytes_read);
        memcpy(content + content_length, buffer, bytes_read);
        content_length += bytes_read;
    }

    // Null terminate the content
    content = realloc(content, content_length + 1);
    content[content_length] = '\0';

    pclose(file);

    return content;
}