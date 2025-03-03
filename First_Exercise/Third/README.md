# Parallel Character Counter

## Overview
This program reads an input file and counts how many times a specific character appears in it. It creates `P` child processes (where `P` is determined from the `P` environment variable, defaulting to `4` if not set). Each child process searches a distinct segment of the file in parallel. The parent process collects and prints the total count. Additionally, when the program receives a `SIGINT` (Ctrl+C), it prints the number of child processes still searching instead of terminating immediately.

## Usage
```sh
./program <input_file> <output_file> <character_to_read>
```

## Features
- Parallel processing using `fork()`.
- Signal handling for `SIGINT` (Ctrl+C) to display active child processes.
- `SIGCHLD` handling to properly reap terminated child processes.
- Pipelined inter-process communication (IPC) using pipes.
- File partitioning for balanced workload distribution.

## Program Breakdown

### 1. Argument Parsing & File Handling
The program begins by verifying the correct number of arguments and checking that the character to read is a single character.

```c
if (argc != 4) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
}
if (strlen(argv[3]) != 1) {
    fprintf(stderr, "Error: character to read must be a single character\n");
    return EXIT_FAILURE;
}
```

The input file is opened and read entirely into memory:

```c
int fd = open(input_file, O_RDONLY);
if (fd == -1) {
    perror("open error");
    return EXIT_FAILURE;
}

struct stat file_stat;
if (fstat(fd, &file_stat) == -1) {
    perror("Error retrieving file stats");
    close(fd);
    return EXIT_FAILURE;
}
```

### 2. Setting Up Parallel Processing
The number of child processes (`P`) is determined from the environment variable:

```c
int P = 4;
char *env_p = getenv("P");
if (env_p != NULL) {
    P = atoi(env_p);
    if (P <= 0) {
        fprintf(stderr, "Invalid value for P. Must be positive.\n");
        free(buf);
        return EXIT_FAILURE;
    }
}
```

Each process gets an equal share of the file:

```c
off_t base_size = file_stat.st_size / P;
off_t remainder = file_stat.st_size % P;
```

### 3. Signal Handling
#### SIGINT (Ctrl+C Handling)
The `SIGINT` signal is handled to print the number of active child processes:

```c
void sigint_handler(int signum) {
    static time_t last_print = 0;
    time_t now = time(NULL);
    if (now - last_print < 1) return;
    last_print = now;
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "\nReceived SIGINT. Active child processes: %d\n", active_children);
    write(STDOUT_FILENO, buf, len);
}
```

#### SIGCHLD (Child Termination Handling)
Child processes are reaped using `SIGCHLD` to prevent zombie processes:

```c
void sigchld_handler(int signum) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        active_children--;
    }
    errno = saved_errno;
}
```

### 4. Forking Child Processes
Each child process searches a different part of the file for the specified character:

```c
for (int i = 0; i < P; i++) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        return EXIT_FAILURE;
    } else if (pid == 0) {  // Child process
        signal(SIGINT, SIG_IGN);  // Ignore SIGINT in child
        off_t start = i * base_size + (i < remainder ? i : remainder);
        off_t segment_size = base_size + (i < remainder ? 1 : 0);
        long long count = 0;
        for (off_t j = start; j < start + segment_size; j++) {
            if (buf[j] == char_to_read[0]) count++;
        }
        write(pipes[i][1], &count, sizeof(count));
        close(pipes[i][1]);
        free(buf);
        exit(EXIT_SUCCESS);
    }
}
```

### 5. Collecting Results
The parent process reads results from the child processes via pipes:

```c
long long total_count = 0;
for (int i = 0; i < P; i++) {
    long long child_count = 0;
    ssize_t read_bytes = 0;
    while (read_bytes < sizeof(child_count)) {
        ssize_t n = read(pipes[i][0], ((char*)&child_count) + read_bytes, sizeof(child_count) - read_bytes);
        if (n <= 0) break;
        read_bytes += n;
    }
    total_count += child_count;
    close(pipes[i][0]);
}
```

### 6. Writing Results to Output File
Finally, the total count is written to the output file:

```c
int fd_out = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
if (fd_out == -1) {
    perror("open output file error");
    return EXIT_FAILURE;
}
char result_msg[1024];
snprintf(result_msg, sizeof(result_msg), "The character '%c' appears %lld times in file %s.\n", char_to_read[0], total_count, input_file);
write(fd_out, result_msg, strlen(result_msg));
close(fd_out);
```

## Summary
- The parent process spawns `P` child processes.
- Each child searches a different segment of the file.
- Results are communicated via pipes.
- The parent process collects results and writes them to the output file.
- `SIGINT` prints active child processes instead of terminating immediately.
- `SIGCHLD` properly reaps terminated child processes.

This implementation ensures efficient parallel execution with proper synchronization and error handling.

