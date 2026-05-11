#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT        8080
#define BUFFER_SIZE 4096

// A server message asks for input if it contains any of these substrings.
// Centralising this list avoids the tangled nested loops that lived in
// the previous version of the client.
static int prompts_for_input(const char *msg) {
    static const char *triggers[] = {
        "Enter choice:",
        "Enter username:",
        "Enter password:",
        "Enter course code:",
        "Enter course code to drop:",
        "Enter capacity",
        NULL
    };
    for (int i = 0; triggers[i]; i++) {
        if (strstr(msg, triggers[i])) return 1;
    }
    return 0;
}

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); close(sock); exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect"); close(sock); exit(EXIT_FAILURE);
    }

    printf("Connected to Academia Portal.\n");

    //  Login 
    char role[BUFFER_SIZE]     = {0};
    char username[BUFFER_SIZE] = {0};
    char password[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};

    printf("\nSelect Role:\n");
    printf("  1. Admin\n");
    printf("  2. Faculty\n");
    printf("  3. Student\n");
    printf("Enter choice: ");
    if (!fgets(role, BUFFER_SIZE, stdin)) goto end;
    role[strcspn(role, "\n")] = '\0';

    printf("Enter username: ");
    if (!fgets(username, BUFFER_SIZE, stdin)) goto end;
    username[strcspn(username, "\n")] = '\0';

    printf("Enter password: ");
    if (!fgets(password, BUFFER_SIZE, stdin)) goto end;
    password[strcspn(password, "\n")] = '\0';

    send(sock, role,     BUFFER_SIZE, 0);
    send(sock, username, BUFFER_SIZE, 0);
    send(sock, password, BUFFER_SIZE, 0);

    if (recv(sock, response, BUFFER_SIZE, 0) <= 0) goto end;
    printf("\nServer: %s\n", response);

    if (!strstr(response, "successful")) goto end;

    // Session loop
    char msg[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    while (1) {
        memset(msg, 0, sizeof(msg));
        ssize_t n = recv(sock, msg, BUFFER_SIZE, 0);
        if (n <= 0) break;

        printf("%s", msg);
        fflush(stdout);

        if (strstr(msg, "Logging out")) break;

        if (prompts_for_input(msg)) {
            if (!fgets(input, BUFFER_SIZE, stdin)) break;
            input[strcspn(input, "\n")] = '\0';
            send(sock, input, BUFFER_SIZE, 0);
        }
        // else: informational message — loop back and wait for next prompt.
    }

end:
    close(sock);
    return 0;
}