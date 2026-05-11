#include "server.h"

// Defining global synchronization primitives
pthread_mutex_t log_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fifo_mutex  = PTHREAD_MUTEX_INITIALIZER;
char g_signal_payload[BUFFER_SIZE];

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_ctx_t;

static void *client_thread(void *arg) {
    client_ctx_t *ctx = (client_ctx_t *)arg;
    int sock = ctx->client_socket;

    pthread_detach(pthread_self());

    char role[BUFFER_SIZE]     = {0};
    char username[BUFFER_SIZE] = {0};
    char password[BUFFER_SIZE] = {0};

    // Receive initial credentials from client
    if (recv(sock, role,     BUFFER_SIZE, 0) <= 0) goto done;
    if (recv(sock, username, BUFFER_SIZE, 0) <= 0) goto done;
    if (recv(sock, password, BUFFER_SIZE, 0) <= 0) goto done;

    role[strcspn(role, "\n")]         = '\0';
    username[strcspn(username, "\n")] = '\0';
    password[strcspn(password, "\n")] = '\0';

    char loglogin[BUFFER_SIZE];

    // Role-based authentication logic
    if (strcmp(role, "1") == 0) { // Admin
        if (strcmp(username, ADMIN_USERNAME) == 0 && strcmp(password, ADMIN_PASSWORD) == 0) {
            send_message(sock, "Admin login successful.\n");
            snprintf(loglogin, sizeof(loglogin), "LOGIN admin (%.80s)", username);
            log_action(loglogin);
            handle_admin(sock);
            log_action("LOGOUT admin");
        } else {
            send_message(sock, "Invalid admin credentials.\n");
            snprintf(loglogin, sizeof(loglogin), "FAILED LOGIN admin (%.80s)", username);
            log_action(loglogin);
        }

    } else if (strcmp(role, "2") == 0) { // Faculty
        if (validate_user(F_FACULTIES, username, password)) {
            send_message(sock, "Faculty login successful.\n");
            snprintf(loglogin, sizeof(loglogin), "LOGIN faculty (%.80s)", username);
            log_action(loglogin);
            handle_faculty(sock, username);
            snprintf(loglogin, sizeof(loglogin), "LOGOUT faculty (%.80s)", username);
            log_action(loglogin);
        } else {
            send_message(sock, "Invalid faculty credentials.\n");
            snprintf(loglogin, sizeof(loglogin), "FAILED LOGIN faculty (%.80s)", username);
            log_action(loglogin);
        }

    } else if (strcmp(role, "3") == 0) { // Student
        if (validate_user(F_STUDENTS, username, password)) {
            send_message(sock, "Student login successful.\n");
            snprintf(loglogin, sizeof(loglogin), "LOGIN student (%.80s)", username);
            log_action(loglogin);
            handle_student(sock, username);
            snprintf(loglogin, sizeof(loglogin), "LOGOUT student (%.80s)", username);
            log_action(loglogin);
        } else {
            send_message(sock, "Invalid student credentials.\n");
            snprintf(loglogin, sizeof(loglogin), "FAILED LOGIN student (%.80s)", username);
            log_action(loglogin);
        }
    } else {
        send_message(sock, "Invalid role selected.\n");
    }

done:
    close(sock);
    free(ctx);
    return NULL;
}

int main(void) {
    // Ignore SIGPIPE to prevent server crash if a client disconnects unexpectedly
    signal(SIGPIPE, SIG_IGN);

    // Setup Signal Handler for IPC (SIGUSR1)
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    // Create Named Pipe (FIFO) for IPC
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0644) < 0 && errno != EEXIST) {
        perror("mkfifo");
    }

    // Start background thread to read from FIFO
    pthread_t fifo_tid;
    pthread_create(&fifo_tid, NULL, fifo_reader_thread, NULL);
    pthread_detach(fifo_tid);

    // Create Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Bind to Port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); 
        close(server_fd); 
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen"); 
        close(server_fd); 
        exit(EXIT_FAILURE);
    }

    printf("Academia Portal server running on port %d ...\n", PORT);
    log_action("SERVER START");

    // Main Acceptance Loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) continue;

        client_ctx_t *ctx = (client_ctx_t *)malloc(sizeof(*ctx));
        if (!ctx) { close(client_socket); continue; }
        ctx->client_socket = client_socket;
        ctx->client_addr   = client_addr;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
            close(client_socket);
            free(ctx);
            continue;
        }
    }

    close(server_fd);
    return 0;
}