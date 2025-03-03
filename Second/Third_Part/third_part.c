/*
 * Author: Stefanos Giannakopoulos
 * Description: This program reads an input file and counts how many times a specific
 *              character appears in it. The parent process takes care of file input/output,
 *              while a child process is in charge of counting the occurrences of the character.
 *              The final result is written to an output file.
 * Usage:       ./program <input_file> <output_file> <character_to_read>
 */

 #include <stdio.h>      
 #include <stdlib.h>    
 #include <unistd.h>     
 #include <sys/types.h>  
 #include <sys/stat.h>   
 #include <fcntl.h>      
 #include <sys/wait.h>  
 #include <string.h>    
 #include <sys/select.h> 
 

 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name);
 }
 
 int main(int argc, char **argv) {
     // Check if the user provided exactly three arguments (plus the program name)
     if (argc != 4) {
         print_usage(argv[0]);
         return EXIT_FAILURE;
     }
 
     // Check that the character provided is exactly one character long
     if (strlen(argv[3]) != 1) {
         fprintf(stderr, "Error: character to read must be a single character\n");
         return EXIT_FAILURE;
     }
 
     // Save the file names and the character from the command line arguments
     const char *input_file = argv[1];
     const char *output_file = argv[2];
     const char *char_to_read = argv[3];
 
     int fd;  // File descriptor
 
     // Open the input file for reading
     if ((fd = open(input_file, O_RDONLY)) == -1) {
         perror("open error");
         return EXIT_FAILURE;
     }
     printf("1/4 -> File opened successfully :)\n");
 
     // Get information about the file (like its size)
     struct stat file_stat;
     if (fstat(fd, &file_stat) == -1) {
         perror("Error retrieving file stats");
         close(fd);
         return EXIT_FAILURE;
     }
     if (file_stat.st_size == 0) {
         fprintf(stderr, "Error: file is empty\n");
         close(fd);
         return EXIT_FAILURE;
     }
 
     // Allocate enough memory to store the entire file content
     char *buf = (char *)malloc(file_stat.st_size);
     if (buf == NULL) {
         perror("malloc error");
         close(fd);
         return EXIT_FAILURE;
     }
 
     // Read the file content into the buffer, making sure to read everything
     ssize_t bytes_read = 0, total_read = 0;
     while (total_read < file_stat.st_size) {
         bytes_read = read(fd, buf + total_read, file_stat.st_size - total_read);
         if (bytes_read == -1) {
             perror("read error");
             close(fd);
             free(buf);
             return EXIT_FAILURE;
         }
         total_read += bytes_read;
     }
     close(fd);  // We are done reading from the input file
     printf("2/4 -> File read successfully :)\n");
 
     // Create a pipe to allow communication between the child and the parent.
     // The child will send the count back to the parent through this pipe.
     int pipefd[2];
     if (pipe(pipefd) == -1) {
         perror("pipe error");
         free(buf);
         return EXIT_FAILURE;
     }
 
     // Fork to create a child process
     pid_t pid;
     long long count;
     if ((pid = fork()) == -1) {
         perror("fork error");
         free(buf);
         return EXIT_FAILURE;
     }
 
     if (pid == 0) { 
         // ------------------------ Child Process ------------------------
         // The child will do the counting
 
         // Close the reading end of the pipe since the child only writes
         close(pipefd[0]);
 
         // Start counting the occurrences of the specified character in the file
         count = 0;
         for (int i = 0; i < file_stat.st_size; i++) {
             if (buf[i] == char_to_read[0]) {
                 count++;
             }
         }
 
         // Write the count value to the pipe for the parent to read
         if (write(pipefd[1], &count, sizeof(count)) != sizeof(count)) {
             perror("Error writing to pipe in child");
             free(buf);
             close(pipefd[1]);
             exit(EXIT_FAILURE);
         }
         close(pipefd[1]);  // Done writing to the pipe
         free(buf);         // Free the memory used for the file content
         exit(EXIT_SUCCESS);
     } else {
         // ------------------------ Parent Process ------------------------
         // The parent will read the count from the pipe and write the result to the output file
 
         // Close the writing end of the pipe since the parent only reads
         close(pipefd[1]);
 
         // Use select() to wait for data from the pipe without blocking indefinitely
         fd_set readfds;
         FD_ZERO(&readfds);
         FD_SET(pipefd[0], &readfds);
         struct timeval tv;
         tv.tv_sec = 5;  // Wait up to 5 seconds
         tv.tv_usec = 0;
 
         int sel_ret = select(pipefd[0] + 1, &readfds, NULL, NULL, &tv);
         if (sel_ret == -1) {
             perror("select error");
             free(buf);
             close(pipefd[0]);
             return EXIT_FAILURE;
         } else if (sel_ret == 0) {
             fprintf(stderr, "Timeout waiting for data from child\n");
             free(buf);
             close(pipefd[0]);
             return EXIT_FAILURE;
         }
 
         // Read the count from the pipe
         if (read(pipefd[0], &count, sizeof(count)) != sizeof(count)) {
             perror("Error reading from pipe in parent");
             free(buf);
             close(pipefd[0]);
             return EXIT_FAILURE;
         }
         close(pipefd[0]);  // Done reading from the pipe
         printf("3/4 -> Character counted successfully: %lld occurrences of character %c :)\n", count, char_to_read[0]);
 
         // Wait until the child process finishes
         if (wait(NULL) == -1) {
             perror("wait error");
             free(buf);
             return EXIT_FAILURE;
         }
     }
 
     // ------------------------ Write Result to Output File ------------------------
     // Open the output file for writing (and truncate if it exists)
     if ((fd = open(output_file, O_WRONLY | O_TRUNC)) == -1) {
         perror("open error");
         free(buf);
         return EXIT_FAILURE;
     }
 
     // Prepare a message to show the result
     char result_msg[1024];
     snprintf(result_msg, sizeof(result_msg), "The character '%c' appears %lld times in file %s.\n", char_to_read[0], count, input_file);
 
     // Write the result message to the output file, ensuring all data is written
     ssize_t bytes_written = 0, total_written = 0;
     size_t result_length = strlen(result_msg);
     while (total_written < result_length) {
         bytes_written = write(fd, result_msg + total_written, result_length - total_written);
         if (bytes_written == -1) {
             perror("Error writing to output file");
             free(buf);
             close(fd);
             return EXIT_FAILURE;
         }
         total_written += bytes_written;
     }
     printf("4/4 -> Result written successfully :)\n");
 
     // Clean up: close the file and free any remaining memory
     close(fd);
     free(buf);
     printf("All done! :)\n");
 
     return EXIT_SUCCESS;
 }
 