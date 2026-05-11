#include "server.h"

//locking utility functions for file-based databases, message sending, logging, signal handling, and IPC via FIFO
int lock_fd(int fd, short type) {
    struct flock fl;
    fl.l_type   = type;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fl.l_pid    = 0;
    return fcntl(fd, F_SETLKW, &fl);
}

// Unlocks a previously locked file descriptor
int unlock_fd(int fd) {
    struct flock fl;
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fl.l_pid    = 0;
    return fcntl(fd, F_SETLK, &fl);
}

// Writes a newline character to the given file descriptor
void write_newline(int fd) {
    ssize_t w = write(fd, "\n", 1);
    (void)w;
}

// Sends a message to the client through the socket, ensuring it is null-terminated and fits within the buffer size
void send_message(int socket, const char *message) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    size_t mlen = strlen(message);
    if (mlen >= BUFFER_SIZE) mlen = BUFFER_SIZE - 1;
    memcpy(buffer, message, mlen);
    send(socket, buffer, BUFFER_SIZE, 0);
}

// Logs an action to the logs file with a timestamp, ensuring thread safety with a mutex lock
void log_action(const char *action) {
    pthread_mutex_lock(&log_mutex);

    int fd = open(F_LOGS, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    if (lock_fd(fd, F_WRLCK) == 0) {
        time_t now = time(NULL);
        char ts[64];
        strncpy(ts, ctime(&now), sizeof(ts) - 1);
        ts[sizeof(ts) - 1] = '\0';
        ts[strcspn(ts, "\n")] = '\0';

        dprintf(fd, "[%s] %s\n", ts, action);
        unlock_fd(fd);
    }
    close(fd);
    pthread_mutex_unlock(&log_mutex);
}

// Signal handler for SIGUSR1 that reads the payload from the global variable and writes it to the FIFO for IPC notifications
void sigusr1_handler(int signo) {
    (void)signo;
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;
    ssize_t w = write(fd, g_signal_payload, strlen(g_signal_payload));
    (void)w;
    close(fd);
}

// Background thread function that continuously reads from the FIFO and logs any messages received for IPC notifications
void *fifo_reader_thread(void *arg) {
    (void)arg;
    char buf[BUFFER_SIZE];
    while (1) {
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd < 0) { sleep(1); continue; }

        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            buf[strcspn(buf, "\n")] = '\0';
            char entry[BUFFER_SIZE + 64];
            snprintf(entry, sizeof(entry), "[IPC/FIFO] %s", buf);
            log_action(entry);
        }
        close(fd);
    }
    return NULL;
}

// Sends an IPC notification by setting the global signal payload and raising SIGUSR1 to trigger the signal handler
void ipc_notify(const char *msg) {
    pthread_mutex_lock(&fifo_mutex);
    snprintf(g_signal_payload, sizeof(g_signal_payload), "%s", msg);
    raise(SIGUSR1);
    usleep(1000);
    pthread_mutex_unlock(&fifo_mutex);
}