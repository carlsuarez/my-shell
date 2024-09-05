#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#define INITIAL_CAPACITY 64 // Start with a reasonably large buffer
#define RESIZE_FACTOR 2     // Double the size when reallocating
#define SHELL "my-shell$ "
#define CTRL_Q 17

// Define string
typedef struct
{
    char *data;
    size_t length;
    size_t capacity;
    size_t cursor;
} string;

// Prototypes
void resize_string(string *cmd);
void handle_input(string *cmd, int ch);
void handle_backspace(string *cmd);
void update_display(const string *cmd, size_t line);

int main()
{
    initscr();            // Start ncurses mode
    raw();                // Get raw input (disable line buffering)
    keypad(stdscr, TRUE); // Enable function keys
    noecho();             // Don't echo typed characters to the screen

    size_t line = 0;

    // Initialize the command
    string command;
    command.capacity = INITIAL_CAPACITY;
    command.length = 0;
    command.cursor = 0;
    command.data = (char *)malloc(command.capacity * sizeof(char));
    if (!command.data)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

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
        case KEY_ENTER:
            command.data[command.length] = '\0'; // Null-terminate the string
            line++;
            command.cursor = command.length; // Move cursor to the end of the command
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
        case 127: // 127 is often the ASCII value for backspace
            handle_backspace(&command);
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

    mvprintw(line, 0, "Exiting...\n");
    refresh();
    endwin(); // End ncurses mode

    free(command.data);
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
    move(line, 0); // Move to the start of the line
    clrtoeol();    // Clear to end of line

    // Print the shell prompt and command
    mvprintw(line, 0, SHELL "%s", cmd->data);

    // Move the cursor to the correct position
    move(line, strlen(SHELL) + cmd->cursor);

    // Refresh to apply changes
    refresh();
}
