/**
 * @file
 *
 * Text-based UI functionality. These functions are primarily concerned with
 * interacting with the readline library.
 */

#ifndef _UI_H_
#define _UI_H_

void init_ui(void);
char *prompt_line(void);
char *prompt_username(void);
char *prompt_hostname(void);
char *prompt_cwd(void);
int prompt_status(void);
unsigned int prompt_cmd_num(void);
void set_status(int status);
char *read_command(void);
void set_search_start(void);
int key_up(int count, int key);
int key_down(int count, int key);
char **command_completion(const char *text, int start, int end);
char *command_generator(const char *text, int state);

#endif
