/**
 * @file
 *
 * Creates a shell by executing commands typed after the prompt.
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

int main(void)
{
    init_ui();

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    hist_init(100);

    while (true) {
        char* command = read_command();
        if (command == NULL) {
            free(command);
            break;
        }
        if ((strcmp(command, "") != 0) && (*command != '!')) {
            hist_add(command);
        }

        set_search_start();

        char **args = calloc(100, sizeof(char*)*sizeof(char*));
        int tokens = 0;
        char *next_tok = command;
        char *curr_tok;

        char *jobs_cmd = strdup(command);
        bool pipes = false;
        bool io_redirection = false;
        int size = 100;

        while ((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL) {
            if (size == 0){
                char **tmp = realloc(args, (tokens)*sizeof(char*)*sizeof(char*));
                if (tmp == NULL) {
                    perror("realloc");
                    free(args);
                    free(command);
                    continue;
                } else {
                    args = tmp;
                }
                size = 100;
            }
            if (*curr_tok == '#') {
                break;
            } else if (*curr_tok == '|') {
                pipes = true;
                args[tokens++] = curr_tok;
            } else if (*curr_tok == '>') {
                io_redirection = true;
                args[tokens++] = curr_tok;
            } else {
                args[tokens++] = curr_tok;
            }
            size--;
        }

        args[tokens] = (char *) 0;
        
        if (args[0] == (char *) 0) {
            free(jobs_cmd);
            free(args);
            free(command);
            continue;
        }

        if (args[tokens - 1] != NULL && strcmp(args[tokens - 1], "&") != 0) {
           free(jobs_cmd);
        }

        if (strcmp(args[0], "!!") == 0) {
            double_bang_handler(args);
        } else if (args[0][0] == '!') {
            bang_handler(args, command);
        }
        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "history") == 0) {
            history_handler(args);
            free(args);
            free(command);
            continue;
        } else if (strcmp(args[0], "cd") == 0) {
            cd_handler(args);
            free(args);
            free(command);
            continue;
        } else if (strcmp(args[0], "jobs") == 0) {
            for (int i = get_job_num(); i >= 0; i--) {
                if (get_jobs_list()[i].command != NULL) {
                    printf("%s\n", get_jobs_list()[i].command);
                }
            }
            free(args);
            free(command);
            continue;
        }

        struct command_line *cmds = NULL; 
        if ((cmds = build_pipes(args, pipes)) == NULL) {
            free(args);
            free(command);
            free(cmds);
            continue;
        }
        
        pid_t child = fork();
        if (child == -1) {
            free(command);
            perror("fork");
        } else if (child == 0) {
            if (args[0] != (char *) 0) {
                if (pipes == true || io_redirection == true) {
                    execute_pipeline(cmds);
                } else {
                    if (execvp(args[0], args) != 0) {
                        perror("mash");
                    }
                }
                free(command);
            }
        } else {
            if (strcmp(args[tokens - 1], "&") == 0) {
                if (get_job_num() == 10) {
                    set_job_num(0);
                }
                get_jobs_list()[get_job_num()].command = jobs_cmd;
                get_jobs_list()[get_job_num()].pid = child;
                set_job_num(get_job_num()+1);
            } else {
                int status;
                waitpid(child, &status, 0);
                set_status(status);
            }
        }
        free(args);
        free(command);
        free(cmds);
    }
    hist_destroy();
    jobs_destroy();

    return 0;
}