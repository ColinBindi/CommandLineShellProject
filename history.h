/**
 * @file
 *
 * Contains shell history data structures and retrieval functions.
 */

#ifndef _HISTORY_H_
#define _HISTORY_H_

void hist_init(unsigned int);
void hist_destroy(void);
void hist_remove(void);
void hist_add(const char *);
void hist_print(void);
const char *hist_search_prefix(char *);
const char *hist_search_cnum(int);
unsigned int hist_last_cnum(void);
const char **get_string_list(void);
int get_front(void);
int get_end(void);
int get_history_num(void);
int get_size(void);
int strings_list_full(void);
int strings_list_empty(void);

#endif
