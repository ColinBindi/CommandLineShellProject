/**
 * @file
 *
 * Contains function headers and structs for piping, redirection, and jobs.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

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

/**
 * Stores command information for piping and redirection
 */
struct command_line 
{
    char **tokens;
    bool stdout_pipe;
    char *stdout_file;
};

/**
 * Stores the command and its pid for jobs
 */
struct job_info 
{
    char *command;
    pid_t pid;
};

char *next_token(char **str_ptr, const char *delim);
void sigint_handler(int signo);
void sigchld_handler(int signo);
struct command_line *build_pipes(char *args[], bool pipes);
void execute_redirection(char *args[]);
void execute_pipeline(struct command_line *cmds);
void jobs_destroy(void);
struct job_info *get_jobs_list(void);
int get_job_num(void);
void set_job_num(int num);
void history_handler(char *args[]);
void cd_handler(char *args[]);
void double_bang_handler(char *args[]);
void bang_handler(char *args[], char* bang_str);

#endif