/*

zoey la
shell208.c part 2
utilized and adapted jeff's sample codes
also lurked on stackoverflow (marked use)

*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#define COMMAND_BUFFER_SIZE       102
#define MAX_COMMAND_SIZE          COMMAND_BUFFER_SIZE - 2
#define COMMAND_INPUT_SUCCEEDED   0
#define COMMAND_INPUT_FAILED      1
#define COMMAND_END_OF_FILE       2
#define COMMAND_TOO_LONG          3
#define MAX_PIPES                 7
#define MAX_ARGS                  50


void int_handler(int);

int get_command(char *command_buffer, int buffer_size);

void execute_command(char *command_line);

void help() {
    printf("this is a simple command shell called 'ClammandLine'\n\n");
    printf("type \"help\" to see ClammandLine's current features\n\n");
    printf("type \"about\" to learn why i named this shell the way i did \n");
    printf("list of commands:\n");
    printf("\t- ls\n");
    printf("\t- pwd\n");
    printf("\t- ...and commands with command-line arguments!\n");
    printf("\ntype \"exit\" to exit...\n");
    printf("\nhave fun:)\n\n");
}

void about() {
    printf("why did i name this shell 'ClammandLine'? i thought you might ask. \n\n");
    printf("clams....\n\n");
}

int main() {
    const char *prompt = "ClammandLine> ";
    char command_line[COMMAND_BUFFER_SIZE];

    // register the interrupt handler for SIGINT
    signal(SIGINT, int_handler);

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        int result = get_command(command_line, COMMAND_BUFFER_SIZE);
        if (result == COMMAND_END_OF_FILE) {
            break;

        } else if (result == COMMAND_INPUT_FAILED) {
            fprintf(stderr, "There was a problem reading your command. Please try again.\n");
            break;

        } else if (result == COMMAND_TOO_LONG) {
            fprintf(stderr, "Commands are limited to length %d. Please try again.\n", MAX_COMMAND_SIZE);

        } else {
            execute_command(command_line);
        }
    }

    return 0;
}

int get_command(char *command_buffer, int buffer_size) {
    assert(buffer_size > 0);
    assert(command_buffer != NULL);

    if (fgets(command_buffer, buffer_size, stdin) == NULL) {
        if (feof(stdin)) {
            return COMMAND_END_OF_FILE;
        } else {
            return COMMAND_INPUT_FAILED;
        }
    }

    int command_length = strlen(command_buffer);
    if (command_buffer[command_length - 1] != '\n') {
        char ch = getchar();
        while (ch != '\n' && ch != EOF) {
            ch = getchar();
        }

        return COMMAND_TOO_LONG;
    }

    command_buffer[command_length - 1] = '\0';  // remove the newline at the end
    return COMMAND_INPUT_SUCCEEDED;
}

void execute_command(char *command_line) {

    if (strcmp(command_line, "help") == 0) {
        help();

    } else if (strcmp(command_line, "exit") == 0) {
        exit(0);

    } else if (strcmp(command_line, "about") == 0) {
        about();

    } else {

        // declared an array of char pointers to hold the segments of the command line
        char *segments[MAX_PIPES + 2]; // +2 for the initial command and the null terminator

        // tokenize the command line using the pipe character "|"
        char *token = strtok(command_line, "|");
        int segCount = 0;

        // loop until all segments are tokenized or maximum pipe limit is reached
        while (token != NULL && segCount < MAX_PIPES + 1) {
            segments[segCount++] = token; // storing the tokenized segment
            token = strtok(NULL, "|"); // tokenizing the next segment
        }
        segments[segCount] = NULL; // mark end of array with NULL for execvp

        // array that stores file descriptors for piping
        int fd[2];

        // array that stores process ID numbers (PIDs) of child processes
        int pids[MAX_PIPES + 1];

        // variable that stores the old file descriptor
        int old_fd = 0;

        for (int i = 0; i < segCount; i++) {
            // creating a pip and checking for failure
            if (pipe(fd) == -1) {
                perror("pipe failed");
                exit(1);
            }

            // creating a child process
            pid_t pid = fork();

            // check if the current process is a child process
            if (pid == 0) {
                // child process

                // declare arrays to:
                char *args[MAX_ARGS]; // store arguments,
                char *output_file = NULL; // store output file name
                char *input_file = NULL; // store input file name

                // tokenize the segment using the space character
                int argCount = 0;
                token = strtok(segments[i], " ");

                // loop until all arguments are tokenized or maximum arguments limit is reached
                while (token != NULL && argCount < MAX_ARGS - 1) {
                    if (strcmp(token, ">") == 0) {
                        // if it's an output redirection symbol, get the output file name
                        output_file = strtok(NULL, " ");
                    } else if (strcmp(token, "<") == 0) {
                        // if it's an input redirection symbol, get the input file name
                        input_file = strtok(NULL, " ");
                    } else {
                        // if it's not a redirection symbol, it's an argument of the command
                        args[argCount++] = token;
                    }
                    token = strtok(NULL, " ");
                }
                args[argCount] = NULL; // mark end of args array with NULL for execvp

                // if input redirection is specified, redirect the standard input to the specified file
                if (input_file) {
                    int in_fd = open(input_file, O_RDONLY); // i used StackOverflow to find out about 0_RDONLY
                    if(in_fd < 0) {
                        perror("Input file opening failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                }

                // if output redirection is specified, redirect the standard output to the specified file
                if (output_file) {
                    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if(out_fd < 0) {
                        perror("Output file opening failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                }

                // if it's not the first segment, replace the standard input with the read end of the old pipe
                if (old_fd) {
                    dup2(old_fd, STDIN_FILENO);
                    close(old_fd);
                }

                // if it's not the last segment, replace the standard output with the write end of the new pipe
                if (segments[i+1]) {
                    dup2(fd[1], STDOUT_FILENO);
                }

                close(fd[0]);
                close(fd[1]);

                execvp(args[0], args);

                perror("execvp failed");
                exit(1);

            } else {
                // parent process
                pids[i] = pid; // store the child's PID
                close(fd[1]); // close the write end of the new pipe
                if (old_fd) {
                    close(old_fd);
                }
                old_fd = fd[0]; // store the read end of the new pipe
            }
        }
        // wait for all child processes to finish
        for (int i = 0; i < segCount; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

void int_handler(int sig) {
    if (sig == SIGINT) {
        fprintf(stderr, "^C\n");
        fprintf(stderr, "ClammandLine> ");
    }
}
