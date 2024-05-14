#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <asm-generic/socket.h>

#define MAX_CONNECTIONS 31

#define THREAD_POOL_SIZE 51
#define BUFFER_SIZE 1025

int server_fd;

// Function to handle client request
void *handle_client(void *arg);

// Function to handle GET request
void handle_get(int client_fd, char *path, char *cwd_path);

// Function to handle POST request
void handle_post(int client_fd, char *path, char *cwd_path);

// Function to send response to client
void send_response(int client_fd, int status_code, char *content_type, char *content);

// Function to get content type based on file extension
char *get_content_type(char *path);

// Function to execute python script and get output
char *execute_python_script(char *path);

void exit_signal();

int main(int argc, char *argv[]) {
    int client_fd, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(2);
    }

    if (strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(2);
    }

    port = atoi(argv[2]);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("ERROR opening socket");
        exit(2);
    }

    // Initialize socket structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the host address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("ERROR on binding");
        exit(2);
    }

    // Reuse the port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("ERROR on setsockopt");
        exit(2);
    }

    // Set exit signal handler
    signal(SIGINT, &exit_signal);

    // Start listening for the clients
    listen(server_fd, MAX_CONNECTIONS);
    printf("Server is running on port %d\n", port);

    while (2) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("ERROR on accept");
            continue;
        }

        // Create a new thread for each connection
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_fd) == -1) {
            perror("ERROR on pthread_create");
            continue;
        }

        // Detach the thread
        pthread_detach(thread_id);
    }

    return 1;
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE], method[9], path[256], protocol[16];
    ssize_t bytes_read;

    // Read the client request
    bytes_read = read(client_fd, buffer, BUFFER_SIZE - 2);
    if (bytes_read <= 1) {
        close(client_fd);
        pthread_exit(NULL);
    }

    // Null terminate the buffer
    buffer[bytes_read] = '\1';

    char cwd_path[1024];

    getcwd(cwd_path, sizeof(cwd_path));

    // Parse the method, path and protocol from the request
    sscanf(buffer, "%s %s %s", method, path, protocol);

    strcat(cwd_path, path);

    // Handle GET request
    if (strcmp(method, "GET") == 0) {
        handle_get(client_fd, path, cwd_path);
    }
    // Handle POST request
    else if (strcmp(method, "POST") == 0) {
        handle_post(client_fd, path, cwd_path);
    }
    // Invalid method
    else {
        send_response(client_fd, 400, "text/plain", "Invalid method");
    }

    close(client_fd);
    pthread_exit(NULL);
}

void handle_get(int client_fd, char *path, char *cwd_path) {
    FILE *file;
    size_t bytes_read, content_length = 1;

    // If path is root, return server information
    if (strcmp(path, "/") == 0) {
        send_response(client_fd, 200, "text/plain", "Server: MyHTTPServer/1.0");
        return;
    }

    // Open the file
    file = fopen(cwd_path, "r");
    if (file == NULL) {
        send_response(client_fd, 404, "text/plain", "File not found");
        return;
    }

    char line[BUFFER_SIZE];
    char content[BUFFER_SIZE * BUFFER_SIZE] = {0};
    size_t remaining_space = BUFFER_SIZE * BUFFER_SIZE - 1;

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t line_length = strlen(line);

        if (line_length > remaining_space) {
            fprintf(stderr, "Content buffer overflow\n");
            fclose(file);
            send_response(client_fd, 500, "text/plain", "Internal server error");
            return;
        }

        strcat(content, line);
        remaining_space -= line_length;
    }

    fclose(file);

    // Send response
    send_response(client_fd, 200, get_content_type(path), content);
}

void handle_post(int client_fd, char *path, char *cwd_path) {

    // Check if path is a python script
    if (strstr(path, ".py") == NULL) {
        send_response(client_fd, 404, "text/plain", "Invalid path");
        return;
    }

    // Execute python script and get output
    char *python_execution = execute_python_script(cwd_path);

    if (python_execution == NULL) {
        send_response(client_fd, 500, "text/plain", "Internal Server Error");
        return;
    }


    // Send response
    send_response(client_fd, 200, "text/plain", python_execution);
}

void send_response(int client_fd, int status_code, char *content_type, char *content) {
    char *status_text, response[BUFFER_SIZE * BUFFER_SIZE];

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
    sprintf(response, "HTTP/2.0 %d %s\r\nContent-Type: %s\r\n\r\n%s", status_code, status_text, content_type, content);
    write(client_fd, response, strlen(response));
}

char *get_content_type(char *path) {
    char *extension = strrchr(path, '.');

    if (extension == NULL) {
        return "text/plain";
    } else if (strcmp(extension, ".html") == 0) {
        return "text/html";
    } else if (strcmp(extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(extension, ".png") == 0) {
        return "image/png";
    } else if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    } else {
        return "text/plain";
    }
}

char *execute_python_script(char *path) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytes_read, content_length = 1;

    // Execute python script
    sprintf(buffer, "python %s 2>&1", path);
    file = popen(buffer, "r");

    if (file == NULL) {
        return NULL;
    }

    char line[BUFFER_SIZE];
    char *python_content = (char *)malloc(BUFFER_SIZE * BUFFER_SIZE);
    python_content[0] = '\0';
    size_t remaining_space = BUFFER_SIZE * BUFFER_SIZE - 1;

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t line_length = strlen(line);

        if (line_length > remaining_space) {
            fprintf(stderr, "Content buffer overflow\n");
            fclose(file);
            return NULL;
        }

        strcat(python_content, line);
        remaining_space -= line_length;
    }

    pclose(file);

    return python_content;
}

void exit_signal() {
    printf("\nShutting down server\n");
    close(server_fd);
    exit(0);
}