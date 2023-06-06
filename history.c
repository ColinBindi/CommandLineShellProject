/**
 * @file
 *
 * Contains functions to create a history list and get elements from it.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
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
#include "util.h"

static char** strings;
static int** integers;
static int size;
static int front = -1, end = -1;
static long int history_num;

/**
 * Checks if the history list is full
 * 
 * @return 1 if the history list is full or 0 if it is not full
 */
int strings_list_full(void)
{
    if ((front == (end + 1)) || (front == 0 && end == (size - 1))) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Checks if the history list is empty
 * 
 * @return 1 if the history list is empty or 0 if it is not empty
 */
int strings_list_empty(void)
{
    if (front == -1) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Initializes history data structures
 * @param limit the maximum number of entries mainted in the history array
 */
void hist_init(unsigned int limit)
{
    front = -1;
    end = -1;
    history_num = 0;
    size = limit;
    strings = malloc(sizeof(char*)*limit);
    integers = malloc(sizeof(int*)*limit);
}

/**
 * Frees memory for the char pointer array called strings
 */
void hist_destroy(void)
{
    for (int i = 0; i < size; i++) {
        free(strings[i]);
    }
    free(strings);
    free(integers);
}

/**
 * If the history array is not empty the element at the front is removed
 */
void hist_remove(void)
{
    if (strings_list_empty()) {
        return;
    } else {
        if (front == end) {
            front = -1;
            end = -1;
        } else {
            free(strings[front]);
            front = (front + 1) % size;
        }
    }
}

/**
 * Adds a new command to the end of the history array
 * @param cmd the command that is added
 */
void hist_add(const char *cmd)
{
    history_num++;
    if (strings_list_full()) {
        hist_remove();
    }
    if (front == -1) {
        front = 0;
    }
    end = (end + 1) % size;
    strings[end] = strdup(cmd);
    integers[end] = (int*) history_num;
}

/**
 * Getter function for history array
 * 
 * @return history array as a const char**
 */
const char **get_string_list(void)
{
  return (const char**) strings;
}

/**
 * Getter function for front index of the history array
 * 
 * @return integer representing the front of the history array
 */
int get_front(void)
{
  return front;
}

/**
 * Getter function for end index of the history array
 * 
 * @return integer representing the end of the history array
 */
int get_end(void)
{
  return end;
}

/**
 * Getter function for the number of commands added to history
 * 
 * @return integer representing the current command number
 */
int get_history_num(void)
{
  return history_num;
}

/**
 * Getter function for the size of the history list
 * 
 * @return integer representing the size of the history list
 */
int get_size(void)
{
  return size;
}

/**
 * Prints the history list in order
 */
void hist_print(void)
{
    if (strings_list_empty()) {
        return;
    } else {
        int i;
        for (i = front; i != end; i = (i + 1) % size) {
            printf("%ld %s\n", (long int) integers[i], strings[i]);
        }
        printf("%ld %s\n", (long int) integers[i], strings[i]);
        fflush(stdout);
    }
}

/**
 * Searches through the history array to find the most recent command starting with the prefix parameter
 * @param prefix prefix of a string in the history array
 * 
 * @return the most recent command in the history array that has the same prefix or NULL if not found
 */
const char *hist_search_prefix(char *prefix)
{
    if (strings_list_empty()) {
        return NULL;
    }
    size_t prefix_length = strlen(prefix);
    for (int i = end; i != end+1; i--) {
        if (i == 0) {
            i = size - 1;
        }
        if(strncmp(strings[i], prefix, prefix_length) == 0) {
            return strings[i];
        }
    }
    return NULL;
}

/**
 * Searches through the history number array to find a command number matching the command number parameter
 * @param command_number the command number of a string in the history array
 * 
 * @return the command in the history array that has the same command number as the parameter or NULL if not found
 */
const char *hist_search_cnum(int command_number)
{
    if (strings_list_empty()) {
        return NULL;
    }
    int i = 0;
    for (i = front; i != end; i = (i + 1) % size) {
        if (((long int) integers[i]) == command_number) {
            return strings[i];
        }
    }
    if (((long int) integers[i]) == command_number) {
        return strings[i];
    }
    return NULL;
}

/**
 * Searches through the history number array to find the most recent command number
 * 
 * @return the unsigned integer at the end of the history number array or 0 if there is no end
 */
unsigned int hist_last_cnum(void)
{
    if (integers[end] > 0) {
        return (long int) integers[end];
    }
    return 0;
}
