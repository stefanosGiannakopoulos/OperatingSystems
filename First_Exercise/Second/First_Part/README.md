# Parent-Child Process Synchronization Using Signals

## Author: Stefanos Giannakopoulos

## Overview
This program demonstrates **parent-child process synchronization** using **signals (`SIGUSR1`, `SIGUSR2`)** and **`sigaction`**. The parent prints its message first and then allows the child to execute.

## Key Features
- **Signal-based synchronization** (No `sleep()` needed).
- **Uses `sigaction` for reliable signal handling**.
- **Prevents race conditions** using `volatile sig_atomic_t` flags.
- **Ensures correct execution order** using `pause()` and signals.

---

## How It Works
### **Step 1: Parent Sets Up `SIGUSR2` Handler**
Before forking, the **parent** registers a handler for `SIGUSR2`. This ensures that it can wait for the child to signal that it is ready.

```c
struct sigaction sa_parent;
sa_parent.sa_handler = handle_parent_signal;
sigemptyset(&sa_parent.sa_mask);
sa_parent.sa_flags = 0;
if (sigaction(SIGUSR2, &sa_parent, NULL) == -1) {
    perror("Error setting up SIGUSR2 handler for parent");
    return EXIT_FAILURE;
}
```

### **Step 2: Fork the Child Process**
After setting up the signal handler, the **parent creates the child**.

```c
pid = fork();
if (pid < 0) {
    perror("fork error");
    return EXIT_FAILURE;
}
```

### **Step 3: Parent Waits for Child Setup**
The **parent calls `pause()`**, waiting for `SIGUSR2` from the child. This ensures that the child has finished setting up its `SIGUSR1` handler before the parent continues.

```c
while (!parent_ready) {
    pause();  // Wait for child to signal readiness
}
```

### **Step 4: Child Registers `SIGUSR1` and Signals the Parent**
The **child registers `SIGUSR1` handler** and then **notifies the parent** that it is ready by sending `SIGUSR2`.

```c
struct sigaction sa_child;
sa_child.sa_handler = handle_child_signal;
sigemptyset(&sa_child.sa_mask);
sa_child.sa_flags = 0;
if (sigaction(SIGUSR1, &sa_child, NULL) == -1) {
    perror("Error setting up SIGUSR1 handler for child");
    exit(EXIT_FAILURE);
}

// Notify parent that child is ready
if (kill(getppid(), SIGUSR2) == -1) {
    perror("Error sending SIGUSR2 to parent");
    exit(EXIT_FAILURE);
}
```

### **Step 5: Parent Prints First and Signals the Child**
Once the **parent receives `SIGUSR2`**, it prints its message and then sends `SIGUSR1` to the child.

```c
printf("I am the parent with PID: %d and my child's PID is: %d\n", getpid(), pid);

// Send SIGUSR1 to the child to allow it to proceed
if (kill(pid, SIGUSR1) == -1) {
    perror("Error sending SIGUSR1 to child");
    return EXIT_FAILURE;
}
```

### **Step 6: Child Waits for `SIGUSR1` and Executes**
The **child waits (`pause()`)** until `SIGUSR1` is received. Then it prints its message and exits.

```c
while (!child_ready) {
    pause();  // Wait for SIGUSR1 from parent
}

printf("I am the child with PID: %d and my parent's PID is: %d\n", getpid(), getppid());
exit(EXIT_SUCCESS);
```

### **Step 7: Parent Waits for Child Termination**
The parent waits for the **child to terminate normally** using `waitpid()`.

```c
if (waitpid(pid, &status, 0) == -1) {
    perror("waitpid error");
    return EXIT_FAILURE;
}
```

---

## **Explanation of Synchronization Variables**
### **1. `volatile sig_atomic_t child_ready = 0;`**
- Used **in the child process**.
- Set to `1` when `SIGUSR1` is received.
- Ensures the child does not proceed **before** the parent signals it.

### **2. `volatile sig_atomic_t parent_ready = 0;`**
- Used **in the parent process**.
- Set to `1` when `SIGUSR2` is received from the child.
- Ensures the parent does not print **before** the child is ready.

---

## **Expected Output (Always Ensures Parent Prints First)**
```
I am the parent with PID: 12344 and my child's PID is: 12345.
I am the child with PID: 12345 and my parent's PID is: 12344.
```

---

## **Why This is the Best Approach**
✔ **No `sleep()` required** – Ensures correct order dynamically.  
✔ **No busy-waiting** – Uses `pause()` for efficient execution.  
✔ **Ensures Parent Executes First** – Child **must wait** for `SIGUSR1`.  
✔ **Ensures Child is Ready** – Parent **must wait** for `SIGUSR2`.  
✔ **Uses `sigaction()` Instead of `signal()`** – More robust and portable.  

---
