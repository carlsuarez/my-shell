#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include "commands.h"

#define SHELL_AND_WD_MAX_LENGTH 256
#define MAX_INPUT 256
#define MAX_HISTORY 32
#define KEY_ESC 27

// Global variables
WINDOW *output_win;
size_t line;

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
void init_ncurses();
void add_to_history(History *history, const Command *command);
char *get_shell_prompt();

int main(void)
{
    // Initialize the ncurses window
    init_ncurses();

    // Start at the user directory
    chdir("/home/cj-suarez");

    History history = {0};
    Command input_buffer = {0}; // Initialize the input buffer
    int ch;
    int history_index = -1; // Index for history navigation

    while (1)
    {
        char *prompt = get_shell_prompt();
        mvwprintw(output_win, line, 0, "%s", prompt);
        wrefresh(output_win); // Refresh to show the prompt

        memset(input_buffer.data, 0, sizeof(input_buffer.data));
        int index = 0;
        input_buffer.length = 1;

        while ((ch = wgetch(output_win)) != '\n') // Read until Enter key
        {
            switch (ch)
            {
            case KEY_F(2):
                // Exit shell
                endwin(); // End ncurses mode
                return 0;

            case KEY_BACKSPACE:
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
                // Navigate through the command history (move back)
                if (history.length > 0 && history_index > 0)
                {
                    history_index--;
                    wmove(output_win, line, strlen(prompt));
                    wclrtoeol(output_win); // Clear from cursor to the end of the line
                    memset(input_buffer.data, 0, input_buffer.length);
                    strcpy(input_buffer.data, history.commands[history_index].data);
                    index = input_buffer.length = strlen(input_buffer.data);
                    mvwprintw(output_win, line, strlen(prompt), "%s", input_buffer.data);
                    wmove(output_win, line, strlen(prompt) + index);
                }
                else if (history.length > 0 && history_index == -1)
                {
                    // First up arrow press after new input, go to last command
                    history_index = history.length - 1;
                    wmove(output_win, line, strlen(prompt));
                    wclrtoeol(output_win);
                    memset(input_buffer.data, 0, input_buffer.length);
                    strcpy(input_buffer.data, history.commands[history_index].data);
                    index = input_buffer.length = strlen(input_buffer.data);
                    mvwprintw(output_win, line, strlen(prompt), "%s", input_buffer.data);
                    wmove(output_win, line, strlen(prompt) + index);
                }
                break;

            case KEY_DOWN:
                // Navigate through the command history (move forward)
                if (history.length > 0 && history_index < history.length - 1)
                {
                    history_index++;
                    wmove(output_win, line, strlen(prompt));
                    wclrtoeol(output_win);
                    memset(input_buffer.data, 0, input_buffer.length);
                    strcpy(input_buffer.data, history.commands[history_index].data);
                    index = input_buffer.length = strlen(input_buffer.data);
                    mvwprintw(output_win, line, strlen(prompt), "%s", input_buffer.data);
                    wmove(output_win, line, strlen(prompt) + index);
                }
                break;

            case KEY_LEFT:
                // Move cursor left
                if (index > 0)
                {
                    index--;
                    wmove(output_win, line, strlen(prompt) + index);
                }
                break;

            case KEY_RIGHT:
                // Move cursor right
                if (index < input_buffer.length)
                {
                    index++;
                    wmove(output_win, line, strlen(prompt) + index);
                }
                break;

            case KEY_ESC:
                // Clear the line
                index = 0;
                wmove(output_win, line, strlen(prompt));
                wclrtoeol(output_win);
                break;

            default:
                // Handle printable characters
                if (ch >= 32 && ch <= 126 && index < MAX_INPUT - 1)
                {
                    history_index = -1;
                    input_buffer.data[index++] = ch;
                    input_buffer.length++;
                    mvwaddch(output_win, line, strlen(prompt) + index - 1, ch);
                }
                break;
            }
        }

        free(prompt);                               // Free the memory that was allocated by get_shell_prompt()
        input_buffer.data[input_buffer.length] = 0; // Make sure that the input_buffer ends with a 0
        if (input_buffer.data[0])                   // Add to history only if there is input
        {
            add_to_history(&history, &input_buffer);
            history_index = history.length; // Set to end of history
        }

        // Process command (similar to original code)
        char *token = strtok(input_buffer.data, " ");
        if (token)
        {
            if (strcmp("about", token) == 0)
            {
                execute_about();
            }
            else if (strcmp("greet", token) == 0)
            {
                execute_greet(strtok(NULL, ""));
            }
            else if (strcmp("clear", token) == 0)
            {
                execute_clear();
            }
            else if (strcmp("echo", token) == 0)
            {
                execute_echo(strtok(NULL, ""));
            }
            else if (strcmp("time", token) == 0)
            {
                execute_time();
            }
            else if (strcmp("cd", token) == 0)
            {
                chdir(strtok(NULL, " "));
            }
            else if (strcmp("pwd", token) == 0)
            {
                char *cwd = getcwd(NULL, 0);
                mvwprintw(output_win, ++line, 0, "%s", cwd);
                free(cwd); // Free memory allocated by getcwd
            }
            else if (strcmp("ls", token) == 0)
            {
                execute_ls(input_buffer.data);
            }
            else
            {
                mvwprintw(output_win, ++line, 0, "`%s` command is unknown! Type `help` for a list of valid commands.", token);
            }
        }

        line++;
        wrefresh(output_win); // Refresh window to show updates
    }
}

void init_ncurses()
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

char *get_shell_prompt()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
    {
        return strdup("my-shell $ "); // fallback in case getcwd fails
    }

    char *prompt = malloc(SHELL_AND_WD_MAX_LENGTH);
    snprintf(prompt, SHELL_AND_WD_MAX_LENGTH, "my-shell %s$ ", cwd);
    free(cwd); // free the memory allocated by getcwd
    return prompt;
}
