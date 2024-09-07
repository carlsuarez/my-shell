#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <ncurses.h>

#define MAX_ARGS 10
#define MAX_INPUT 256
#define BUFFER_SIZE 1024

// Globals
extern WINDOW *output_win;
extern size_t line;

// Function prototypes
void execute_about(void);
void execute_greet(char *name);
void execute_clear(void);
void execute_echo(char *args);
void execute_time(void);
void execute_ls(char *input);

#endif // COMMANDS_H
