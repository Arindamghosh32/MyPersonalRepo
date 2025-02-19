/*
 * BSD Socket Database Client
 * This client connects to the team lead's server and sends database commands.
 * It provides a command-line interface similar to the existing database system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LENGTH 512

/* Global variables */
int client_socket;
char server_ip[16];

/* Function prototypes */
void initialize_client(void);
void handle_user_input(void);
void cleanup_client(void);
void handle_signal(int sig);
int start_socket_client(const char *ip_address, int port, void (*signal_handler)(int));

/* Signal handler for graceful shutdown */
void handle_signal(int sig) {
    printf("\nDisconnecting from server...\n");
    cleanup_client();
    exit(0);
}

/* Function to start and run the socket client */
int start_socket_client(const char *ip_address, int port, void (*signal_handler)(int)) {
    /* Store server IP */
    strncpy(server_ip, ip_address, sizeof(server_ip) - 1);
    server_ip[sizeof(server_ip) - 1] = '\0';

    /* Initialize signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct sockaddr_in server_addr;

    /* Create socket */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    /* Convert IP address from string to binary form */
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        return -1;
    }

    /* Connect to server */
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Connected to server at %s:%d\n", ip_address, port);

    /* Handle user input */
    handle_user_input();

    return 0;
}

int main(int argc, char *argv[]) {
    /* Check command line arguments */
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        printf("Example: %s 192.168.1.100\n", argv[0]);
        exit(1);
    }

    return start_socket_client(argv[1], SERVER_PORT, handle_signal);
}

// Handle user input and server responses 
void handle_user_input(void) {
    char command[MAX_COMMAND_LENGTH];
    char buffer[BUFFER_SIZE];
    
     //Receive welcome message 
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE-1, 0);
    printf("%s", buffer);

    while (1) {
        /* Get user input */
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            break;
        }

        /* Check for exit command */
        if (strncmp(command, "exit", 4) == 0) {
            printf("Disconnecting from server...\n");
            break;
        }

        /* Send command to server */
        send(client_socket, command, strlen(command), 0);

        /* Receive server response */
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE-1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected\n");
            break;
        }

        /* Print server response */
        printf("%s", buffer);
    }
}

/* Clean up client resources */
void cleanup_client(void) {
    close(client_socket);
}
