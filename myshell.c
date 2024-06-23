#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>    /* defines the requests and arguments for use by open() for Output-redirection */
#include <unistd.h>   /* symbolic constants are defined for file streams: STDIN_FILENO (0), STDOUT_FILENO (1), STDERR_FILENO (2) */
#include <sys/wait.h> /* for waitpid */
#include <errno.h>    /* defines the integer variable errno - number of last error; CREDIT for choosing the error-line format: https://stackoverflow.com/questions/503878/how-to-know-what-the-errno-means (returned by strerror() and e printed to stderr) */
#include <signal.h>   /* for signals */

/* SIGNALS FUNCTIONS HEADERS*/
void sigint_of_sig_child(void);
void handle_sigchild(int signal);
void sigint_of_sig_dfl(void);
void sigint_of_sig_ign(void);

/* HELPERS HEADERS */
char **update_arglist(int option, int i, int count, char **arglist);
void executing_commands(int count, char **arglist); /* Case 1 */
void executing_commands_in_the_background(int count, char **arglist); /* Case 2 */
void single_piping(int count, char **arglist, int i); /* Case 3 */
void output_redirecting(int count, char **arglist, int i); /* Case 4 */

/* MAIN FUNCTIONS HEADERS */
int prepare(void);
int process_arglist(int count, char **arglist);
int finalize(void);

/* FUNCTIONS IMPLEMENTATION */

/* DESC:
    establish a sigchild handler.
   CREDIT:
    - ERAN'S TRICK
    - https://www.youtube.com/watch?v=jF-1eFhyz1U */

/* When a child process stops or terminates, SIGCHLD is sent to the parent process. The default response to the signal is to ignore it. The signal can be caught and the exit status from the child process can be obtained by immediately calling wait.
This allows zombie process entries to be removed as quickly as possible. */
void sigint_of_sig_child(void)
{
    struct sigaction sa; /* initialize a sigaction structure (<signal.h> is included)*/
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;                /* avoid an EINTR “error” (error handling hint in the assignment) */
    sa.sa_handler = &handle_sigchild;        /* a pointer to a function 'handle_sigchild' - the function that is going to be called whenever we receive a signal */
    if (sigaction(SIGCHLD, &sa, NULL) == -1) /* specify the signal by the sigaction function */
    {
        fprintf(stderr, "An error occurred in sig_child. %s \n", strerror(errno));
        exit(1);
    }
}

/* DESC:
 When a child process stops or terminates, SIGCHLD is sent to the parent process. The default response to the signal is to ignore it. The signal can be caught and the exit status from the child process can be obtained by immediately calling wait.
    This allows zombie process entries to be removed as quickly as possible.
    Input: a signal.
    Output: handle the sigchild signal - to delete zombies.
   CREDIT:
    - ERAN'S TRICK
    - https://www.youtube.com/watch?v=jF-1eFhyz1U */
void handle_sigchild(int signal)
{
    int pid, status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        /* pid is terminated (WNOHANG flag specifies that waitpid should return immediately instead of waiting, if there is no child process ready to be noticed) */
    }
}

/* DESC:
    - signal is ignored.
    - Background child processes should not terminate upon SIGINT -> is used for Executing commands in the background.
   CREDIT:
    ERAN'S TRICK */
void sigint_of_sig_ign(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* avoid an EINTR “error” (error handling hint in the assignment) */
    sa.sa_handler = SIG_IGN;  /* handle signal by ignoring */
    if (sigaction(SIGINT, &sa, 0) == -1)
    {
        fprintf(stderr, "An error occurred in sig_ign: %s \n", strerror(errno));
        exit(1);
    }
}

/* DESC:
    - SIG_DFL specifies the default action for the particular signal.
    - Foreground child processes (regular commands or parts of a pipe) should terminate upon SIGINT -> is used for all options except for Executing commands in the background. */
void sigint_of_sig_dfl(void)
{
    struct sigaction dfl_sa;
    sigemptyset(&dfl_sa.sa_mask);
    dfl_sa.sa_flags = SA_RESTART; /* avoid an EINTR “error” (error handling hint in the assignment) */
    dfl_sa.sa_handler = SIG_DFL;  /* handle signal by ignoring */
    if (sigaction(SIGINT, &dfl_sa, 0) == -1)
    {
        fprintf(stderr, "An error occurred in sig_dfl: %s \n", strerror(errno));
        exit(1);
    }
}

/* DESC: gets an arglist, returns an updated arglist according to the shell symbols (NULL instead of the symbol, if excists). */
char **update_arglist(int option, int i, int count, char **arglist)
{
    if (option == 2)
    {
        i = count - 1;
    }
    arglist[i] = NULL;
    return arglist;
}

/* DESC: executes a command and waits until it completes before accepting another one. */
void executing_commands(int count, char **arglist)
{
    int pid;
    pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "An error occurred in executing commands: %s \n", strerror(errno));
        exit(1);
    }

    else if (pid == 0) /* child process */
    {
        sigint_of_sig_dfl();
        if (execvp(arglist[0], arglist) == -1)
        {
            fprintf(stderr, "An error occurred in executing commands: %s \n", strerror(errno));
            exit(1);
        }
    }

    if (waitpid(pid, NULL, 0) == EINVAL) /*CREDIT: https://linux.die.net/man/2/waitpid + in error handling is written that ECHILD, EINTR are not considered actual errors */
    {
        fprintf(stderr, "An error occurred in executing commands: %s \n", strerror(errno));
        exit(1);
    }
}

/* DESC: executes the command but does not wait for its completion before accepting another command.*/
void executing_commands_in_the_background(int count, char **arglist)
{
    int pid;
    pid = fork();

    if (pid == -1)
    {
        fprintf(stderr, "An error occurred in executing commands in the background: %s \n", strerror(errno));
        exit(1);
    }

    if (pid == 0) /* child process */
    {
        if (execvp(arglist[0], arglist) == -1)
        {
            fprintf(stderr, "An error occurred in executing commands in the background: %s \n", strerror(errno));
            exit(1);
        }
    }
}

/* DESC:
    - Input: The user enters two commands separated by a pipe symbol (|).
    - Output: The shell executes both commands concurrently, piping the standard output of the first command to the standard input of the second command/
   NOTE:
    The shell waits until both commands complete before accepting another command.
   CREDIT:
    https://www.youtube.com/watch?v=6xbLgZpOBi8&t=12s */
void single_piping(int count, char **arglist, int i)
{
    int fd[2], pid_ping, pid_grep; /* fd[0] - read, fd[1] - write */

    /* Creates the pipe */
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
        exit(1);
    }

    /* Create the processes */
    pid_ping = fork();
    if (pid_ping == -1)
    {
        fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
        exit(1);
    }

    /* Reroute the standart output of the process that starts the ping */
    else if (pid_ping == 0) /* child process (ping)*/
    {
        sigint_of_sig_dfl();

        if (dup2(fd[1], STDOUT_FILENO) == -1) /* writes to the pipe */
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (close(fd[0]) == -1)
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (close(fd[1]) == -1)
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (execvp(arglist[0], arglist) == -1) /* exec the 1st process */
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }
    }

    pid_grep = fork();
    if (pid_grep == -1)
    {
        fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
        exit(1);
    }

    else if (pid_grep == 0) /* child process (grep)*/
    {
        sigint_of_sig_dfl();

        if (dup2(fd[0], STDIN_FILENO) == -1) /* reads from the pipe */
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (close(fd[0]) == -1)
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (close(fd[1]) == -1)
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }

        if (execvp(arglist[i + 1], arglist + i + 1) == -1) /* exec the 2nd process */
        {
            fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
            exit(1);
        }
    }

    if ((close(fd[0]) == -1) || (close(fd[1]) == -1))
    {
        fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
        exit(1);
    }

    if ((waitpid(pid_ping, NULL, 0) == EINVAL) || (waitpid(pid_grep, NULL, 0) == EINVAL)) /*CREDIT: https://linux.die.net/man/2/waitpid + in error handling is written that ECHILD, EINTR are not considered actual errors */
    {
        fprintf(stderr, "An error occurred in single piping: %s \n", strerror(errno));
        exit(1);
    }
}

/* DESC:
    - executes the command so that its standard output is redirected to the output file.
    - If the specified output file does not exist, it is created. If it exists, it is overwritten.
    - The shell waits for the command to complete before accepting another command.
   CREDIT:
    - https://www.youtube.com/watch?v=5fnVr-zH-SE
    - https://www.youtube.com/watch?v=PIb2aShU_H4 */
void output_redirecting(int count, char **arglist, int i)
{
    int pid, redirect_fd;
    pid = fork();

    if (pid == -1)
    {
        fprintf(stderr, "An error occurred in output redirecting: %s \n", strerror(errno));
        exit(1);
    }

    else if (pid == 0) /* child process */
    {
        sigint_of_sig_dfl();

        redirect_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); /* open the file with the flags: O_WRONLY (Open for writing only), O_CREAT (If the specified output file does not exist, it is created); O_TRUNC (causes the file to be truncated if it exists) */
        if (redirect_fd == -1)
        {
            fprintf(stderr, "An error occurred in output redirecting: %s \n", strerror(errno));
            exit(1);
        }

        if (dup2(redirect_fd, STDOUT_FILENO) == -1) /* the file descriptor STDOUT_FILENO is adjusted so that it now refers to the same open file description as file */
        {
            fprintf(stderr, "An error occurred in output redirecting: %s \n", strerror(errno));
            exit(1);
        }

        if (close(redirect_fd) == -1) /* there are two references to redirect_fd; closing the original fd and remain with the new fd (1) */
        {
            fprintf(stderr, "An error occurred in output redirecting: %s \n", strerror(errno));
            exit(1);
        }

        if (execvp(arglist[0], arglist) == -1)
        {
            fprintf(stderr, "An error occurred in output redirecting: %s \n", strerror(errno));
            exit(1);
        }
    }
}

/* DESC:
    - the skeleton calls this function before the first invocation of process_arglist().
    - after prepare() finishes, the parent (shell) should not terminate upon SIGINT -> sig_ign.
    - should prevent zombies and remove them as fast as possible - > sig_chld.
   NOTE:
   thats why in ptrpare we establish -
    - a sig-child handler.
    - a sig-ignore. */
int prepare(void)
{
    sigint_of_sig_child(); /* prevent zombies */
    sigint_of_sig_ign();   /* ignore sigint */
    return 0;              /* returns 0 on success; any other return value indicates an error. */
}

/* DESC: gets an array (and its length + 1 for NULL) contains the parsed command line, and excecues it according to the given behavior in the assignment description */
int process_arglist(int count, char **arglist)
{
    int i, option;
    option = 1;

    if (strcmp(arglist[count - 1], "&") == 0)
    {
        option = 2;
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            if (strcmp(arglist[i], "|") == 0)
            {
                option = 3;
                break;
            }
            if (strcmp(arglist[i], ">") == 0)
            {
                option = 4;
                break;
            }
        }
    }

    arglist = update_arglist(option, i, count, arglist); /* Null instead of shell signs*/

    if (option == 1) /* Executing commands */
        executing_commands(count, arglist);

    else if (option == 2) /* Executing commands in the background */
        executing_commands_in_the_background(count, arglist);

    else if (option == 3) /* Single piping */
        single_piping(count, arglist, i);

    else if (option == 4) /* Output redirecting */
        output_redirecting(count, arglist, i);

    return 1; /* In the original (shell/parent) process, process_arglist() should return 1 if no error occurs. */
}

/* DESC: the skeleton calls this function before exiting. This function returns 0 on success; any other return value indicates an error. */
int finalize(void)
{
    return 0;
}