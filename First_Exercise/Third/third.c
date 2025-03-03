/*
 * Author: Stefanos Giannakopoulos (extended)
 * Description: This program reads an input file and counts how many times a specific
 *              character appears in it. It now creates P child processes (P is read from
 *              the environment variable "P", or defaults to 4 if not set). Each child 
 *              searches a distinct segment of the file in parallel. The parent process 
 *              collects and prints the total result. When the program receives SIGINT 
 *              (Control+C), instead of terminating it prints the number of child processes 
 *              still searching.
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
 #include <signal.h>
 #include <errno.h>
 #include <time.h>   // For time() in the SIGINT handler
 
 volatile sig_atomic_t active_children = 0;  // Global counter for active child processes
 
 // Debounced SIGINT handler: prints at most once per second.
 void sigint_handler(int signum) {
     static time_t last_print = 0;
     time_t now = time(NULL);
     if (now - last_print < 1) {
         return;
     }
     last_print = now;
     char buf[128];
     int len = snprintf(buf, sizeof(buf), "\nReceived SIGINT. Active child processes: %d\n", active_children);
     write(STDOUT_FILENO, buf, len);
 }
 
 // SIGCHLD handler: reaps terminated children and decrements active_children.
 void sigchld_handler(int signum) {
     int saved_errno = errno;
     while (waitpid(-1, NULL, WNOHANG) > 0) {
         active_children--;
     }
     errno = saved_errno;
 }
 
 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name);
 }
 
 int main(int argc, char **argv) {
     // Check for correct number of arguments.
     if (argc != 4) {
         print_usage(argv[0]);
         return EXIT_FAILURE;
     }
     if (strlen(argv[3]) != 1) {
         fprintf(stderr, "Error: character to read must be a single character\n");
         return EXIT_FAILURE;
     }
     
     const char *input_file = argv[1];
     const char *output_file = argv[2];
     const char *char_to_read = argv[3];
     
     int fd;
     // Open input file.
     if ((fd = open(input_file, O_RDONLY)) == -1) {
         perror("open error");
         return EXIT_FAILURE;
     }
     printf("1/4 -> File opened successfully :)\n");
     
     // Get file stats.
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
     
     // Read the entire file into memory.
     char *buf = malloc(file_stat.st_size);
     if (buf == NULL) {
         perror("malloc error");
         close(fd);
         return EXIT_FAILURE;
     }
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
     close(fd);
     printf("2/4 -> File read successfully :)\n");
     
     // Determine the number of child processes from the environment variable "P" (default 4).
     int P = 4;
     char *env_p = getenv("P");
     if (env_p != NULL) {
         P = atoi(env_p);
         printf("P environment variable found: %d\n", P);
         if (P <= 0) {
             fprintf(stderr, "Invalid value for P. Must be positive.\n");
             free(buf);
             return EXIT_FAILURE;
         }
     }
     printf("Using %d child processes for searching.\n", P);
     
     // Set up signal handlers in the parent (only once).
     struct sigaction sa_int;
     sa_int.sa_handler = sigint_handler;
     sigemptyset(&sa_int.sa_mask);
     sa_int.sa_flags = 0;
     if (sigaction(SIGINT, &sa_int, NULL) == -1) {
         perror("sigaction SIGINT error");
         free(buf);
         return EXIT_FAILURE;
     }
     struct sigaction sa_chld;
     sa_chld.sa_handler = sigchld_handler;
     sigemptyset(&sa_chld.sa_mask);
     sa_chld.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
         perror("sigaction SIGCHLD error");
         free(buf);
         return EXIT_FAILURE;
     }
     
     // Create an array of pipes (one per child).
     int (*pipes)[2] = malloc(P * sizeof(int[2]));
     if (pipes == NULL) {
         perror("malloc error for pipes");
         free(buf);
         return EXIT_FAILURE;
     }
     
     // Calculate file partitioning.
     off_t file_size = file_stat.st_size;
     off_t base_size = file_size / P;
     off_t remainder = file_size % P;
     
     active_children = P;  // Initialize active children counter.
     pid_t *child_pids = malloc(P * sizeof(pid_t));
     if (child_pids == NULL) {
         perror("malloc error for child_pids");
         free(buf);
         free(pipes);
         return EXIT_FAILURE;
     }
     
     // Fork P child processes.
     for (int i = 0; i < P; i++) {
         if (pipe(pipes[i]) == -1) {
             perror("pipe error");
             free(buf);
             free(pipes);
             free(child_pids);
             return EXIT_FAILURE;
         }
         pid_t pid = fork();
         if (pid == -1) {
             perror("fork error");
             free(buf);
             free(pipes);
             free(child_pids);
             return EXIT_FAILURE;
         } else if (pid == 0) {
             // ------------- Child Process -------------
             // Ignore SIGINT in the child so only the parent handles it.
             signal(SIGINT, SIG_IGN);
             close(pipes[i][0]);
             off_t start = i * base_size + (i < remainder ? i : remainder);
             off_t segment_size = base_size + (i < remainder ? 1 : 0);
             off_t end = start + segment_size;
             long long count = 0;
             for (off_t j = start; j < end; j++) {
                 if (buf[j] == char_to_read[0]) {
                     count++;
                 }
                 // Add a delay for testing purposes.
                 usleep(100000);
             }
             if (write(pipes[i][1], &count, sizeof(count)) != sizeof(count)) {
                 perror("Error writing to pipe in child");
                 free(buf);
                 close(pipes[i][1]);
                 exit(EXIT_FAILURE);
             }
             close(pipes[i][1]);
             free(buf);
             exit(EXIT_SUCCESS);
         } else {
             // ------------- Parent Process -------------
             child_pids[i] = pid;
             close(pipes[i][1]);
         }
     }
     
     // Parent collects results from all children.
     long long total_count = 0;
     for (int i = 0; i < P; i++) {
         long long child_count = 0;
         ssize_t n, read_bytes = 0;
         fd_set readfds;
         struct timeval tv;
         // Use select() to wait for data on this pipe (retry if interrupted).
         while (1) {
             FD_ZERO(&readfds);
             FD_SET(pipes[i][0], &readfds);
             tv.tv_sec = 5;
             tv.tv_usec = 0;
             int ret = select(pipes[i][0] + 1, &readfds, NULL, NULL, &tv);
             if (ret == -1) {
                 if (errno == EINTR)
                     continue;
                 else {
                     perror("select error on pipe");
                     free(buf);
                     free(pipes);
                     free(child_pids);
                     return EXIT_FAILURE;
                 }
             }
             break;
         }
         if (FD_ISSET(pipes[i][0], &readfds)) {
             while (read_bytes < sizeof(child_count)) {
                 n = read(pipes[i][0], ((char*)&child_count) + read_bytes, sizeof(child_count) - read_bytes);
                 if (n == -1) {
                     if (errno == EINTR)
                         continue;
                     perror("Error reading from pipe in parent");
                     free(buf);
                     free(pipes);
                     free(child_pids);
                     return EXIT_FAILURE;
                 }
                 if (n == 0)
                     break;  // Pipe closed
                 read_bytes += n;
             }
             total_count += child_count;
         } else {
             fprintf(stderr, "Timeout waiting for data on pipe %d\n", i);
         }
         close(pipes[i][0]);
     }
     
     // Wait for all child processes to finish.
     for (int i = 0; i < P; i++) {
         waitpid(child_pids[i], NULL, 0);
     }
     free(child_pids);
     free(pipes);
     
     printf("3/4 -> Character counted successfully: total %lld occurrences of character '%c' :)\n", total_count, char_to_read[0]);
     
     // Write the result to the output file.
     if ((fd = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1) {
         perror("open output file error");
         free(buf);
         return EXIT_FAILURE;
     }
     char result_msg[1024];
     snprintf(result_msg, sizeof(result_msg), "The character '%c' appears %lld times in file %s.\n", char_to_read[0], total_count, input_file);
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
     close(fd);
     printf("4/4 -> Result written successfully :)\n");
     
     free(buf);
     printf("All done! :)\n");
     return EXIT_SUCCESS;
 }
 