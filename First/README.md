# Count Character Program Documentation

## Author
**Stefanos Giannakopoulos**

## Overview
This program reads a text file and counts the number of times a specified character appears in it. The result is then written to an output file. The program is implemented in C and utilizes system calls for file operations, ensuring robust error handling throughout the process.

## Features
- **File Reading:** Opens the specified input file in read-only mode.
- **File Metadata Retrieval:** Uses `fstat` to obtain the file size and other metadata.
- **Dynamic Memory Allocation:** Allocates memory based on the file size to store its contents.
- **Character Counting:** Iterates through the file's contents to count occurrences of the specified character.
- **Result Output:** Writes a formatted message with the count to the specified output file.
- **Error Handling:** Provides detailed error messages for common issues (e.g., file not found, empty file, memory allocation failure).

## Usage
```bash
./program <input_file> <output_file> <character_to_read>
```

<input_file>: Path to the file from which the program will read.
<output_file>: Path to the file where the result will be written.
<character_to_read>: A single character whose occurrences in the input file are to be counted.

## Code Explanation
1. Include Directives

The necessary header files are included for standard I/O operations, file control, system types, and string manipulation.

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

2. Helper Function: print_usage

This function prints the correct usage of the program to the standard error stream when the user provides incorrect arguments.

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name);
}

3. Main Function

The main function handles argument validation, file operations, character counting, and result output.
a. Argument Validation

    Checks that exactly four arguments are provided.
    Validates that the character-to-read argument is exactly one character long using strlen.
```c
if(argc != 4){ 
    print_usage(argv[0]);
    return EXIT_FAILURE;
}

if(strlen(argv[3]) != 1){ 
    fprintf(stderr, "Error: character to read must be a single character\n");
    return EXIT_FAILURE;
}
```

b. File Opening and Metadata Retrieval

    Opens the input file in read-only mode.
    Uses fstat to retrieve file statistics, including its size.
    Exits with an error if the file cannot be opened or if the file is empty.

```c
int fd;
const char *input_file = argv[1]; 
const char *output_file = argv[2];
const char *char_to_read = argv[3];

if((fd = open(input_file, O_RDONLY)) == -1){  
    perror("open error");
    return EXIT_FAILURE;
}

printf("1/4 -> File opened successfully :)\n");

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

c. Memory Allocation and File Reading

    Allocates a buffer sized to the file using malloc.
    Reads the entire file into the allocated buffer.
    Handles errors for memory allocation and reading.

char *buf = (char *)malloc(file_stat.st_size);
if(buf == NULL){
    perror("malloc error");
    close(fd);
    free(buf);
    return EXIT_FAILURE;
}

if(read(fd, buf, file_stat.st_size) == -1){ 
    perror("read error");
    close(fd);
    free(buf);
    return EXIT_FAILURE;
}

printf("2/4 -> File read successfully :)\n");
```

d. Character Counting

    Iterates through the buffer and compares each character with the specified character.
    Increments a counter for every match.
```c
long long count = 0; 
for(int i = 0; i < file_stat.st_size; i++){
    if(buf[i] == char_to_read[0]){
        count++;
    }
}

printf("3/4 -> Character counted successfully: %lld occurences of character %c :)\n", count, char_to_read[0]);
```

e. Writing the Result

    Closes the input file descriptor.
    Opens the output file in write-only mode with truncation.
    Formats a result message and writes it to the output file.
    Checks for errors during the write process.
```c
close(fd);

if((fd = open(output_file, O_WRONLY | O_TRUNC)) == -1){
    perror("open error");
    free(buf);
    return EXIT_FAILURE;
}

char result_msg[1024];
snprintf(result_msg, sizeof(result_msg), "The character '%c' appears %lld times in file %s.\n", char_to_read[0], count, input_file);

ssize_t bytes_written = write(fd, result_msg, strlen(result_msg));
if (bytes_written == -1) {
    perror("Error writing to output file");
    free(buf);
    close(fd);
    return EXIT_FAILURE;  
}

printf("4/4 -> Result written successfully :)\n");
```
f. Cleanup and Exit

    Closes the output file descriptor.
    Frees the allocated memory.
    Prints a completion message and exits successfully.

```c
close(fd);
free(buf);

printf("All done! :)\n");

return EXIT_SUCCESS;
```