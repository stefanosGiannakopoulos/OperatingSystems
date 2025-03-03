# Understanding Process Memory Separation with `fork()`

## **Author: Stefanos Giannakopoulos**

## **Overview**
This program demonstrates how variables are handled in parent and child processes after a `fork()`. The key takeaway is that **each process gets a separate copy of the variable** after `fork()`, meaning changes in one process do not affect the other.

---

## **How `fork()` Works**
The `fork()` system call is used to create a **new child process** by duplicating the parent process. After `fork()`:

1. **Two separate processes exist** (Parent and Child).
2. **Each process has its own memory space** (no shared memory by default).
3. **The child receives an exact copy of the parent's memory at the time of `fork()`**.
4. **Changes made to variables in one process do not affect the other**.

---

## **Code Explanation**
### **C Program**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int x = 10;  // Initialize a variable in the parent process
    
    // Create a child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {  
        // Child Process
        x = 20;  // Modify x in the child process
        printf("Child Process: PID = %d, x = %d\n", getpid(), x);
        exit(EXIT_SUCCESS);
    } else {  
        // Parent Process
        x = 30;  // Modify x in the parent process
        printf("Parent Process: PID = %d, x = %d\n", getpid(), x);

        // Wait for the child process to terminate
        if (wait(NULL) == -1) {
            perror("Wait failed");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
```

---

## **How Memory is Copied in `fork()`**
### **Step 1: Parent Initializes `x`**
- Before `fork()`, the parent process initializes `x = 10`.

### **Step 2: `fork()` Creates a Child Process**
- A duplicate of the **parent process's memory** is created for the child.
- Both parent and child have a separate copy of `x` (initially 10).

### **Step 3: Parent and Child Modify Their Own `x` Separately**
- **Child process changes `x` to 20** (This change only affects the child).
- **Parent process changes `x` to 30** (This change only affects the parent).

### **Step 4: Each Process Prints Its Own Version of `x`**
- The **child prints `x = 20`**.
- The **parent prints `x = 30`**.

---

## **Expected Output**
```
Parent Process: PID = 12345, x = 30
Child Process: PID = 12346, x = 20
```

**Key Observations:**
- The **parent and child do not share memory**.
- Each process works with **its own copy of `x`**.
- Modifications made in one process **do not affect** the other.

---

## **Why Does Each Process Get a Separate Copy?**
By default, `fork()` creates a **copy-on-write** (COW) **snapshot** of the parent's memory space. This means:
- Initially, the child **shares** the same memory as the parent.
- When either process **modifies a variable**, a separate copy is created **only in that process's memory**.

This is why the child's change to `x = 20` **does not affect the parent's `x = 30`**, and vice versa.

---
