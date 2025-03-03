# Character Count Program Documentation

## Author
Stefanos Giannakopoulos

## Overview
This program reads an input file and counts how many times a specific character appears in it. The parent process is responsible for handling file input/output, while a child process performs the character counting. The result is written to an output file.

## Usage
```
./program <input_file> <output_file> <character_to_read>
```

### Arguments:
1. `<input_file>`: Path to the input file.
2. `<output_file>`: Path to the output file where the result will be stored.
3. `<character_to_read>`: The character to count occurrences of (must be a single character).

## Implementation Details
The program consists of the following key steps:

### 1. Argument Validation
- Ensures that exactly three arguments are provided.
- Ensures that `<character_to_read>` is a single character.

### 2. File Handling
- Opens the input file in read-only mode (`O_RDONLY`).
- Retrieves file metadata using `fstat()` to determine the file size.
- Reads the entire content of the file into a dynamically allocated buffer.

### 3. Process Communication Using Pipes
- A pipe (`pipefd[2]`) is created to enable communication between the parent and child process.
- The program forks a child process.
  - The child process counts occurrences of the character and writes the result to the pipe.
  - The parent process reads the result from the pipe and writes it to the output file.

### 4. Child Process (Character Counting)
- The child closes the read end of the pipe (`pipefd[0]`).
- Iterates through the file buffer to count occurrences of the target character.
- Writes the count to the pipe (`pipefd[1]`).
- Frees allocated memory and exits.

### 5. Parent Process (Result Handling)
- The parent closes the write end of the pipe (`pipefd[1]`).
- Uses `select()` to wait up to 5 seconds for data from the child process.
- Reads the count from the pipe and writes it to the output file.
- Waits for the child process to terminate.

### 6. Writing to Output File
- Opens the output file in write-only mode (`O_WRONLY | O_TRUNC`).
- Writes the formatted result string.
- Ensures all bytes are written before closing the file.

## Error Handling
The program includes error handling at every step:
- Ensures correct number of arguments.
- Checks if the input file can be opened.
- Handles cases where the file is empty.
- Validates memory allocation.
- Handles read/write errors.
- Ensures the child process terminates correctly.

## Example Execution
```
$ ./program input.txt output.txt a
```
### Sample `output.txt` content:
```
The character 'a' appears 42 times in file input.txt.
```

## Conclusion
This program demonstrates inter-process communication (IPC) using pipes, process creation using `fork()`, and efficient file handling in C. It ensures robustness through proper error handling and system call validations.

