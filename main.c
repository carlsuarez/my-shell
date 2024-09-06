#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#define INITIAL_CAPACITY 64 // Start with a reasonably large buffer
#define RESIZE_FACTOR 2     // Double the size when reallocating
#define SHELL "my-shell$ "
#define ENTER 10
#define CTRL_Q 17

// Define string
typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
    size_t cursor;
} string;

typedef struct
{
    char *data[32]; // Store 32 previous commands
    size_t length;
} strings;

// Prototypes
void resize_string(string *cmd);
void reset_command(string *cmd);
void handle_input(string *cmd, int ch);
void handle_backspace(string *cmd);
void update_display(const string *cmd, size_t line);
void add_to_history(strings *history, string *cmd);
void get_from_history(strings *history, string *cmd, int index);

int main()
{
    initscr();            // Start ncurses mode
    raw();                // Get raw input (disable line buffering)
    keypad(stdscr, TRUE); // Enable function keys
    noecho();             // Don't echo typed characters to the screen

    size_t line = 0;

    // Initialize history
    strings command_his;
    command_his.length = 0;

    // Initialize the command
    string command;
    command.capacity = INITIAL_CAPACITY;
    command.length = 0;
    command.cursor = 0;
    command.data = (char *)malloc(command.capacity * sizeof(char));
    // Check that malloc worked
    if (!command.data)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int history_index = -1; // To track history navigation
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

            // Add command to history
            if (command.length > 0)
            {
                add_to_history(&command_his, &command);
            }

            // Move to the next line
            line++;

            // Reset the command object
            reset_command(&command);

            history_index = -1; // Reset history navigation
            break;
        case KEY_LEFT:
            if (command.cursor > 0)
            {
                command.cursor--;
            }
            break;
        case KEY_RIGHT:
            if (command.cursor < command.length)
            {
                command.cursor++;
            }
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
                memset(command.data, 0, command.length); // Clear command if at the bottom of history
                command.length = 0;
            }
            break;
        default:
            if (ch >= 32 && ch <= 126)
            { // Only allow printable characters
                handle_input(&command, ch);
            }
            break;
        }

        update_display(&command, line);
    }

    mvprintw(line + 1, 0, "Exiting...\n");
    refresh();
    endwin(); // End ncurses mode

    return 0;
}

// Function to resize the string buffer
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

// Function to reset the cmd after enter is pressed
void reset_command(string *cmd)
{
    // Clear command data
    memset(cmd->data, 0, cmd->capacity); // Use cmd->capacity to avoid overstepping bounds

    // Reset size and cursor
    cmd->length = 0;
    cmd->cursor = 0;

    // Attempt to shrink the buffer back to INITIAL_CAPACITY
    char *new_data = realloc(cmd->data, INITIAL_CAPACITY * sizeof(char));
    if (!new_data)
    {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    cmd->data = new_data;
    cmd->capacity = INITIAL_CAPACITY;
}

// Function to handle input and insert it into the command buffer
void handle_input(string *cmd, int ch)
{
    if (cmd->length + 1 >= cmd->capacity)
    {
        resize_string(cmd);
    }

    // Insert character at the cursor position
    if (cmd->length > cmd->cursor)
    {
        memmove(cmd->data + cmd->cursor + 1, cmd->data + cmd->cursor, cmd->length - cmd->cursor);
    }
    cmd->data[cmd->cursor] = ch;
    cmd->length++;
    cmd->cursor++;
}

// Function to handle backspace and remove character from the command buffer
void handle_backspace(string *cmd)
{
    if (cmd->cursor > 0)
    {
        // Move characters back one position
        memmove(cmd->data + cmd->cursor - 1, cmd->data + cmd->cursor, cmd->length - cmd->cursor);
        cmd->length--;
        cmd->cursor--;
        cmd->data[cmd->length] = '\0'; // Null-terminate the string
    }
}

// Function to update the display with the current command
void update_display(const string *cmd, size_t line)
{
    // Print the shell prompt and command
    mvprintw(line, 0, SHELL "%s", cmd->data);

    // Move the cursor to the correct position
    move(line, strlen(SHELL) + cmd->cursor);

    // Refresh to apply changes
    refresh();
}

// Function to store the command into history
void add_to_history(strings *history, string *cmd)
{
    if (history->length < 32)
    {
        history->data[history->length] = (char *)malloc((cmd->length + 1) * sizeof(char)); // Allocate space for the command
        strcpy(history->data[history->length], cmd->data);                                 // Copy the command string
        history->length++;
    }
    else
    {
        // Free the oldest history entry
        free(history->data[0]);

        // Shift the history up by one
        memmove(&history->data[0], &history->data[1], 31 * sizeof(char *));

        // Allocate space for the new command
        history->data[31] = (char *)malloc((cmd->length + 1) * sizeof(char));
        strcpy(history->data[31], cmd->data);
    }
}

// Function to copy command from history
void get_from_history(strings *history, string *cmd, int index)
{
    if (index >= 0 && index < history->length)
    {
        strcpy(cmd->data, history->data[index]);    // Copy the command from history to the current command buffer
        cmd->length = strlen(history->data[index]); // Set the correct length
        cmd->cursor = cmd->length;                  // Move cursor to end
    }
}