# Process Creation and Execution using fork() and execv()

## Author: Stefanos Giannakopoulos

## Overview
This program demonstrates process creation using `fork()` and process execution using `execv()`. It creates a child process that executes an external program (`count_char`). The parent process waits for the child process to complete before continuing execution.

## Usage
```
./program <input_file> <output_file> <character_to_read>
```

## Code Breakdown

### 1. **Header Inclusions**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
```
These headers provide necessary functions for process management, standard I/O, and string handling.

### 2. **Printing Usage Instructions**
```c
void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name);
}
```
This function prints the correct program usage if incorrect arguments are provided.

### 3. **Main Function**
```c
int main(int argc, char *argv[]) {
```
The `main` function handles argument parsing, process creation, and execution.

### 4. **Process Creation with fork()**
```c
pid_t pid = fork();
```
- The `fork()` system call creates a new process (child).
- If `fork()` returns `-1`, it means the process creation failed.

```c
if (pid < 0) {
    perror("Fork failed");
    return EXIT_FAILURE;
}
```
If `fork()` fails, an error message is displayed and the program exits.

### 5. **Child Process Execution**
```c
if (pid == 0) {
    const char *path = "../../First/count_char"; // Path to the external program
    execv(path, argv);
    exit(EXIT_SUCCESS);
}
```
- If `pid == 0`, the process is the child process.
- The `execv()` function replaces the current process with the external program `count_char`.
- If `execv()` is successful, the program is replaced and execution continues in `count_char`.
- If `execv()` fails, the child process exits.

### 6. **Parent Process Handling**
```c
else {
    wait(NULL);
    printf("Parent Process: PID = %d waited successfully for the child to execute the program of exercise 1!\n", getpid());
}
```
- The parent process waits for the child process to finish execution using `wait(NULL)`.
- Once the child finishes, a message is printed indicating successful completion.

### 7. **Program Termination**
```c
return EXIT_SUCCESS;
```
The program exits successfully.

## Summary
- **`fork()`** creates a new process.
- **Child process** executes `count_char` using `execv()`.
- **Parent process** waits for the child process to finish.
- The program ensures proper execution and process management.

This approach allows process isolation and independent execution of programs within a system.

