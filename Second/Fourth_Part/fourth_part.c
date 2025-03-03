/*
 * Author: Stefanos Giannakopoulos
 * Description: This program creates a child process using fork() and executes 
 *              an external program count_char. It passes command-line arguments 
 *              to the child process, validates the input, and ensures proper execution.
 * Usage:       ./program <input_file> <output_file> <character_to_read>
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <string.h>
 

 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name);
 }
 
 int main(int argc, char *argv[]) {
     // Create a new process using fork()
     pid_t pid = fork();
 
     // Check if fork() failed
     if (pid < 0) {
         perror("Fork failed"); // Print error message if fork fails
         return EXIT_FAILURE;
     } 
     
     // Child process
     if (pid == 0) {
         const char *path = "../../First/count_char"; // Path to the program to execute
         
         // Execute the external program with the provided arguments
         execv(path, argv); 
         exit(EXIT_SUCCESS);
     } 
     // Parent process
     else {
         wait(NULL); // Wait for the child process to complete execution
         printf("Parent Process: PID = %d waited successfully for the child to execute the program of exercise 1!\n", getpid());
     }
 
     return EXIT_SUCCESS;
 }
 