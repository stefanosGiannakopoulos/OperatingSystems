/*
 * Author: Stefanos Giannakopoulos
 * Description: This program demonstrates how variables are handled in 
 *              parent and child processes after a fork. It shows that 
 *              each process gets a separate copy of the variable.
 * Usage:       ./program
 */

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
 