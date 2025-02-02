/*
 * BSD Socket Database Server
 * This server runs on the team lead's computer and handles all database operations.
 * It uses BSD sockets for network communication and implements the database commands.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define DATABASE_PATH "./data/"

/* Structure to hold client information */
typedef struct {
    int socket;
    struct sockaddr_in address;
    char current_database[256];
} client_t;

/* Global variables */
client_t clients[MAX_CLIENTS];
int server_socket;

/* Function prototypes */
void initialize_server(void);
void handle_client_connection(int client_socket, struct sockaddr_in client_addr);
void process_command(int client_socket, char *command);
void create_database(int client_socket, char *db_name);
void use_database(int client_socket, char *db_name, char *current_db);
void create_table(int client_socket, char *command, char *current_db);
void show_databases(int client_socket);
void show_tables(int client_socket, char *current_db);
void cleanup_server(void);

/* Signal handler for graceful shutdown */
void handle_signal(int sig) {
    printf("\nShutting down server...\n");
    cleanup_server();
    exit(0);
}

int main(void) {
    /* Initialize signal handlers */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* Initialize server */
    initialize_server();

    /* Create data directory if it doesn't exist */
    mkdir(DATABASE_PATH, 0777);

    printf("Server is running on port %d...\n", PORT);

    /* Main server loop */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        /* Accept new client connection */
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        /* Handle client in a new process */
        pid_t pid = fork();
        if (pid == 0) {
            /* Child process */
            close(server_socket);
            handle_client_connection(client_socket, client_addr);
            exit(0);
        } else {
            /* Parent process */
            close(client_socket);
        }
    }

    return 0;
}

/* Initialize the server socket and bind to port */
void initialize_server(void) {
    struct sockaddr_in server_addr;

    /* Create socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    /* Set socket options */
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    /* Listen for connections */
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }
}

/* Handle individual client connection */
void handle_client_connection(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char current_database[256] = "";
    
    /* Send welcome message */
    char *welcome = "Connected to TOS Database Server\nTOS> ";
    send(client_socket, welcome, strlen(welcome), 0);

    while (1) {
        /* Receive command from client */
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE-1, 0);
        
        if (bytes_received <= 0) {
            printf("Client disconnected: %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            break;
        }

        /* Process the command */
        buffer[bytes_received] = '\0';
        process_command(client_socket, buffer);

        /* Send prompt */
        send(client_socket, "TOS> ", 5, 0);
    }

    close(client_socket);
}

/* Process client commands */
void process_command(int client_socket, char *command) {
    char cmd_copy[BUFFER_SIZE];
    strcpy(cmd_copy, command);
    
    /* Parse command */
    char *token = strtok(cmd_copy, " \n");
    if (!token) return;

    /* Handle different commands */
    if (strcmp(token, "make") == 0) {
        token = strtok(NULL, " \n");
        if (!token) {
            send(client_socket, "Error: Invalid command syntax\n", 29, 0);
            return;
        }

        if (strcmp(token, "db") == 0) {
            token = strtok(NULL, " \n");
            if (token) {
                create_database(client_socket, token);
            } else {
                send(client_socket, "Error: Database name required\n", 29, 0);
            }
        } else if (strcmp(token, "table") == 0) {
            create_table(client_socket, command, clients[client_socket].current_database);
        }
    }
    /* Add other commands here */
}

/* Create a new database */
void create_database(int client_socket, char *db_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", DATABASE_PATH, db_name);

    /* Create database directory */
    if (mkdir(path, 0777) == 0) {
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Database '%s' created successfully\n", db_name);
        send(client_socket, response, strlen(response), 0);
    } else {
        if (errno == EEXIST) {
            send(client_socket, "Error: Database already exists\n", 30, 0);
        } else {
            send(client_socket, "Error: Could not create database\n", 32, 0);
        }
    }
}

/* Clean up server resources */
void cleanup_server(void) {
    close(server_socket);
}
