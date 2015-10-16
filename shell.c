#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <mcheck.h>

#include "parser.h"
#include "shell.h"

/**
 * Program that simulates a simple shell.
 * The shell covers basic commands, including builtin commands 
 * (cd and exit only), standard I/O redirection and piping (|). 
 
 */

#define MAX_DIRNAME 100
#define MAX_COMMAND 1024
#define MAX_TOKEN 128

/* Functions to implement, see below after main */
int execute_cd(char** words);
int execute_nonbuiltin(simple_command *s);
int execute_simple_command(simple_command *cmd);
int execute_complex_command(command *cmd);
int execute_command(char **tokens);

int main(int argc, char** argv) {
        
        char cwd[MAX_DIRNAME];           /* Current working directory */
        char command_line[MAX_COMMAND];  /* The command */
        char *tokens[MAX_TOKEN];         /* Command tokens (program name, 
                                          * parameters, pipe, etc.) */

        while (1) {

                /* Display prompt */            
                getcwd(cwd, MAX_DIRNAME-1);
                printf("%s> ", cwd);
                
                /* Read the command line */
                fgets(command_line, MAX_COMMAND, stdin);
                /* Strip the new line character */
                if (command_line[strlen(command_line) - 1] == '\n') {
                        command_line[strlen(command_line) - 1] = '\0';
                }
                
                /* Parse the command into tokens */
                parse_line(command_line, tokens);

                /* Check for empty command */
                if (!(*tokens)) {
                        continue;
                }
                
                /* Construct chain of commands, if multiple commands */
                command *cmd = construct_command(tokens);
                // print_command(cmd, 0);
    
                int exitcode = 0;
                if (cmd->scmd) {
                        exitcode = execute_simple_command(cmd->scmd);
                        if (exitcode == -1) {
                                release_command(cmd);
                                break;
                        }
                }
                else {
                        exitcode = execute_complex_command(cmd);
                        if (exitcode == -1) {
                                release_command(cmd);
                                break;
                        }
                }
                release_command(cmd);
        }
    
        return 0;
}


/**
 * Changes directory to a path specified in the words argument;
 * For example: words[0] = "cd"
 *              words[1] = "csc209/assignment3/"
 * Your command should handle both relative paths to the current 
 * working directory, and absolute paths relative to root,
 * e.g., relative path:  cd csc209/assignment3/
 *       absolute path:  cd /u/bogdan/csc209/assignment3/
 */
int execute_cd(char** words) {
        
        /** 
	 * The first word contains the "cd" string, the second one contains 
         * the path.
         * Check possible errors:
         * - The words pointer could be NULL, the first string or the second 
         *   string could be NULL, or the first string is not a cd command
         * - If so, return an EXIT_FAILURE status to indicate something is 
         *   wrong.
         */
         if(!words || !words[0] || !words[1] || strcmp(words[0], "cd")) {
            return EXIT_FAILURE;
        }

        /**
         * Change directory to words[1], and return the result of that
	 * chdir call. If the directory to be changed to (words[1] or
	 * the cwd plus '/' and words[1]) is too long, return
	 * EXIT_FAILURE
         */
        if(is_relative(words[1])) {
            char dir[MAX_DIRNAME];
            getcwd(dir, MAX_DIRNAME);
            if (strlen(words[1]) + strlen(dir) + 1 >= MAX_DIRNAME) {
                return EXIT_FAILURE;
            }
        } else {
            if(strlen(words[1]) >= MAX_DIRNAME) {
                return EXIT_FAILURE;
            }
        }
        return chdir(words[1]);
}


/**
 * Executes a program, based on the tokens provided as 
 * an argument.
 * For example, "ls -l" is represented in the tokens array by 
 * 2 strings "ls" and "-l", followed by a NULL token.
 * The command "ls -l | wc -l" will contain 5 tokens, 
 * followed by a NULL token. 
 */
int execute_command(char **tokens) {
        
        /**
         * Execute a program, based on the tokens provided.
         * The first token is the command name, the rest are the arguments 
         * for the command. Use perror and return EXIT_FAILURE 
	 * if an error occurs.
         * Function returns only in case of a failure (EXIT_FAILURE).
         */
    execvp(tokens[0], tokens);
    /* These lines reached only if the execution failed. */
    perror(tokens[0]);
    return EXIT_FAILURE;
}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s) {
        /**
         * Check if the in, out, and err fields are set (not NULL),
         * and, IN EACH CASE:
         * - Open a new file descriptor (make sure you have the correct flags,
         *   and permissions);
         * - redirect stdin/stdout/stderr to the corresponding file.
         *   (hint: see dup2 man pages).
         * - close the newly opened file descriptor in the parent as well. 
         *   (Avoid leaving the file descriptor open across an exec!) 
         * - finally, execute the command using the tokens (see execute_command
         *   function above).
         * This function returns only if the execution of the program fails.
         */
    /*If the command's input is to be redirected, set up a file descriptor
     * for the input file and make stdin be a duplicate of the input.
     */
    if(s->in) {
        int fd;
        if((fd = open(s->in, O_RDONLY)) == -1) {
            perror("open");
            return EXIT_FAILURE;
        }
        if(dup2(fd, fileno(stdin)) == -1) {
            perror("dup2");
            return EXIT_FAILURE;
        }
        if(close(fd) == -1) {
            perror("close");
            return EXIT_FAILURE;
        }
    }
    /* As above but for the output. */
    if(s->out) { 
        int fd;
        if((fd = open(s->out, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU)) == -1) {
            perror("open");
            return EXIT_FAILURE;
        }
        if(dup2(fd, fileno(stdout)) == -1) {
            perror("dup2");
            return EXIT_FAILURE;
        }
        if(close(fd) == -1) {
            perror("close");
            return EXIT_FAILURE;
        }
    }
    /* As above but for stderr. */
    if(s->err) { 
        int fd;
        if((fd = open(s->err, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU)) == -1) {
            perror("open");
            return EXIT_FAILURE;
        }
        if(dup2(fd, fileno(stderr)) == -1) {
            perror("dup2");
            return EXIT_FAILURE;
        }
        if(close(fd) == -1) {
            perror("close");
            return EXIT_FAILURE;
        }
    }
    execute_command(s->tokens);
    
    return EXIT_FAILURE;
}


/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {

    /**
     * Check if the command is builtin.
     * 1. If it is, then handle BUILTIN_CD (see execute_cd function provided) 
     *    and BUILTIN_EXIT (simply exit with an appropriate exit status).
     * 2. If it isn't, then you must execute the non-builtin command. 
     * - Fork a process to execute the nonbuiltin command 
     *   (see execute_nonbuiltin function above).
     * - The parent should wait for the child.
     *   (see wait man pages).
     */
    int command_number;
    /* Handle builtin commands. */
    if((command_number = is_builtin(*(cmd->tokens)))) {
        switch(command_number) {
            case(BUILTIN_CD):
                if(execute_cd(cmd->tokens) != EXIT_SUCCESS) {
                    printf("No such file or directory.\n");
                }
                return 0;
                break;
            case(BUILTIN_EXIT):
                return -1;
                break;
        }
    /* Handle nonbuiltin commands.
     * Fork a process and use execute_nonbuiltin to execute the command.
     */
    } else {
        int pid;
        if((pid = fork()) < 0) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) { //child
            /* The exit status of the child process will be the return value
             * of execute_nonbuiltin (if there is an error), or the exit
             * status of the command exec'd.
             */
            exit(execute_nonbuiltin(cmd));
        } else { //parent
            int status;
            /* Wait for the command to return, then return its exit status*/
            if(wait(&status) != -1) {
                if(WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                } else {
                    return EXIT_FAILURE;
                }
            } else {
                perror("wait");
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_FAILURE;
}


/**
 * Executes a complex command.  A complex command is two commands chained 
 * together with a pipe operator.
 */
int execute_complex_command(command *c) {
    
    /**
     * Check if this is a simple command, using the scmd field.
     * Execute nonbuiltin commands only. If it's exit or cd, you should not 
     * execute these in a piped context, so simply ignore builtin commands. 
     */
    if(c->scmd) {
        /* The exit status of the child process will be the return value
         * of execute_nonbuiltin (if there is an error), or the exit
         * status of the command exec'd.
         */
        exit(execute_nonbuiltin(c->scmd));
        // printf("%s\n", c->scmd->tokens[0]);
        return 0;
    }


    /** 
     * Shell currently only supports | operator.
     */

    if (!strcmp(c->oper, "|")) {
        
        /**
         * Create a pipe "pfd" that generates a pair of file 
         * descriptors, to be used for communication between the 
         * parent and the child. Make sure to check any errors in 
         * creating the pipe.
         */
        int pfd[2];
        if(pipe(pfd) != 0) {
            perror("pipe");
            return EXIT_FAILURE;
        }
        /**
         * Fork a new process.
         * In the child:
         *  - close one end of the pipe pfd and close the stdout 
         * file descriptor.
         *  - connect the stdout to the other end of the pipe (the 
         * one you didn't close).
         *  - execute complex command cmd1 recursively. 
         * In the parent: 
         *  - fork a new process to execute cmd2 recursively.
         *  - In child 2:
         *     - close one end of the pipe pfd (the other one than 
         *       the first child), and close the standard input file 
         *       descriptor.
         *     - connect the stdin to the other end of the pipe (the 
         *       one you didn't close).
         *     - execute complex command cmd2 recursively. 
         *  - In the parent:
         *     - close both ends of the pipe. 
         *     - wait for both children to finish.
         */
        
        int pid;
        if((pid = fork()) < 0) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) { //child
            close(pfd[0]);
            if(dup2(pfd[1], fileno(stdout)) < 0) {
                perror("dup2");
                return -1;
            }
            /* The exit status of the child process will be the return value
             * of execute_nonbuiltin (if there is an error), or the exit
             * status of the command exec'd.
             */
            exit(execute_complex_command(c->cmd1));
        } else { //parent
            if((pid = fork()) < 0) { 
                perror("fork");
                return EXIT_FAILURE;
            } else if (pid == 0) { //child
                close(pfd[1]);
                if(dup2(pfd[0], fileno(stdin)) < 0) {
                    perror("dup2");
                    return -1;
                }
                /* The exit status of the child process will be the return
                 * value of execute_nonbuiltin (if there is an error), or 
                 * the exit status of the command exec'd.
                 */
                exit(execute_complex_command(c->cmd2));
            } else { //parent
                close(pfd[0]);
                close(pfd[1]);

                int status;
                int exitcode = -1;
                int waitedPID;
                while ((waitedPID = wait(&status)) > 0) {
                    if(WIFEXITED(status)) {
                        /*exitcode should be the exit status of the
                         *last command in the pipeline.
                         */
                        if(waitedPID == pid) {
                            exitcode = WEXITSTATUS(status);
                        }
                    } else {
                        return EXIT_FAILURE;
                    }
                }
                return exitcode;
            }
        }
    }
    return 0;
}
