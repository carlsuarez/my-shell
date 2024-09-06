#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>

#define INITIAL_CAPACITY 64
#define RESIZE_FACTOR 2
#define SHELL "my-shell$ "
#define ENTER 10
#define CTRL_Q 17
#define MAX_HISTORY 32

typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
    size_t cursor;
} string;

typedef struct
{
    char *data[MAX_HISTORY];
    size_t length;
} strings;

// Prototypes
void resize_string(string *cmd);
void reset_command(string *cmd);
void handle_input(string *cmd, int ch);
void handle_backspace(string *cmd);
void update_display(const string *cmd);
void add_to_history(strings *history, const string *cmd);
void get_from_history(strings *history, string *cmd, int index);

// Commands
void execute_about();
void execute_greet(char *name);
void execute_clear();
void execute_echo();
void execute_time();

// Global variable
static size_t line = 0;

int main(void)
{
    initscr();            // Initialize ncurses
    raw();                // Disable line buffering
    keypad(stdscr, TRUE); // Enable function keys
    noecho();             // Disable echo of input

    strings command_his = {0}; // Initialize history
    string command = {malloc(INITIAL_CAPACITY * sizeof(char)), 0, INITIAL_CAPACITY, 0};

    if (!command.data)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int history_index = -1;
    bool QUIT = false;

    while (!QUIT)
    {
        mvprintw(line, 0, SHELL);  // Print the shell prompt
        move(line, strlen(SHELL)); // Move the cursor after the prompt

        int ch = getch();
        switch (ch)
        {
        case CTRL_Q:
            QUIT = true;
            break;
        case ENTER:
            if (command.length > 0)
            {
                char *token = strtok(command.data, " ");

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
                    execute_echo();
                }
                else if (strcmp("time", token) == 0)
                {
                    execute_time();
                }
                else
                {
                    mvprintw(++line, 0, "`%s` command is unknown! Type `help` for a list of valid commands.", token);
                }
                add_to_history(&command_his, &command);
            }
            reset_command(&command);
            history_index = -1;
            line++;
            break;
        case KEY_LEFT:
            if (command.cursor > 0)
                command.cursor--;
            break;
        case KEY_RIGHT:
            if (command.cursor < command.length)
                command.cursor++;
            break;
        case KEY_BACKSPACE:
            handle_backspace(&command);
            break;
        case KEY_UP:
            if (command_his.length > 0 && history_index < (int)(command_his.length - 1))
            {
                history_index++;
                get_from_history(&command_his, &command, command_his.length - 1 - history_index);
            }
            break;
        case KEY_DOWN:
            if (history_index > 0)
            {
                history_index--;
                get_from_history(&command_his, &command, command_his.length - 1 - history_index);
            }
            else if (history_index == 0)
            {
                history_index--;
                reset_command(&command);
            }
            break;
        default:
            if (ch >= 32 && ch <= 126)
            {
                handle_input(&command, ch);
            }
            break;
        }

        update_display(&command);
    }

    mvprintw(++line, 0, "Exiting...\n");
    refresh();
    endwin(); // End ncurses mode

    free(command.data); // Free allocated memory for command data
    for (size_t i = 0; i < command_his.length; ++i)
    {
        free(command_his.data[i]); // Free history data
    }

    return 0;
}

// Resize the command buffer when necessary
void resize_string(string *cmd)
{
    cmd->capacity *= RESIZE_FACTOR;
    char *new_data = realloc(cmd->data, cmd->capacity * sizeof(char));
    if (!new_data)
    {
        perror("realloc");
        free(cmd->data);
        exit(EXIT_FAILURE);
    }
    cmd->data = new_data;
}

// Reset command string and shrink buffer if needed
void reset_command(string *cmd)
{
    memset(cmd->data, 0, cmd->length);
    cmd->length = 0;
    cmd->cursor = 0;
    char *new_data = realloc(cmd->data, INITIAL_CAPACITY * sizeof(char));
    if (!new_data)
    {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    cmd->data = new_data;
    cmd->capacity = INITIAL_CAPACITY;
}

// Insert input character at the cursor position
void handle_input(string *cmd, int ch)
{
    if (cmd->length + 1 >= cmd->capacity)
    {
        resize_string(cmd);
    }
    memmove(cmd->data + cmd->cursor + 1, cmd->data + cmd->cursor, cmd->length - cmd->cursor);
    cmd->data[cmd->cursor++] = ch;
    cmd->length++;
}

// Handle backspace key
void handle_backspace(string *cmd)
{
    if (cmd->cursor > 0)
    {
        memmove(cmd->data + cmd->cursor - 1, cmd->data + cmd->cursor, cmd->length - cmd->cursor);
        cmd->length--;
        cmd->cursor--;
        cmd->data[cmd->length] = '\0'; // Null-terminate the string
    }
}

// Update display with current command
void update_display(const string *cmd)
{
    mvprintw(line, 0, SHELL);                       // Print the shell prompt
    clrtoeol();                                     // Clear the line from the cursor to the end
    mvprintw(line, strlen(SHELL), "%s", cmd->data); // Print the current command
    move(line, strlen(SHELL) + cmd->cursor);        // Move cursor to the correct position
    refresh();
}

// Add a command to history
void add_to_history(strings *history, const string *cmd)
{
    if (history->length < MAX_HISTORY)
    {
        history->data[history->length] = strdup(cmd->data);
        history->length++;
    }
    else
    {
        free(history->data[0]);
        memmove(&history->data[0], &history->data[1], (MAX_HISTORY - 1) * sizeof(char *));
        history->data[MAX_HISTORY - 1] = strdup(cmd->data);
    }
}

// Retrieve command from history
void get_from_history(strings *history, string *cmd, int index)
{
    if (index >= 0 && index < (int)history->length)
    {
        strcpy(cmd->data, history->data[index]);
        cmd->length = strlen(history->data[index]);
        cmd->cursor = cmd->length;
    }
}

// Command implementations
void execute_about()
{
    mvprintw(++line, 0, "Welcome to my-shell. A simple shell for beginners!");
}

void execute_greet(char *name)
{
    mvprintw(++line, 0, "Hello, %s", name ? name : "John Doe (please provide a name after `greet`)");
}

void execute_clear()
{
    line = -1;
    clear();
}

void execute_echo()
{
    char *token = strtok(NULL, " ");
    if (!token)
    {
        mvprintw(++line, 0, "*cricket noises*");
        return;
    }
    mvprintw(++line, 0, "%s", token);
    while ((token = strtok(NULL, " ")))
    {
        printw(" %s", token);
    }
}

void execute_time()
{
    // Get the current time
    time_t current_time = time(NULL);

    // Check if the time retrieval was successful
    if (current_time == -1)
    {
        mvprintw(++line, 0, "Failed to get the current time.");
        return;
    }

    // Convert to local time format
    struct tm *local_time = localtime(&current_time);
    if (!local_time)
    {
        mvprintw(++line, 0, "Failed to convert to local time.");
        return;
    }

    // Format and print the time
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    mvprintw(++line, 0, "Current date/time: %s", time_str);
}
