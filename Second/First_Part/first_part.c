/*
 * Author: Stefanos Giannakopoulos
 * Description: This program creates a child process that prints its PID
 *              and its parent's PID. The parent ensures it prints first
 *              before allowing the child to execute using sigaction and signals.
 * Usage:       ./program
 */

 #include <stdlib.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <signal.h>
 

 /*sig_atomic_t is a type that can be safely modified within a signal handler. 
 This prevents race conditions where a signal interrupts normal execution and modifies a shared variable. */
 
 volatile sig_atomic_t child_ready = 0;  
 volatile sig_atomic_t parent_ready = 0; 
 
 
 // Signal handler for SIGUSR1 in the child process.
 void handle_child_signal(int sig) {
     (void)sig;  // Suppress unused parameter warning
     child_ready = 1;
 }
 
 
// Signal handler for SIGUSR2 in the parent process.
 void handle_parent_signal(int sig) {
     (void)sig;  // Suppress unused parameter warning
     parent_ready = 1;
 }
 
 int main() {
     pid_t pid;
     int status;
 
     // Set up signal handler for SIGUSR2 (Parent will wait for child setup)
     struct sigaction sa_parent;
     sa_parent.sa_handler = handle_parent_signal;
     sigemptyset(&sa_parent.sa_mask);
     sa_parent.sa_flags = 0;
 
     if (sigaction(SIGUSR2, &sa_parent, NULL) == -1) {
         perror("Error setting up SIGUSR2 handler for parent");
         return EXIT_FAILURE;
     }
 
     // Fork a child process
      pid = fork();
 
     if (pid < 0) {
         perror("fork error");
         return EXIT_FAILURE;
     }
 
     if (pid > 0) {  
         // Parent Process 
         // Wait for child to signal that it is ready
         while (!parent_ready) {
             pause();               // This were the magic is being done (loop until the child finished setting up)
         } 
         
         printf("I am the parent with PID: %d and my child's PID is: %d\n", getpid(), pid);
 
         // Send SIGUSR1 to the child to allow it to proceed
         if (kill(pid, SIGUSR1) == -1) {
             perror("Error sending SIGUSR1 to child");
             return EXIT_FAILURE;
         }
 
         // Wait for the child process to terminate
         if (waitpid(pid, &status, 0) == -1) {
             perror("waitpid error");
             return EXIT_FAILURE;
         }
 
         if (WIFEXITED(status)) {
            // do nothing
         } else {
             printf("Child process %d terminated abnormally.\n", pid);
         }
     } 
     else {  
         // Child Process
         // Set up signal handler for SIGUSR1 (Child will wait for parent signal)
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
 
         // Wait until SIGUSR1 is received
         while (!child_ready) {
             pause();
         }
         printf("I am the child with PID: %d and my parent's PID is: %d\n", getpid(), getppid());
 
         exit(EXIT_SUCCESS);
     }
     return EXIT_SUCCESS;
 }
 