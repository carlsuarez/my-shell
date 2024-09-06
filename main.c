#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include "commands.h"

#define MAX_INPUT 256
#define MAX_HISTORY 32
#define KEY_ESC 27

// Global variables
WINDOW *output_win;
size_t line;

// Structure to hold command history
typedef struct
{
    char *data[MAX_HISTORY];
    size_t length;
} History;

// Prototypes
void init_ncurses();
void add_to_history(History *history, const char *command);
char *get_shell_prompt();

int main(void)
{
    // Initialize the ncurses window
    init_ncurses();

    chdir("/home/cj-suarez");
    History history = {0};
    char input_buffer[MAX_INPUT];
    int ch;
    int history_index = -1; // Index for history navigation

    while (1)
    {

        mvwprintw(output_win, line, 0, "%s", get_shell_prompt());
        wrefresh(output_win); // Refresh to show the prompt

        // Read input from user
        memset(input_buffer, 0, sizeof(input_buffer));
        int index = 0;
        while ((ch = wgetch(output_win)) != '\n') // Read until Enter key
        {
            switch (ch)
            {
            case KEY_F(1):
                // Exit shell
                endwin(); // End ncurses mode

                // Free allocated memory for history
                for (size_t i = 0; i < history.length; ++i)
                {
                    free(history.data[i]);
                }
                return 0;

            case KEY_BACKSPACE:
                // Delete a character
                if (index > 0)
                {
                    index--;
                    mvwaddch(output_win, line, index + strlen(get_shell_prompt()), ' ');
                    wmove(output_win, line, index + strlen(get_shell_prompt()));
                }
                break;

            case KEY_UP:
                // Navigate through the command history
                if (history.length > 0 && history_index > 0)
                {
                    history_index--;
                    // Clear the line before displaying the previous command
                    wmove(output_win, line, strlen(get_shell_prompt()));
                    wclrtoeol(output_win); // Clear from cursor to the end of the line

                    strcpy(input_buffer, history.data[history_index]);
                    index = strlen(input_buffer);
                    mvwprintw(output_win, line, strlen(get_shell_prompt()), "%s", input_buffer);
                    wmove(output_win, line, strlen(get_shell_prompt()) + index);
                }
                break;

            case KEY_DOWN:
                // Navigate through the command history
                if (history.length > 0 && history_index < history.length - 1)
                {
                    history_index++;
                    // Clear the line before displaying the next command
                    wmove(output_win, line, strlen(get_shell_prompt()));
                    wclrtoeol(output_win);

                    strcpy(input_buffer, history.data[history_index]);
                    index = strlen(input_buffer);
                    mvwprintw(output_win, line, strlen(get_shell_prompt()), "%s", input_buffer);
                    wmove(output_win, line, strlen(get_shell_prompt()) + index);
                }
                else
                {
                    // Clear the input when there are no more history items to show
                    history_index = history.length;
                    input_buffer[0] = '\0';
                    wmove(output_win, line, strlen(get_shell_prompt()));
                    wclrtoeol(output_win);
                }
                break;
            case KEY_ESC:
                // Clear the line
                index = 0;
                wmove(output_win, line, strlen(get_shell_prompt()));
                wclrtoeol(output_win);
            default:
                if (ch >= 32 && ch <= 126)
                { // Handle printable characters
                    if (index < MAX_INPUT - 1)
                    {
                        input_buffer[index++] = ch;
                        mvwaddch(output_win, line, index + strlen(get_shell_prompt()) - 1, ch);
                    }
                }
                break;
            }
        }
        input_buffer[index] = '\0'; // Null-terminate the string

        if (index > 0) // Add to history only if there is input
        {
            add_to_history(&history, input_buffer);
            history_index = history.length; // Set to end of history
        }

        // Process command
        char *token = strtok(input_buffer, " ");
        if (token)
        {
            if (strcmp("about", token) == 0)
            {
                execute_about();
            }
            else if (strcmp("greet", token) == 0)
            {
                execute_greet(strtok(NULL, " "));
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
                mvwprintw(output_win, ++line, 0, "%s", getcwd(NULL, 0));
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

void add_to_history(History *history, const char *command)
{
    if (history->length < MAX_HISTORY)
    {
        history->data[history->length] = strdup(command);
        history->length++;
    }
    else
    {
        free(history->data[0]);
        memmove(&history->data[0], &history->data[1], (MAX_HISTORY - 1) * sizeof(char *));
        history->data[MAX_HISTORY - 1] = strdup(command);
    }
}

char *get_shell_prompt()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
    {
        return "my-shell $ "; // fallback in case getcwd fails
    }

    char *prompt = malloc(256);
    snprintf(prompt, sizeof(prompt), "my-shell %s$ ", cwd);
    free(cwd); // free the memory allocated by getcwd
    return prompt;
}