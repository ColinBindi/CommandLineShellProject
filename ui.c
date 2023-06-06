/**
 * @file
 *
 * Contains history navigation, autocompletion, and UI / prompt functions.
 */

#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <locale.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>

#include "history.h"
#include "logger.h"
#include "ui.h"
#include "util.h"

static const char *good_str = "😌";

static const char *bad_str  = "🤯";

static int readline_init(void);

static int num = 0;

static bool scripting = false;

static int error_check;

static bool initialized = false;

static int search_start = -1;

static char *user_input;

static int iterator = 0;

static DIR *directory = NULL;

static struct dirent *entry;

static char *path = NULL;

static char *curr_path = NULL;

/**
 * Initializes the UI and allows script mode
 */
void init_ui(void)
{
    iterator = 0;
    directory = NULL;
    entry = NULL;
    path = NULL;
    curr_path = NULL;

    LOGP("Initializing UI...\n");
    char *locale = setlocale(LC_ALL, "en_US.UTF-8");
    LOG("Setting locale: %s\n", (locale != NULL) ? locale : "could not set locale!");
    if (isatty(STDIN_FILENO) == false) {
        LOGP("data piped in on stdin; entering script mode\n");
        scripting = true;
    }
    rl_startup_hook = readline_init;
    //-- anything with "rl_" prefix is a readline function
}

/**
 * Builds the prompt having it include the username, hostname, current working directory, and status
 * 
 * @return char pointer of the built prompt
 */
char *prompt_line(void)
{
    iterator = 0;
    directory = NULL;
    entry = NULL;
    path = NULL;
    curr_path = NULL;

    const char *status = prompt_status() ? bad_str : good_str;

    char cmd_num[25];
    snprintf(cmd_num, 25, "%u", prompt_cmd_num());

    char *user = prompt_username();
    char *host = prompt_hostname();
    char *cwd = prompt_cwd();

    char *format_str = ">>-[%s]-[%s]-[%s@%s:%s]-> ";

    size_t prompt_sz
        = strlen(format_str)
        + strlen(status)
        + strlen(cmd_num)
        + strlen(user)
        + strlen(host)
        + strlen(cwd)
        + 1;

    char *prompt_str = malloc(sizeof(char) * prompt_sz);

    snprintf(prompt_str, prompt_sz, format_str,
            status,
            cmd_num,
            user,
            host,
            cwd);

    return prompt_str;
}

/**
 * Finds the username for the prompt
 * 
 * @return char pointer of username in terminal or "unknown_user" if it could not be found
 */
char *prompt_username(void)
{
    char *username = getlogin();
    if (username == NULL) {
        return "unknown_user";
    } else {
        return username;
    }
}

/**
 * Finds the hostname for the prompt
 * 
 * @return char pointer of hostname in terminal or "unknown_host" if it could not be found
 */
char *prompt_hostname(void)
{
    char *hostname = malloc(sizeof(char) * HOST_NAME_MAX);
    if (gethostname(hostname, HOST_NAME_MAX) == 0) {
        return hostname;
    } else {
        free(hostname);
        return "unknown_host";
    }
}

/**
 * Finds the current working directory for the prompt
 * 
 * @return char pointer of username in terminal or "/unknown/path" if it could not be found
 */
char *prompt_cwd(void)
{
    char *cwd;
    if ((cwd = getcwd(NULL, 0)) != NULL) {
        char *ptr = strstr(cwd, "/home");
        if (ptr != NULL) {
            char *next_tok = strdup(cwd);
            char *data = next_tok;
            char *home = next_token(&next_tok, "/");
            if (strcmp(home, "home") != 0) {
                free(data);
                return cwd;
            }
            next_token(&next_tok, "/");
            char *temp = next_token(&next_tok, " ");
            if (temp != NULL) {
                char value[PATH_MAX] = "~/";
                strcat(value, temp);
                char *p = strdup(value);
                free(data);
                return p;
            } else {
                return "~"; 
            }
        }
        return cwd;
    } else {
        return "/unknown/path";
    }
}

/**
 * Sets error_check to the status of the current process
 * @param status integer representing the status of the current process
 */
void set_status(int status) {
    error_check = status;
}

/**
 * Finds the status for the prompt
 * 
 * @return integer representing the status of the current process
 */
int prompt_status(void)
{
    return error_check;
}

/**
 * Finds the current command number for the prompt
 * 
 * @return integer representing the current command number
 */
unsigned int prompt_cmd_num(void)
{
    return ++num;
}

/**
 * Reads and returns the command typed on the command line and prints the prompt
 * 
 * @return char pointer of the command typed in on the command line
 */
char *read_command(void)
{
    if (scripting == true) {
        char *line = NULL;
        size_t line_sz = 0;
        ssize_t read_sz = getline(&line, &line_sz, stdin);
        if (read_sz == -1) {
            free(line);
            return NULL;
        }
        line[read_sz - 1] = '\0';
        return line;
    } else {
        char *prompt = prompt_line();
        char *command = readline(prompt);
        if (command == NULL) {
            free(prompt);
            return NULL;
        }
        free(prompt);
        return command;
    }
}


/**
 * Initializes the readline functionality for key history navigation and tab autocompletion
 * 
 * @return 0 on success
 */
int readline_init(void)
{
    rl_bind_keyseq("\\e[A", key_up);
    rl_bind_keyseq("\\e[B", key_down);
    rl_variable_bind("show-all-if-ambiguous", "on");
    rl_variable_bind("colored-completion-prefix", "on");
    rl_attempted_completion_function = command_completion;
    return 0;
}

/**
 * Sets search_start to -1 to start at the beginning
 */
void set_search_start(void) {
    search_start = -1;
}

/**
 * Retrieves the next element in the history list
 * @param prefix user's input in the shell
 * @param strings the history list
 * 
 * @return next char pointer in the history list or NULL if not found
 */
char *hist_search_prefix_up(char *prefix, char **strings)
{
    if (strings_list_empty()) {
        return NULL;
    }
    if (search_start == -1) {
        if (get_size() > get_history_num()) {
            return strings[get_front()]; 
        }
        search_start = get_size() - 1;
    }
    if (search_start == (get_end()+1)) {
        if (strcmp(strings[get_end()+1], rl_line_buffer) != 0) {
            user_input = strdup(rl_line_buffer);
            prefix = user_input;
            search_start = get_end();
        } else {
            return strings[get_end()+1];
        }
    }
    size_t prefix_length = strlen(prefix);
    for (int i = search_start; i != get_end()+1; i--) {
        if (strings[i] == NULL) {
            return strings[search_start+1];
        } 
        if(strncmp(strings[i], prefix, prefix_length) == 0) { 
            search_start = i - 1;
            return strings[i];
        }
        if (i == 0) {
            i = get_size() - 1;
        }
    }
    if (strncmp(strings[get_end()+1], prefix, prefix_length) == 0) {
        return strings[get_end()+1];
    } else if (strncmp(strings[search_start+1], prefix, prefix_length) == 0) {
        return strings[search_start+1];
    } else {
        return prefix;
    }
}

/**
 * Displays the next element in the history list on the command line
 * @param count integer from readline
 * @param key integer from readline
 * 
 * @return 0 on completion
 */
int key_up(int count, int key)
{
    const char **strings = get_string_list();
    if (initialized == false) {
        initialized = true;
        user_input = strdup(rl_line_buffer);
        search_start = get_end();
    }
    if (strncmp(rl_line_buffer, user_input, strlen(user_input)) != 0) {
        user_input = strdup(rl_line_buffer);
    }
    const char *prefix_string = hist_search_prefix_up(user_input, (char**) strings);
    if (prefix_string != NULL) {
        if (strings[get_end()+2] != NULL) {
            if (strcmp(prefix_string, strings[get_end()+2]) == 0) {
                rl_replace_line(strings[get_end()+1], 1);
                rl_point = rl_end;
                return 0;
            }
        }
        rl_replace_line(prefix_string, 1);
        rl_point = rl_end;
        return 0;
    }
    return 0;
}

/**
 * Retrieves the previous element in the history list
 * @param prefix user's input in the shell
 * @param strings the history list
 * 
 * @return previous char pointer in the history list or NULL if not found
 */
char *hist_search_prefix_down(char *prefix, char **strings)
{
    if (strings_list_empty()) {
        return NULL;
    }
    size_t prefix_length = strlen(prefix);
    if (search_start == -1) {
        if (get_size() > get_history_num()) {
            if(strncmp(strings[search_start+1], prefix, prefix_length) == 0) {
                if (strcmp(prefix, "") == 0) {
                    search_start++;
                    return strings[search_start+1];
                }
                return NULL;
            }
            search_start = get_front();
            return strings[get_front()+1]; 
        }
        search_start = get_end();
    }
    int i = 0;
    for (i = search_start+2; i != get_end()+1; i++) {
        if(strncmp(strings[i], prefix, prefix_length) == 0) {
            search_start = i - 1;
            return strings[i];
        }
        if (i == get_size()-1) {
            i = 0;
        }
    }
    return NULL;
}

/**
 * Displays the previous element in the history list on the command line
 * @param count integer from readline
 * @param key integer from readline
 * 
 * @return 0 on completion
 */
int key_down(int count, int key)
{
    const char **strings = get_string_list();
    const char *prefix_string = hist_search_prefix_down(user_input, (char**) strings);
    if (prefix_string != NULL) {
        rl_replace_line(prefix_string, 1);
        rl_point = rl_end;
        return 0;
    } else {
        initialized = false;
        rl_replace_line("", 1); 
        rl_point = rl_end;
    }
    return 0;
}

/**
 * Finds the matches of text when the user presses "tab" by using the command_generator function
 * @param text const char pointer of what the user typed into the shell prompt
 * @param start integer from readline
 * @param end integer from readline
 * 
 * @return double char pointer of the matches found with the command_generator function
 */
char **command_completion(const char *text, int start, int end)
{
    /* Tell readline that if we don't find a suitable completion, it should fall
     * back on its built-in filename completion. */
    rl_attempted_completion_over = 0;
    return rl_completion_matches(text, command_generator);
}

/**
 * This function is called repeatedly by the readline library to build a list of
 * possible completions. It returns one match per function call. Once there are
 * no more completions available, it returns NULL.
 * @param text const char pointer of what the user typed into the shell prompt
 * @param state integer from readline
 * 
 * @return char pointer of the match found
 */
char *command_generator(const char *text, int state)
{
    char *builtins[] = {"history", "cd", "jobs", "exit"};
    while (iterator < 4) {
        if (strncmp(builtins[iterator], text, strlen(text)) == 0) {
            return strdup(builtins[iterator++]);
        }
        iterator++;
    }
    if (path == NULL || strcmp(path, "") == 0) {
        path = getenv("PATH");
        curr_path = next_token(&path, ":");
    }
    while (curr_path != NULL) {
        if (directory == NULL) {
            if ((directory = opendir(curr_path)) == NULL) {
                while ((directory = opendir(curr_path = next_token(&path, ":"))) == NULL) {
                    if (curr_path == NULL) {
                        iterator = 0;
                        directory = NULL;
                        entry = NULL;
                        path = NULL;
                        curr_path = NULL;
                        return NULL;
                    }
                }
                path = NULL;
            }
        }
        while ((entry = readdir(directory)) != NULL) {
            if (strncmp(text, entry->d_name, strlen(text)) == 0) {
                return strdup(entry->d_name);
            }
        }
        directory = NULL;
        curr_path = next_token(&path, ":");
    }
    iterator = 0;
    directory = NULL;
    entry = NULL;
    path = NULL;
    curr_path = NULL;
    return NULL;
}
