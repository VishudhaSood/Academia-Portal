#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define PORT          8080
#define BUFFER_SIZE   4096
#define MAX_CLIENTS   64

#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "admin123"

#define FIFO_PATH      "data/notify.fifo"

// File paths
#define F_STUDENTS     "data/students.txt"
#define F_FACULTIES    "data/faculties.txt"
#define F_COURSES      "data/courses.txt"
#define F_ENROLLMENTS  "data/enrollments.txt"
#define F_WAITLIST     "data/waitlist.txt"
#define F_LOGS         "data/logs.txt"

// Global sync primitives & IPC variables (defined in server.c)
extern pthread_mutex_t log_mutex;
extern pthread_mutex_t fifo_mutex;
extern char g_signal_payload[BUFFER_SIZE];

// utils.c
int lock_fd(int fd, short type);
int unlock_fd(int fd);
void write_newline(int fd);
void send_message(int socket, const char *message);
void log_action(const char *action);
void sigusr1_handler(int signo);
void *fifo_reader_thread(void *arg);
void ipc_notify(const char *msg);

// db.c
int validate_user(const char *filename, const char *username, const char *password);
int user_exists(const char *filename, const char *username);
void add_user(const char *filename, int client_socket);
void view_users(int client_socket);
int course_exists(const char *course_code);
void add_course(int client_socket, const char *faculty_name);
void view_courses_all(int client_socket);
void view_courses_faculty(int client_socket, const char *faculty_name);
int already_enrolled(const char *student, const char *course);
int append_record(const char *path, const char *student, const char *course);
int update_course_enrolled(const char *course_code, int new_count);
int get_course_seats(const char *course_code, int *capacity, int *enrolled);
int pop_waitlist_for(const char *course_code, char *out_student);
int remove_enrollment(const char *student, const char *course);
void enroll_course(int client_socket, const char *student_name);
void drop_course(int client_socket, const char *student_name);
void view_my_enrollments(int client_socket, const char *student_name);

// menus.c
void handle_admin(int client_socket);
void handle_faculty(int client_socket, const char *faculty_name);
void handle_student(int client_socket, const char *student_name);

#endif