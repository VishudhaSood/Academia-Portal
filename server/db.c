#include "server.h"

// This file contains all the database-related functions, which are responsible for
int validate_user(const char *filename, const char *username, const char *password) { // Returns 1 if valid, 0 otherwise
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); return 0; }

    FILE *file = fdopen(fd, "r");
    if (!file) { unlock_fd(fd); close(fd); return 0; }

    char line[BUFFER_SIZE];
    int  ok = 0;

    while (fgets(line, sizeof(line), file)) {
        char stored_user[BUFFER_SIZE];
        char stored_pass[BUFFER_SIZE];
        line[strcspn(line, "\n")] = '\0';

        if (sscanf(line, "%[^:]:%s", stored_user, stored_pass) == 2) {
            if (strcmp(username, stored_user) == 0 && strcmp(password, stored_pass) == 0) {
                ok = 1;
                break;
            }
        }
    }

    unlock_fd(fd);
    fclose(file); 
    return ok;
}

// Checks if a username already exists in either students or faculties file
int user_exists(const char *filename, const char *username) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); return 0; }

    FILE *file = fdopen(fd, "r");
    if (!file) { unlock_fd(fd); close(fd); return 0; }

    char line[BUFFER_SIZE];
    int  found = 0;
    while (fgets(line, sizeof(line), file)) {
        char stored_user[BUFFER_SIZE];
        line[strcspn(line, "\n")] = '\0';
        if (sscanf(line, "%[^:]", stored_user) == 1) {
            if (strcmp(username, stored_user) == 0) { found = 1; break; }
        }
    }

    unlock_fd(fd);
    fclose(file);
    return found;
}

//
void add_user(const char *filename, int client_socket) { // adds a user to either students or faculties file based on filename argument
    char username[BUFFER_SIZE] = {0};
    char password[BUFFER_SIZE] = {0};

    send_message(client_socket, "Enter username: ");
    recv(client_socket, username, BUFFER_SIZE, 0);
    username[strcspn(username, "\n")] = '\0';

    send_message(client_socket, "Enter password: ");
    recv(client_socket, password, BUFFER_SIZE, 0);
    password[strcspn(password, "\n")] = '\0';

    // Basic validation
    if (strlen(username) == 0 || strlen(password) == 0) {
        send_message(client_socket, "Username/password cannot be empty.\n");
        return;
    }

    if (user_exists(F_STUDENTS, username) || user_exists(F_FACULTIES, username)) {
        send_message(client_socket, "Username already exists.\n");
        return;
    }

    int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        send_message(client_socket, "Error opening file.\n");
        return;
    }

    if (lock_fd(fd, F_WRLCK) != 0) {
        close(fd);
        send_message(client_socket, "Could not acquire file lock.\n");
        return;
    }

    // Ensure file ends with a newline before appending
    off_t end = lseek(fd, 0, SEEK_END);
    if (end > 0) {
        char last;
        if (pread(fd, &last, 1, end - 1) == 1 && last != '\n') {
            write_newline(fd);
        }
    }
    dprintf(fd, "%s:%s\n", username, password);

    unlock_fd(fd);
    close(fd);

    // Log the new user addition and notify any waiting clients via IPC
    char logmsg[BUFFER_SIZE];
    snprintf(logmsg, sizeof(logmsg), "New user added: %.100s", username);
    log_action(logmsg);
    send_message(client_socket, "User added successfully.\n");
}

// Views all students and faculties (admin only)
void view_users(int client_socket) {
    char output[BUFFER_SIZE * 4];
    output[0] = '\0';

    strcat(output, "\n--- Students ---\n");
    int fd = open(F_STUDENTS, O_RDONLY);
    if (fd >= 0) {
        if (lock_fd(fd, F_RDLCK) == 0) {
            FILE *f = fdopen(fd, "r");
            if (f) {
                char line[512];
                while (fgets(line, sizeof(line), f)) {
                    strncat(output, line, sizeof(output) - strlen(output) - 2);
                    if (line[strlen(line) - 1] != '\n') strcat(output, "\n");
                }
                unlock_fd(fd);
                fclose(f);
            } else { unlock_fd(fd); close(fd); }
        } else close(fd);
    }

    strcat(output, "\n--- Faculties ---\n");
    fd = open(F_FACULTIES, O_RDONLY);
    if (fd >= 0) {
        if (lock_fd(fd, F_RDLCK) == 0) {
            FILE *f = fdopen(fd, "r");
            if (f) {
                char line[512];
                while (fgets(line, sizeof(line), f)) {
                    strncat(output, line, sizeof(output) - strlen(output) - 2);
                    if (line[strlen(line) - 1] != '\n') strcat(output, "\n");
                }
                unlock_fd(fd);
                fclose(f);
            } else { unlock_fd(fd); close(fd); }
        } else close(fd);
    }
    send_message(client_socket, output);
}

// Checks if a course code already exists in the courses file
int course_exists(const char *course_code) {
    int fd = open(F_COURSES, O_RDONLY);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); return 0; }
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); return 0; }

    char line[BUFFER_SIZE];
    int  found = 0;
    while (fgets(line, sizeof(line), f)) {
        char code[100];
        if (sscanf(line, "%[^:]", code) == 1) {
            if (strcmp(code, course_code) == 0) { found = 1; break; }
        }
    }
    unlock_fd(fd);
    fclose(f);
    return found;
}

// Adds a new course to the courses file with the given faculty as owner and specified capacity
void add_course(int client_socket, const char *faculty_name) {
    char course[BUFFER_SIZE]  = {0};
    char cap_str[BUFFER_SIZE] = {0};

    send_message(client_socket, "Enter course code: ");
    recv(client_socket, course, BUFFER_SIZE, 0);
    course[strcspn(course, "\n")] = '\0';

    send_message(client_socket, "Enter capacity (default 3): ");
    recv(client_socket, cap_str, BUFFER_SIZE, 0);
    cap_str[strcspn(cap_str, "\n")] = '\0';

    int capacity = atoi(cap_str);
    if (capacity <= 0) capacity = 3;

    if (strlen(course) == 0) {
        send_message(client_socket, "Course code cannot be empty.\n");
        return;
    }
    if (course_exists(course)) {
        send_message(client_socket, "Course already exists.\n");
        return;
    }

    int fd = open(F_COURSES, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        send_message(client_socket, "Error opening courses file.\n");
        return;
    }
    if (lock_fd(fd, F_WRLCK) != 0) {
        close(fd);
        send_message(client_socket, "Could not lock courses file.\n");
        return;
    }

    off_t end = lseek(fd, 0, SEEK_END);
    if (end > 0) {
        char last;
        if (pread(fd, &last, 1, end - 1) == 1 && last != '\n') {
            write_newline(fd);
        }
    }
    dprintf(fd, "%s:%s:%d:0\n", course, faculty_name, capacity);

    unlock_fd(fd);
    close(fd);

    char logmsg[BUFFER_SIZE];
    snprintf(logmsg, sizeof(logmsg), "Course added: %.80s by %.80s (cap=%d)", course, faculty_name, capacity);
    log_action(logmsg);
    send_message(client_socket, "Course added successfully.\n");
}

// Views all courses with their faculty and seat availability
void view_courses_all(int client_socket) {
    int fd = open(F_COURSES, O_RDONLY);
    if (fd < 0) { send_message(client_socket, "No courses available.\n"); return; }
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); send_message(client_socket, "Could not read courses.\n"); return; }
    
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); send_message(client_socket, "Could not read courses.\n"); return; }

    char output[BUFFER_SIZE * 4];
    output[0] = '\0';
    strcat(output, "\n--- Courses ---\n");

    char line[512];
    int  any = 0;
    while (fgets(line, sizeof(line), f)) {
        char code[100], faculty[100];
        int  capacity, enrolled;
        if (sscanf(line, "%[^:]:%[^:]:%d:%d", code, faculty, &capacity, &enrolled) == 4) {
            char row[256];
            snprintf(row, sizeof(row), "%s | Faculty: %s | Seats: %d/%d\n", code, faculty, enrolled, capacity);
            strncat(output, row, sizeof(output) - strlen(output) - 2);
            any = 1;
        }
    }
    if (!any) strcat(output, "(none)\n");

    unlock_fd(fd);
    fclose(f);
    send_message(client_socket, output);
}

// Views courses owned by a specific faculty with seat availability
void view_courses_faculty(int client_socket, const char *faculty_name) {
    int fd = open(F_COURSES, O_RDONLY);
    if (fd < 0) { send_message(client_socket, "No courses available.\n"); return; }
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); send_message(client_socket, "Could not read courses.\n"); return; }
    
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); return; }

    char output[BUFFER_SIZE * 4];
    output[0] = '\0';
    snprintf(output, sizeof(output), "\n--- Courses by %s ---\n", faculty_name);

    char line[512];
    int  any = 0;
    while (fgets(line, sizeof(line), f)) {
        char code[100], fac[100];
        int  capacity, enrolled;
        if (sscanf(line, "%[^:]:%[^:]:%d:%d", code, fac, &capacity, &enrolled) == 4) {
            if (strcmp(fac, faculty_name) == 0) {
                char row[256];
                snprintf(row, sizeof(row), "%s | Seats: %d/%d\n", code, enrolled, capacity);
                strncat(output, row, sizeof(output) - strlen(output) - 2);
                any = 1;
            }
        }
    }
    if (!any) strcat(output, "(no courses owned by you)\n");

    unlock_fd(fd);
    fclose(f);
    send_message(client_socket, output);
}

// Checks if a student is already enrolled in a course
int already_enrolled(const char *student, const char *course) {
    int fd = open(F_ENROLLMENTS, O_RDONLY);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); return 0; }
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); return 0; }

    char line[BUFFER_SIZE];
    int  found = 0;
    while (fgets(line, sizeof(line), f)) {
        char s[100], c[100];
        line[strcspn(line, "\n")] = '\0';
        if (sscanf(line, "%[^:]:%s", s, c) == 2) {
            if (strcmp(s, student) == 0 && strcmp(c, course) == 0) { found = 1; break; }
        }
    }
    unlock_fd(fd);
    fclose(f);
    return found;
}

// Appends a new enrollment record to the enrollments file
int append_record(const char *path, const char *student, const char *course) {
    int fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1;
    if (lock_fd(fd, F_WRLCK) != 0) { close(fd); return -1; }

    off_t end = lseek(fd, 0, SEEK_END);
    if (end > 0) {
        char last;
        if (pread(fd, &last, 1, end - 1) == 1 && last != '\n') write_newline(fd);
    }
    dprintf(fd, "%s:%s\n", student, course);

    unlock_fd(fd);
    close(fd);
    return 0;
}

// Updates the enrolled count for a course in the courses file
int update_course_enrolled(const char *course_code, int new_count) {
    int fd = open(F_COURSES, O_RDWR);
    if (fd < 0) return -1;
    if (lock_fd(fd, F_WRLCK) != 0) { close(fd); return -1; }

    FILE *src = fdopen(fd, "r");
    if (!src) { unlock_fd(fd); close(fd); return -1; }
    rewind(src);

    FILE *tmp = fopen("data/courses.tmp", "w");
    if (!tmp) { unlock_fd(fd); fclose(src); return -1; }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), src)) {
        char code[100], faculty[100];
        int  cap, enrolled;
        if (sscanf(line, "%[^:]:%[^:]:%d:%d", code, faculty, &cap, &enrolled) == 4) {
            if (strcmp(code, course_code) == 0) {
                fprintf(tmp, "%s:%s:%d:%d\n", code, faculty, cap, new_count);
            } else {
                fprintf(tmp, "%s:%s:%d:%d\n", code, faculty, cap, enrolled);
            }
        }
    }
    fclose(tmp);
    rename("data/courses.tmp", F_COURSES);

    unlock_fd(fd);
    fclose(src);
    return 0;
}

// Retrieves the capacity and enrolled count for a course
int get_course_seats(const char *course_code, int *capacity, int *enrolled) {
    int fd = open(F_COURSES, O_RDONLY);
    if (fd < 0) return -1;
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); return -1; }
    
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); return -1; }

    char line[BUFFER_SIZE];
    int  found = 0;
    while (fgets(line, sizeof(line), f)) {
        char code[100], faculty[100];
        int  cap, en;
        if (sscanf(line, "%[^:]:%[^:]:%d:%d", code, faculty, &cap, &en) == 4) {
            if (strcmp(code, course_code) == 0) {
                *capacity = cap; *enrolled = en;
                found = 1; break;
            }
        }
    }
    unlock_fd(fd);
    fclose(f);
    return found ? 0 : -1;
}

// Pops the first student from the waitlist for a course and returns their name in out_student. Returns 1 if a student was promoted, 0 otherwise.
int pop_waitlist_for(const char *course_code, char *out_student) {
    int fd = open(F_WAITLIST, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_WRLCK) != 0) { close(fd); return 0; }

    FILE *src = fdopen(fd, "r");
    if (!src) { unlock_fd(fd); close(fd); return 0; }
    rewind(src);

    FILE *tmp = fopen("data/waitlist.tmp", "w");
    if (!tmp) { unlock_fd(fd); fclose(src); return 0; }

    char line[BUFFER_SIZE];
    int  promoted = 0;
    while (fgets(line, sizeof(line), src)) {
        char s[100], c[100];
        line[strcspn(line, "\n")] = '\0';

        if (sscanf(line, "%[^:]:%s", s, c) == 2) {
            if (!promoted && strcmp(c, course_code) == 0) {
                strcpy(out_student, s);
                promoted = 1;
                continue; 
            }
            fprintf(tmp, "%s:%s\n", s, c);
        }
    }
    fclose(tmp);
    rename("data/waitlist.tmp", F_WAITLIST);

    unlock_fd(fd);
    fclose(src);
    return promoted;
}

// Removes an enrollment record for a student and course. Returns 1 if removed, 0 if not found.
int remove_enrollment(const char *student, const char *course) {
    int fd = open(F_ENROLLMENTS, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return 0;
    if (lock_fd(fd, F_WRLCK) != 0) { close(fd); return 0; }

    FILE *src = fdopen(fd, "r");
    if (!src) { unlock_fd(fd); close(fd); return 0; }
    rewind(src);

    FILE *tmp = fopen("data/enrollments.tmp", "w");
    if (!tmp) { unlock_fd(fd); fclose(src); return 0; }

    char line[BUFFER_SIZE];
    int  removed = 0;
    while (fgets(line, sizeof(line), src)) {
        char s[100], c[100];
        line[strcspn(line, "\n")] = '\0';
        if (sscanf(line, "%[^:]:%s", s, c) == 2) {
            if (!removed && strcmp(s, student) == 0 && strcmp(c, course) == 0) {
                removed = 1;
                continue;
            }
            fprintf(tmp, "%s:%s\n", s, c);
        }
    }
    fclose(tmp);
    rename("data/enrollments.tmp", F_ENROLLMENTS);

    unlock_fd(fd);
    fclose(src);
    return removed;
}

// Enrolls a student in a course if seats are available, otherwise adds them to the waitlist
void enroll_course(int client_socket, const char *student_name) {
    char course_code[BUFFER_SIZE] = {0};

    send_message(client_socket, "Enter course code: ");
    recv(client_socket, course_code, BUFFER_SIZE, 0);
    course_code[strcspn(course_code, "\n")] = '\0';

    if (!course_exists(course_code)) {
        send_message(client_socket, "Course not found.\n");
        return;
    }
    if (already_enrolled(student_name, course_code)) {
        send_message(client_socket, "You are already enrolled in this course.\n");
        return;
    }

    int capacity = 0, enrolled = 0;
    if (get_course_seats(course_code, &capacity, &enrolled) < 0) {
        send_message(client_socket, "Could not read seat data.\n");
        return;
    }

    if (enrolled < capacity) {
        if (update_course_enrolled(course_code, enrolled + 1) < 0 || append_record(F_ENROLLMENTS, student_name, course_code) < 0) {
            send_message(client_socket, "Enrollment failed.\n");
            return;
        }
        char logmsg[BUFFER_SIZE];
        snprintf(logmsg, sizeof(logmsg), "%.80s enrolled in %.80s", student_name, course_code);
        log_action(logmsg);
        send_message(client_socket, "Enrollment successful.\n");
    } else {
        if (append_record(F_WAITLIST, student_name, course_code) < 0) {
            send_message(client_socket, "Waitlist add failed.\n");
            return;
        }
        char logmsg[BUFFER_SIZE];
        snprintf(logmsg, sizeof(logmsg), "%.80s waitlisted for %.80s", student_name, course_code);
        log_action(logmsg);
        send_message(client_socket, "Course full. Added to waitlist.\n");
    }
}

// Drops a course for a student, updates enrolled count, and promotes from waitlist if applicable
void drop_course(int client_socket, const char *student_name) {
    char course_code[BUFFER_SIZE] = {0};

    send_message(client_socket, "Enter course code to drop: ");
    recv(client_socket, course_code, BUFFER_SIZE, 0);
    course_code[strcspn(course_code, "\n")] = '\0';

    if (!already_enrolled(student_name, course_code)) {
        send_message(client_socket, "You are not enrolled in that course.\n");
        return;
    }

    if (!remove_enrollment(student_name, course_code)) {
        send_message(client_socket, "Drop failed.\n");
        return;
    }

    int capacity = 0, enrolled = 0;
    if (get_course_seats(course_code, &capacity, &enrolled) == 0) {
        char promoted[100] = {0};
        if (pop_waitlist_for(course_code, promoted)) {
            update_course_enrolled(course_code, enrolled); 
            append_record(F_ENROLLMENTS, promoted, course_code);

            char logmsg[BUFFER_SIZE];
            snprintf(logmsg, sizeof(logmsg), "%.80s dropped %.80s; auto-promoted %.80s", student_name, course_code, promoted);
            log_action(logmsg);

            char ipcmsg[BUFFER_SIZE * 2 + 100];
            snprintf(ipcmsg, sizeof(ipcmsg), "Auto-promotion: %.50s -> %.50s", promoted, course_code);
            ipc_notify(ipcmsg);
        } else {
            if (enrolled > 0) update_course_enrolled(course_code, enrolled - 1);

            char logmsg[BUFFER_SIZE];
            snprintf(logmsg, sizeof(logmsg), "%.80s dropped %.80s", student_name, course_code);
            log_action(logmsg);
        }
    }
    send_message(client_socket, "Course dropped successfully.\n");
}

// Views all courses a student is enrolled in
void view_my_enrollments(int client_socket, const char *student_name) {
    int fd = open(F_ENROLLMENTS, O_RDONLY);
    if (fd < 0) { send_message(client_socket, "(no enrollments)\n"); return; }
    if (lock_fd(fd, F_RDLCK) != 0) { close(fd); send_message(client_socket, "Could not read enrollments.\n"); return; }
    
    FILE *f = fdopen(fd, "r");
    if (!f) { unlock_fd(fd); close(fd); return; }

    char output[BUFFER_SIZE * 2];
    snprintf(output, sizeof(output), "\n--- Your enrolled courses (%s) ---\n", student_name);

    char line[BUFFER_SIZE];
    int  any = 0;
    while (fgets(line, sizeof(line), f)) {
        char s[100], c[100];
        line[strcspn(line, "\n")] = '\0';
        if (sscanf(line, "%[^:]:%s", s, c) == 2) {
            if (strcmp(s, student_name) == 0) {
                strncat(output, c, sizeof(output) - strlen(output) - 2);
                strcat(output, "\n");
                any = 1;
            }
        }
    }
    if (!any) strcat(output, "(none)\n");

    unlock_fd(fd);
    fclose(f);
    send_message(client_socket, output);
}