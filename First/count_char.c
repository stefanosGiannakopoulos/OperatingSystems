/*
 * Author: Stefanos Giannakopoulos
 * Description: This program reads a file and counts the number of times a given 
 *              character appears in it. The result is then written to an output file.
 * Usage:       ./program <input_file> <output_file> <character_to_read>
 */

#include<stdlib.h>
#include<stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/* This programs reads a file and counts the number of times a character appears in it. The result is written in another file. */

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_read>\n", program_name); // stderr is a file pointer to the standard error stream
}

int main(int argc, char ** argv){
    // Checks that need to be done to assure the correct input 
    
    // argc is the number of arguments passed to the program nad must be equal to 4 (the 1st argv[0] is the name of the program, the 2nd is the input file, the 3rd is the output file and the 4th is the character to read)
    if(argc!=4){ 
        print_usage(argv[0]); // argv[0] is the name of the program 
        return EXIT_FAILURE; // EXIT_FAILURE is a constant that is defined in stdlib.h and is used to indicate that the program has failed
    }
    
    // If the character to read is not a single character return an error
    if(strlen(argv[3])!=1){ 
        fprintf(stderr,"Error: character to read must be a single character\n"); // print the error message
        return EXIT_FAILURE; // return EXIT_FAILURE to indicate that the program has failed
    }
   
    // All the checks were done successfully, now we can start the program

    int fd; // file descriptor, which is an integer that is used to identify the file that we are going to read from afted we open it
    
    // Extract arguments, each one is an array of characters -> char * -> argv is an array of char *
    const char *input_file = argv[1]; 
    const char *output_file = argv[2];
    const char *char_to_read = argv[3];
   
    if((fd = open(input_file,O_RDONLY))==-1){  // open the file in read only mode, we don't need to write in it
        // if the file descriptor is -1 then the file could not be opened
        perror("open error"); // perror prints a description for the last error that occurred
        return EXIT_FAILURE;
    }
    
    printf("1/4 -> File opened successfully :)\n");
    
    // Use fstat to get file information
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error retrieving file stats");
        close(fd); // close the file descriptor
        return EXIT_FAILURE;
    }
    
    // If the file is empty, then we don't need to read it
    if (file_stat.st_size == 0) {
        fprintf(stderr, "Error: file is empty\n");
        close(fd); // close the file descriptor
        return EXIT_FAILURE;
    }

    char * buf =  (char *)malloc(file_stat.st_size); // allocate memory for the buffer that will store the file contents in order to read ... 
    if(buf==NULL){ // if the memory allocation failed
        perror("malloc error"); // print the error message
        close(fd); // close the file descriptor
        free(buf); 
        return EXIT_FAILURE;
    }
 
    // read the file contents and store them in the buffer to avoid partial reads
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
    
    close(fd); // close the file descriptor for the input file

    printf("2/4 -> File read successfully :)\n");

    // Time to count the number of times the character appears in the file ... 

    long long count = 0; 
    
    for(int i=0;i<file_stat.st_size;i++){ // iterate through the buffer
        if(buf[i]==char_to_read[0]){ // if the character in the buffer is the same as the character we are looking for
            count++; // increase the count
        }
    }
    
    printf("3/4 -> Character counted successfully: %lld occurences of character %c :)\n",count,char_to_read[0]);


    
    // Now we need to write the result in the output file ...
    
    if((fd = open(output_file,O_WRONLY|O_TRUNC))==-1){ // open the file in write only mode and truncate it 
        perror("open error"); // print the error message
        free(buf); // free the memory allocated for the buffer
        return EXIT_FAILURE;
    }
    
    // Format the result message
    char result_msg[1024];
    
    snprintf(result_msg, sizeof(result_msg), "The character '%c' appears %lld times in file %s.\n", char_to_read[0], count, input_file);
    

    // Write the result to the output file to avoid partial writes 
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


    close(fd); // close the file descriptor
    free(buf); // free the memory allocated for the buffer
    
    printf("All done! :)\n");

    return EXIT_SUCCESS; // return 0 to indicate that the program has run successfully

}