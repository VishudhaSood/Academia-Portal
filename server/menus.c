#include "server.h"

// This file contains all the menu handling functions for the different user roles, which are responsible for
// Admin handler function that displays the admin menu and processes admin actions
void handle_admin(int client_socket) {
    char choice[BUFFER_SIZE];
    while (1) {
        send_message(client_socket,
                     "\n--- Admin Menu ---\n"
                     "1. Add Student\n"
                     "2. Add Faculty\n"
                     "3. View Users\n"
                     "4. View All Courses\n"
                     "5. Logout\n"
                     "Enter choice: ");

        memset(choice, 0, sizeof(choice));
        if (recv(client_socket, choice, BUFFER_SIZE, 0) <= 0) return;
        choice[strcspn(choice, "\n")] = '\0';

        if      (strcmp(choice, "1") == 0) add_user(F_STUDENTS, client_socket);
        else if (strcmp(choice, "2") == 0) add_user(F_FACULTIES, client_socket);
        else if (strcmp(choice, "3") == 0) view_users(client_socket);
        else if (strcmp(choice, "4") == 0) view_courses_all(client_socket);
        else if (strcmp(choice, "5") == 0) {
            send_message(client_socket, "Logging out...\n");
            break;
        } else send_message(client_socket, "Invalid choice.\n");
    }
}

// Faculty handler function that displays the faculty menu and processes faculty actions
void handle_faculty(int client_socket, const char *faculty_name) {
    char choice[BUFFER_SIZE];
    while (1) {
        send_message(client_socket,
                     "\n--- Faculty Menu ---\n"
                     "1. View My Courses\n"
                     "2. View All Courses\n"
                     "3. Add Course\n"
                     "4. Logout\n"
                     "Enter choice: ");

        memset(choice, 0, sizeof(choice));
        if (recv(client_socket, choice, BUFFER_SIZE, 0) <= 0) return;
        choice[strcspn(choice, "\n")] = '\0';

        if      (strcmp(choice, "1") == 0) view_courses_faculty(client_socket, faculty_name);
        else if (strcmp(choice, "2") == 0) view_courses_all(client_socket);
        else if (strcmp(choice, "3") == 0) add_course(client_socket, faculty_name);
        else if (strcmp(choice, "4") == 0) {
            send_message(client_socket, "Logging out...\n");
            break;
        } else send_message(client_socket, "Invalid choice.\n");
    }
}

// Student handler function that displays the student menu and processes student actions
void handle_student(int client_socket, const char *student_name) {
    char choice[BUFFER_SIZE];
    while (1) {
        send_message(client_socket,
                     "\n--- Student Menu ---\n"
                     "1. View All Courses\n"
                     "2. Enroll in Course\n"
                     "3. Drop Course\n"
                     "4. View My Enrollments\n"
                     "5. Logout\n"
                     "Enter choice: ");

        memset(choice, 0, sizeof(choice));
        if (recv(client_socket, choice, BUFFER_SIZE, 0) <= 0) return;
        choice[strcspn(choice, "\n")] = '\0';

        if      (strcmp(choice, "1") == 0) view_courses_all(client_socket);
        else if (strcmp(choice, "2") == 0) enroll_course(client_socket, student_name);
        else if (strcmp(choice, "3") == 0) drop_course(client_socket, student_name);
        else if (strcmp(choice, "4") == 0) view_my_enrollments(client_socket, student_name);
        else if (strcmp(choice, "5") == 0) {
            send_message(client_socket, "Logging out...\n");
            break;
        } else send_message(client_socket, "Invalid choice.\n");
    }
}