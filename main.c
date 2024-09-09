#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include "commands.h"
#include <errno.h>

#define SHELL_AND_WD_MAX_LENGTH 128
#define LINE_LENGTH 512
#define MAX_INPUT 200
#define MAX_HISTORY 32
#define KEY_ESC 27
#define NUM_COMMANDS 14

// Structure to hold a command
typedef struct
{
    char data[MAX_INPUT];
    size_t length;
} Command;

// Structure to hold command history
typedef struct
{
    Command commands[MAX_HISTORY];
    size_t length;
} History;

// Prototypes
void init_ncurses(void);
void add_to_history(History *history, const Command *command);
void add_to_scroll_history(Scroll_History *history, char *data);
char *get_shell_prompt(void);
inline void adjust_window(void);
bool shell_at_bottom(void);
void handle_command(char *command, int *history_index, int *index, int *scroll_offset);
void redraw_output(Scroll_History *history, int offset);
void handle_scroll_reset(int *offset, char *prompt, char *data, int index);
bool check_bins(char *token, char *command);
char *get_home(void);

// Global variables
WINDOW *output_win;
int line;
Scroll_History scroll_his = {.length = 0};

int main(void)
{
    // Initialize the ncurses window
    init_ncurses();

    // Start at the user directory
    chdir("/home/cj-suarez");

    History history = {.length = 0}; // Initialize command history
    Command input_buffer = {0};      // Initialize the input buffer
    int ch;
    int history_index = -1; // Index for history navigation
    int scroll_offset = -1; // Index for scroll navigation

    while (1)
    {
        char *prompt = get_shell_prompt();
        mvwprintw(output_win, line, 0, "%s", prompt);
        wrefresh(output_win); // Refresh to show the prompt

        memset(input_buffer.data, 0, sizeof(input_buffer.data));
        int index = 0;
        input_buffer.length = 1;

        /* Handle input*/
        while ((ch = wgetch(output_win)) != '\n') // Read until Enter key
        {
            switch (ch)
            {
            case KEY_F(2):
                // Exit shell
                endwin(); // End ncurses mode
                return 0;

            case KEY_BACKSPACE:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Delete a character
                if (index > 0)
                {
                    index--;
                    history_index = -1;
                    input_buffer.length--;
                    memmove(&input_buffer.data[index], &input_buffer.data[index + 1], input_buffer.length - index);
                    mvwdelch(output_win, line, index + strlen(prompt));
                    wmove(output_win, line, index + strlen(prompt));
                }
                break;

            case KEY_UP:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Navigate through the command history (move back)
                if (history.length)
                {
                    if (history_index > 0)
                        history_index--;
                    else if (history_index == -1)
                        history_index = history.length - 1;
                    wmove(output_win, line, strlen(prompt));
                    wclrtoeol(output_win);
                    memset(input_buffer.data, 0, MAX_INPUT);
                    strcpy(input_buffer.data, history.commands[history_index].data);
                    index = input_buffer.length = strlen(input_buffer.data);
                    mvwprintw(output_win, line, strlen(prompt), "%s", input_buffer.data);
                    wmove(output_win, line, strlen(prompt) + index);
                }
                break;

            case KEY_DOWN:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Navigate through the command history
                if (history_index < history.length - 1)
                {
                    history_index++;
                    strcpy(input_buffer.data, history.commands[history_index].data);
                }
                else if (history_index == history.length - 1)
                {
                    history_index = -1;
                    memset(input_buffer.data, 0, MAX_INPUT);
                }
                input_buffer.length = index = strlen(input_buffer.data);

                // Clear and redraw the input buffer
                wmove(output_win, line, strlen(prompt));
                wclrtoeol(output_win);
                mvwprintw(output_win, line, strlen(prompt), "%s", input_buffer.data);
                wmove(output_win, line, strlen(prompt) + index);

                break;

            case KEY_LEFT:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Move cursor left
                if (index > 0)
                    wmove(output_win, line, strlen(prompt) + --index);
                break;

            case KEY_RIGHT:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Move cursor right
                if (index < input_buffer.length)
                    wmove(output_win, line, strlen(prompt) + ++index);
                break;

            case KEY_ESC:
                handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);
                // Clear the line
                index = 0;
                wmove(output_win, line, strlen(prompt));
                memset(input_buffer.data, 0, input_buffer.length);
                wclrtoeol(output_win);
                break;

            case KEY_SR: // Scroll up
                if (shell_at_bottom() && scroll_his.length - LINES - scroll_offset + 2 > 0)
                {
                    scroll_offset++;                           // Increase offset when scrolling up
                    redraw_output(&scroll_his, scroll_offset); // Redraw with new offset
                }
                break;

            case KEY_SF: // Scroll down
                if (scroll_offset > 0)
                {
                    scroll_offset--;                           // Decrease offset when scrolling down
                    redraw_output(&scroll_his, scroll_offset); // Redraw with new offset
                }
                else if (scroll_offset == 0)
                {
                    redraw_output(&scroll_his, scroll_offset);                                 // Redraw with new offset
                    mvwprintw(output_win, LINES - 2, 0, "%s", prompt);                         // Redraw prompt on the last visible line
                    mvwprintw(output_win, LINES - 2, strlen(prompt), "%s", input_buffer.data); // Redraw current input line
                    wmove(output_win, LINES - 2, strlen(prompt) + index);                      // Move cursor to the end of input line
                }
                break;

            default:
                if (ch >= 32 && ch <= 126 && index < LINE_LENGTH - 1) // Printable characters
                {
                    handle_scroll_reset(&scroll_offset, prompt, input_buffer.data, index);

                    history_index = -1;              // Reset history navigation
                    input_buffer.data[index++] = ch; // Add the character to input buffer
                    input_buffer.length++;

                    mvwaddch(output_win, line, strlen(prompt) + index - 1, ch); // Display the typed character
                    wmove(output_win, line, strlen(prompt) + index);            // Move cursor after typed character
                }
                break;
            }
            wrefresh(output_win);
        }

        /* Adding line to history */
        char buff[LINE_LENGTH];                   // Create buffer to store line data
        strcpy(buff, prompt);                     // Copy the shell prompt into buffer
        strcat(buff, input_buffer.data);          // Append the command into buffer
        add_to_scroll_history(&scroll_his, buff); // Add the line into scroll history

        free(prompt); // Free the memory that was allocated by get_shell_prompt()

        /* Adding comamnd to history */
        if (input_buffer.data[0]) // Add to history only if there is input
        {
            add_to_history(&history, &input_buffer);
            history_index = history.length; // Set to end of history
        }

        handle_command(input_buffer.data, &history_index, &index, &scroll_offset);

        memset(input_buffer.data, 0, input_buffer.length);
        input_buffer.length = 0;
        adjust_window();
        wrefresh(output_win);
    }
}

void init_ncurses(void)
{
    // Initialize ncurses
    initscr();
    if (stdscr == NULL)
    {
        perror("Error initializing ncurses");
        exit(EXIT_FAILURE);
    }

    cbreak();
    noecho();

    // Create a new window
    output_win = newwin(LINES - 1, COLS, 0, 0); // Create a window with screen size minus 1 row
    if (output_win == NULL)
    {
        perror("Error creating new window");
        endwin();
        exit(EXIT_FAILURE);
    }

    keypad(output_win, TRUE);
    scrollok(output_win, TRUE); // Allow scrolling in the window
    wrefresh(output_win);       // Refresh to show the window
}

void add_to_history(History *history, const Command *command)
{
    if (history->length < MAX_HISTORY)
    {
        // Add new command to history
        history->commands[history->length] = *command;
        history->length++;
    }
    else
    {
        // Shift commands to make room for the new one
        for (size_t i = 0; i < MAX_HISTORY - 1; i++)
        {
            history->commands[i] = history->commands[i + 1];
        }
        history->commands[MAX_HISTORY - 1] = *command;
    }
}

char *get_shell_prompt(void)
{
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
        return strdup("my-shell $ "); // fallback in case getcwd fails

    char *hd = get_home(); // Get the user's home directory
    bool hd_in_cwd = true;

    // Check if the home directory path is a prefix of cwd
    if (strlen(cwd) >= strlen(hd))
    {
        for (int i = 0; hd[i] != 0; i++)
        {
            if (hd[i] != cwd[i])
            {
                hd_in_cwd = false;
                break;
            }
        }

        // If the home directory is a prefix, replace it with "~"
        if (hd_in_cwd)
        {
            // Allocate memory for the prompt and replace /home/{$USER} with ~
            size_t offset = strlen(hd);
            char *shortened_cwd = cwd + offset; // Get the part after home directory
            char *prompt = malloc(SHELL_AND_WD_MAX_LENGTH);
            snprintf(prompt, SHELL_AND_WD_MAX_LENGTH, "my-shell ~%s$ ", shortened_cwd);
            free(cwd);
            free(hd);
            return prompt;
        }
    }

    // If the cwd is not in home, just display the full cwd
    char *prompt = malloc(SHELL_AND_WD_MAX_LENGTH);
    snprintf(prompt, SHELL_AND_WD_MAX_LENGTH, "my-shell %s$ ", cwd);
    free(cwd);
    free(hd);
    return prompt;
}

char *get_home(void)
{
    char *user = getenv("USER");
    if (!user)
        return strdup("/home/"); // Fallback if getenv fails

    // Allocate memory for home directory and construct it
    char *home = malloc(strlen("/home/") + strlen(user) + 1);
    strcpy(home, "/home/");
    strcat(home, user);

    return home;
}

inline void adjust_window(void)
{
    if (line >= LINES - 2) // Check if we need to scroll
    {
        wscrl(output_win, 1);
        line = LINES - 2; // Keep cursor on the last visible line
    }
    else
    {
        line++;
    }
}

void add_to_scroll_history(Scroll_History *history, char *data)
{
    if (history->length < MAX_SCROLLBACK)
        // Add new command to history
        strcpy(history->data[history->length++], data);
    else
    {
        // Shift commands to make room for the new one
        for (size_t i = 0; i < MAX_SCROLLBACK - 1; i++)
        {
            strcpy(history->data[i], history->data[i + 1]);
        }
        strcpy(history->data[MAX_SCROLLBACK - 1], data);
    }
}

void redraw_output(Scroll_History *history, int offset)
{
    werase(output_win); // Clear the window

    size_t l = 0;
    int start = history->length - LINES - offset + 2;
    for (int i = start; i < history->length; i++)
    {
        mvwprintw(output_win, l++, 0, "%s", history->data[i]);
    }
}

// Process command
void handle_command(char *command, int *history_index, int *index, int *scroll_offset)
{
    // Make a copy of the original command so strtok doesn't modify it
    char command_copy[strlen(command) + 1];
    strcpy(command_copy, command);

    char *token = strtok(command_copy, " ");
    if (token)
    {
        if (strcmp("about", token) == 0)
            execute_about();
        else if (strcmp("greet", token) == 0)
            execute_greet(strtok(NULL, ""));
        else if (strcmp("clear", token) == 0)
            execute_clear(history_index, index, scroll_offset);
        else if (strcmp("echo", token) == 0)
            execute_echo(strtok(NULL, ""));
        else if (strcmp("time", token) == 0)
            execute_time();
        else if (strcmp("cd", token) == 0)
        {
            if (chdir(strtok(NULL, " ")) == -1)
            {
                adjust_window();
                mvwprintw(output_win, line, 0, "%s", strerror(errno));
            }
        }
        else if (strcmp("pwd", token) == 0)
        {
            adjust_window();
            char *cwd = getcwd(NULL, 0);
            mvwprintw(output_win, line, 0, "%s", cwd);
            free(cwd); // Free memory allocated by getcwd
        }
        else if (check_bins(token, command))
            execute_bin(command); // Pass original command here
        else
        {
            adjust_window();
            unsigned int len = strlen(token) + strlen("`` command is unknown! Type `help` for a list of valid commands.");
            char *error_msg = malloc(len);
            snprintf(error_msg, len, "`%s` command is unknown! Type `help` for a list of valid commands.", token);
            mvwprintw(output_win, line, 0, "%s", error_msg);
            add_to_scroll_history(&scroll_his, error_msg);
            free(error_msg);
        }
    }
}

void handle_scroll_reset(int *offset, char *prompt, char *data, int index)
{
    if (*offset >= 0)
    {
        *offset = 0;                                                  // Reset scroll offset
        redraw_output(&scroll_his, *offset);                          // Redraw output with new offset
        mvwprintw(output_win, LINES - 2, 0, "%s", prompt);            // Redraw prompt on the last visible line
        mvwprintw(output_win, LINES - 2, strlen(prompt), "%s", data); // Redraw current input line
        wmove(output_win, LINES - 2, strlen(prompt) + index);         // Move cursor to end of input line
    }
}

bool shell_at_bottom(void) { return line == LINES - 2; }

bool check_bins(char *token, char *command)
{
    char *bins[NUM_COMMANDS] = {"ls", "mv", "mkdir", "cp", "rm", "rmdir", "touch", "cat", "find", "grep", "gzip", "gunzip", "tar", "whoami"};
    for (int i = 0; i < NUM_COMMANDS; i++)
        if (strcmp(token, bins[i]) == 0)
            return true;
    return false;
}