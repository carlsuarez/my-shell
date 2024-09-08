#include "commands.h"

void execute_about()
{
    adjust_window();
    mvwprintw(output_win, line, 0, "Welcome to my-shell. A simple shell for beginners!");
    wrefresh(output_win);
}

void execute_greet(char *name)
{
    adjust_window();
    mvwprintw(output_win, line, 0, "Hello, %s", name ? name : "John Doe (please provide a name after `greet`)");
    char buff[LINE_LENGTH] = "Hello, ";
    strcat(buff, name ? name : "John Doe (please provide a name after `greet`)");
    add_to_scroll_history(&scroll_his, buff);
    wrefresh(output_win);
}

void execute_clear()
{
    line = -1; // Reset the line to the top
    for (int i = 0; i < MAX_SCROLLBACK; i++)
        memset(scroll_his.data[i], 0, LINE_LENGTH);
    werase(output_win);   // Clear the window
    wrefresh(output_win); // Refresh to clear the screen
}

void execute_echo(char *message)
{
    adjust_window();
    mvwprintw(output_win, line, 0, "%s", message ? message : "*cricket noises*");
    char buff[LINE_LENGTH];
    strcat(buff, message ? message : "*cricket noises*");
    add_to_scroll_history(&scroll_his, buff);
    wrefresh(output_win);
}

void execute_time()
{
    adjust_window();

    char buff[LINE_LENGTH];
    // Get the current time
    time_t current_time = time(NULL);

    // Check if the time retrieval was successful
    if (current_time == (time_t)-1)
    {
        char *message = "Failed to get the current time.";
        mvwprintw(output_win, line, 0, "%s", message);
        strcpy(buff, message);
        add_to_scroll_history(&scroll_his, buff);
        wrefresh(output_win);
        return;
    }

    // Convert to local time format
    struct tm *local_time = localtime(&current_time);
    if (!local_time)
    {
        char *message = "Failed to convert to local time.";
        mvwprintw(output_win, line, 0, "Failed to convert to local time.");
        strcpy(buff, message);
        add_to_scroll_history(&scroll_his, buff);
        wrefresh(output_win);
        return;
    }

    // Format and print the time
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    mvwprintw(output_win, line, 0, "Current date/time: %s", time_str);

    strcpy(buff, "Current date/time: ");
    strcat(buff, time_str);
    add_to_scroll_history(&scroll_his, buff);
}

void execute_ls(char *input)
{
    // Open a pipe to capture the output of `ls` with arguments
    FILE *fp = popen(input, "r");
    if (fp == NULL)
    {
        perror("popen failed");
        return;
    }

    // Read and print the output of the `ls` command to the ncurses window
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        // Strip the newline character, if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0'; // Replace newline with null terminator

        adjust_window();
        mvwprintw(output_win, line, 0, "%s", buffer);
        add_to_scroll_history(&scroll_his, buffer);
    }

    pclose(fp);
    wrefresh(output_win); // Refresh window to show updates
}
