#  Academia Portal  
### Smart Course Scheduling & Academic Management System

<div align="center">

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Linux](https://img.shields.io/badge/Platform-Linux-green.svg)
![Sockets](https://img.shields.io/badge/Networking-TCP%20Sockets-orange.svg)
![POSIX Threads](https://img.shields.io/badge/Concurrency-Pthreads-red.svg)
![OS Project](https://img.shields.io/badge/Project-Operating%20Systems-purple.svg)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen.svg)

</div>

---

##  Overview

**Academia Portal** is a multi-threaded, role-based academic management system developed in **C using Linux System Programming concepts**.  

The project simulates a real-world university portal where **Admins, Faculty members, and Students** interact securely through a **client-server architecture** supporting concurrency, synchronization, persistent storage, and inter-process communication.

The system was designed as an **Operating Systems Mini Project** to demonstrate practical implementation of core OS concepts in a scalable and modular academic application.

---

#  Features

##  Admin Module
- Add Students
- Add Faculty Members
- View Registered Users
- View All Courses

##  Faculty Module
- Add Courses
- Assign Course Capacities
- View Owned Courses
- View All Available Courses

##  Student Module
- Browse Courses
- Enroll in Courses
- Drop Courses
- View Personal Enrollments
- Automatic Waitlist Handling

---

#  Operating Systems Concepts Implemented

-  TCP Socket Programming
-  Multi-threading using POSIX Threads
-  File Locking using `fcntl()`
-  Synchronization using Mutexes
-  Inter-Process Communication (FIFO + Signals)
-  Persistent File-Based Storage
-  Concurrent Client Handling
-  Logging & Resource Management


---

#  Project Structure

```text
Academia-Portal/
│
├── client/
│   └── client.c
│
├── server/
│   ├── server.c
│   ├── db.c
│   ├── menus.c
│   ├── utils.c
│   └── server.h
│
├── data/
│   ├── students.txt
│   ├── faculties.txt
│   ├── courses.txt
│   ├── enrollments.txt
│   ├── waitlist.txt
│   └── logs.txt
│
├── Makefile
└── README.md
```

---

#  Tech Stack

| Category | Technology |
|---|---|
| Language | C |
| Platform | Linux |
| Networking | TCP Sockets |
| Concurrency | POSIX Threads |
| Synchronization | Mutex Locks |
| IPC | FIFO + Signals |
| Storage | File-Based Database |
| Build Tool | Makefile |

---

#  Authentication System

The portal supports **three user roles**:

| Role | Authentication Source |
|---|---|
| Admin | Hardcoded Credentials |
| Faculty | `faculties.txt` |
| Student | `students.txt` |

### Default Admin Credentials

```text
Username: admin
Password: admin123
```

---

#  Course Management

Each course stores:
- Course Code
- Faculty Owner
- Capacity
- Current Enrollment Count

### Example

```text
CS101:prof1:2:2
```

Meaning:

```text
CourseCode : Faculty : Capacity : EnrolledStudents
```

---

#  Waitlist System

If a course reaches maximum capacity:

- Students are automatically added to a waitlist.
- Queue order is maintained.
- When a student drops a course:
  - First waitlisted student is auto-promoted.
  - Enrollment records are updated automatically.
  - IPC notifications are triggered.

This demonstrates:
- Resource allocation
- Queue scheduling
- Transaction consistency
- Fairness management

---

#  Concurrency & Synchronization

The server supports multiple simultaneous clients using:

```c
pthread_create()
```

### Synchronization Mechanisms
- File locking using `fcntl()`
- Mutex protection for:
  - Logging
  - IPC communication
  - Shared resources

This prevents:
- Race conditions
- Data corruption
- Concurrent write conflicts

---

#  Inter-Process Communication (IPC)

The system implements:
- **Named Pipes (FIFO)**
- **Signals (`SIGUSR1`)**

Used for:
- Waitlist promotion notifications
- Asynchronous communication
- Event handling

---

#  Logging System

All critical activities are logged:

- Login attempts
- User additions
- Course creation
- Enrollments
- Waitlist promotions
- Logout events

### Example Log Entry

```text
[Thu Apr 30 18:51:52 2026] james dropped CS101; auto-promoted alice
```

---

#  Compilation & Execution

## 1️ Clone the Repository

```bash
git clone https://github.com/VishudhaSood/Academia-Portal.git
cd Academia-Portal
```

## 2️ Compile the Project

```bash
make
```

## 3️ Run the Server

```bash
./server
```

## 4️ Run the Client

Open another terminal:

```bash
./client
```

---

#  Sample Workflow

## Admin Workflow

```text
Login → Add Students/Faculty → View Users → Logout
```

## Faculty Workflow

```text
Login → Add Courses → View Courses → Logout
```

## Student Workflow

```text
Login → Browse Courses → Enroll/Drop → View Enrollments → Logout
```

---

#  Project Strengths

- Strong alignment with Operating Systems concepts
- Real-world academic management simulation
- Modular architecture
- Resume-worthy implementation
- Networking + synchronization integration
- Efficient resource management

---

#  Limitations

- File-based database instead of SQL
- Terminal-based interface
- Basic authentication model
- Limited UI sophistication

---

#  Future Enhancements

- SQL/NoSQL database integration
- Web-based frontend
- Password hashing & encryption
- Notification dashboards
- Analytics modules
- Advanced scheduling algorithms

---

#  Author

### Vishudha Sood  
**IIIT Bangalore**  
Roll Number: IMT2024067

---

#  Conclusion

Academia Portal successfully demonstrates the integration of critical Operating Systems concepts into a practical academic management platform.

The project combines:
- Networking
- File Systems
- Synchronization
- Multi-threading
- IPC
- Resource Allocation

into a modular, scalable, and professional Linux system programming application.

---

<div align="center">

### 🌟 If you found this project interesting, consider giving it a star!

</div>
