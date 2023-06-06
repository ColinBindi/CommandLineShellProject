/**
 * @file
 *
 * Contains retrieval, utility, signal, piping, redirection, jobs, and builtin functions.
 */

#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "history.h"
#include "logger.h"
#include "ui.h"
#include "util.h"

static struct command_line *cmds;
static struct job_info jobs[10] = { 0 };
static int job_num = 0;

/**
 * Retrieves the next token from a string.
 *
 * Parameters:
 * - str_ptr: maintains context in the string, i.e., where the next token in the
 *   string will be. If the function returns token N, then str_ptr will be
 *   updated to point to token N+1. To initialize, declare a char * that points
 *   to the string being tokenized. The pointer will be updated after each
 *   successive call to next_token.
 *
 * - delim: the set of characters to use as delimiters
 *
 * Returns: char pointer to the next token in the string.
 */
char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  == 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}

/**
 * Flushes the standard output when ^C is pressed
 * @param signo SIGINT integer
 */
void sigint_handler(int signo)
{
    fflush(stdout);
}

/**
 * Allows jobs to be run in the background and removes jobs when they finish
 * @param signo SIGCHLD integer
 */
void sigchld_handler(int signo)
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    for (int i = 0; i <= job_num; i++) {
        if (jobs[i].pid == pid) {
            free(jobs[i].command);
            jobs[i].command = NULL;
        }
    }
}

/**
 * Builds an array of command_line structs
 * @param args command arguments
 * @param pipes boolean that determines if there are pipes in the command
 * 
 * @return pointer to array of command_line structs
 */
struct command_line *build_pipes(char *args[], bool pipes) 
{
    cmds = calloc(100, sizeof(struct command_line));
    if (pipes == true) {
        int i = 0;
        int j = 0;
        cmds[j].tokens = args;
        cmds[j].stdout_pipe = true;
        int size = 100;
        while (args[i] != (char *) 0) {
            if (size == 0){
                struct command_line *tmp = realloc(cmds, i*sizeof(struct command_line));
                if (tmp == NULL) {
                    perror("realloc");
                    return NULL;
                } else {
                    cmds = tmp;
                }
                size = 100;
            }
            if (strcmp(args[i], "|") == 0) {
                j++;
                args[i] = (char *) 0;
                cmds[j].tokens = &args[i+1];
                cmds[j].stdout_pipe = true;
            }
            i++;
            size--;
        }
        cmds[j].stdout_pipe = false;
    } else {
        cmds[0].tokens = args;
        cmds[0].stdout_pipe = false; 
    }
    return cmds;
}

/**
 * Executes redirection on the command if the symbols ">", ">>", or "<" are found
 * @param args command arguments
 */
void execute_redirection(char *args[])
{
    struct command_line *redir = calloc(100, sizeof(struct command_line));
    int i = 0;
    int j = 0;
    int size = 100;
    while (args[i] != (char *) 0) {
        if (size == 0){
            struct command_line *tmp = realloc(redir, i*sizeof(struct command_line));
            if (tmp == NULL) {
                perror("realloc");
                return;
            } else {
                redir = tmp;
            }
            size = 100;
        }
        redir[j].stdout_file = NULL; 
        if (strcmp(args[i], ">") == 0) {
            j++;
            args[i] = (char *) 0;
            redir[j].stdout_file = args[i+1];
            int out_fd = open(redir[j].stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
                perror("fd");
                free(redir);
                return;
            }
            if (dup2(out_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                free(redir);
                return;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            j++;
            args[i] = (char *) 0;
            redir[j].stdout_file = args[i+1];
            int out_fd = open(redir[j].stdout_file, O_APPEND | O_CREAT | O_WRONLY, 0666);
            if (out_fd == -1) {
                perror("fd");
                free(redir);
                return;
            }
            if (dup2(out_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                free(redir);
                return;
            }
        } else if (strcmp(args[i], "<") == 0) {
            j++;
            args[i] = (char *) 0;
            redir[j].stdout_file = args[i+1];
            int in_fd = open(redir[j].stdout_file, O_RDONLY);
            if (in_fd == -1) {
                perror("fd");
                free(redir);
                return;
            }
            if (dup2(in_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                free(redir);
                return;
            }
        }
        i++;
        size--;
    }
    free(redir);
}

/**
 * Executes piping on the command if the symbol "|" is found
 * @param cmds command_line struct containing data on each argument of the command
 */
void execute_pipeline(struct command_line *cmds)
{
    int num = 0;
    while (true) {
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return;
        }
        execute_redirection(cmds[num].tokens);
        if (cmds[num].stdout_pipe == false) {
            if (execvp(cmds[num].tokens[0], cmds[num].tokens) == -1) {
                perror("execvp");
                return;
            }
            break;
        }
        pid_t pid = fork();
        if (pid == 0) {
            /* Child */
            if (dup2(fd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                return;
            }
            close(fd[0]);
            if (execvp(cmds[num].tokens[0], cmds[num].tokens) == -1) {
                perror("execvp");
                return;
            }
        } else {
            /* Parent */
            if (dup2(fd[0], STDIN_FILENO) == -1) {
                perror("dup2");
                return;
            }
            close(fd[1]);
        }
        num++;
    }
}

/**
 * Frees memory for the job_info struct called jobs
 */
void jobs_destroy(void)
{
    for (int i = 0; i < 10; i++) 
    {
        free(jobs[i].command);
    }
}

/**
 * Getter function for jobs array
 * 
 * @return jobs array as a pointer to a job_info struct
 */
struct job_info *get_jobs_list(void)
{
    return jobs;
}

/**
 * Getter function for the number of jobs
 * 
 * @return integer representing number of jobs
 */
int get_job_num(void)
{
    return job_num;
}

/**
 * Setter function for the number of jobs
 * @param num integer representing number of jobs
 */
void set_job_num(int num)
{
    job_num = num;
}

/**
 * Prints the history of previously entered commands
 * @param args command arguments
 */
void history_handler(char *args[]) 
{
    hist_print();
}

/**
 * Changes the process's working directory
 * @param args command arguments
 */
void cd_handler(char *args[]) 
{
    if (args[1] != NULL) {
        if (chdir(args[1]) == -1) {
            perror("chdir");
        }
    } else {
        struct passwd *pwuid = getpwuid(getuid());
        if (chdir(pwuid->pw_dir) == -1) {
            perror("chdir");
        }
    }
}

/**
 * Changes the command from "!!" to the most recent command in history
 * @param args command arguments
 */
void double_bang_handler(char *args[]) 
{
    char *bang_str = NULL;
    int num = hist_last_cnum();
    char *temp = (char*) hist_search_cnum(num);
    bang_str = strdup(temp);
    if (bang_str != NULL) {
        hist_add(bang_str);
        char *tmp = bang_str;
        char *split;
        int i = 0;
        while ((split = next_token(&tmp, " \t\r\n")) != NULL) {
            args[i++] = split;
        }
        args[i] = (char *) 0;
    }
}

/**
 * Changes the command if it starts with "!" followed by a number to the command in history at that number
 * Or changes the command if it starts with "!" followed by a prefix to the most recent command in history with that prefix
 * @param args command arguments
 */
void bang_handler(char *args[], char* bang_str) 
{
    if (((*(bang_str+1)) >= '0') && (((*(bang_str+1)) <= '9'))) {
        const char *num = (bang_str+1);
        int n = atoi(num);
        char *str = (char*) hist_search_cnum(n);
        if (str != NULL) {
            char *temp = strdup(str);
            hist_add(temp);
            free(temp);
            char *split;
            int i = 0;
            while ((split = next_token(&str, " \t\r\n")) != NULL) {
                args[i++] = split;
            }
            args[i] = (char *) 0;
            free(str);
        }
        free(str);
    } else {
        char *prefix = (bang_str+1);
        char *str = (char*) hist_search_prefix(prefix);
        if (str != NULL) {
            hist_add(str);
            char *split;
            int i = 0;
            while ((split = next_token(&str, " \t\r\n")) != NULL) {
                args[i++] = split;
            }
            args[i] = (char *) 0;
        }
        free(str);
    }
}