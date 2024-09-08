#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_SCROLLBACK 1000 // Number of lines to keep in scrollback buffer
#define LINE_LENGTH 512

// Globals
extern WINDOW *output_win;
extern int line;

// Structure to hold every line printed
typedef struct
{
    char data[MAX_SCROLLBACK][LINE_LENGTH];
    size_t length;
} Scroll_History;

extern Scroll_History scroll_his; // Global variable

// Function prototypes
void execute_about(void);
void execute_greet(char *name);
void execute_clear(void);
void execute_echo(char *args);
void execute_time(void);
void execute_ls(char *input);
extern void adjust_window();
extern void add_to_scroll_history(Scroll_History *history, char *data);

#endif // COMMANDS_H
