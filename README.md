## Mini Shell 

## Project Overview

This project involves the implementation of a simple shell program. The shell supports basic functionalities such as executing commands, handling background processes, piping commands, and redirecting output. 

## Features

- Executing Commands: Run programs with arguments, e.g., `sleep 10`.
- Background Execution: Run commands in the background using `&`, e.g., `sleep 10 &`.
- Single Piping: Pipe the output of one command to another, e.g., `cat foo.txt | grep bar`.
- Output Redirection: Redirect the output of a command to a file using `>`, e.g., `cat foo > file.txt`.

## Assumptions

1. The pipe (`|`), background (`&`), and redirection (`>`) symbols are used correctly and appear only once per command.
2. Command lines will not contain quotation marks or apostrophes.
3. Command lines will not combine pipes, background execution, and output redirection.
4. The results of the provided parser are assumed to be correct.

## Implementation Details

### Functions

1. int prepare(void)
   - Called before the first invocation of `process_arglist()`.
   - Used for any necessary initialization.
   - Returns `0` on success, otherwise indicates an error.

2. int process_arglist(int count, char **arglist)
   - Handles the core functionality of executing commands.
   - Manages background execution, piping, and output redirection.
   - Returns `1` if no error occurs, otherwise returns `0`.

3. int finalize(void)
   - Called before the shell exits.
   - Used for any necessary cleanup.
   - Returns `0` on success, otherwise indicates an error.

### Signal Handling

- The shell should not terminate upon receiving a `SIGINT` signal.
- Foreground processes should terminate upon `SIGINT`.
- Background processes should not terminate upon `SIGINT`.

### Error Handling

- Errors in the parent shell process do not need to notify running child processes.
- Errors in a child process before calling `execvp()` should terminate the child process with an error message.

## Compilation

To compile the program, use the following command:

gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c -o myshell

## (This project is part of the Operating Systems course at Tel Aviv University).
