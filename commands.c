#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include "commands.h"

void execute_about()
{
    mvwprintw(output_win, ++line, 0, "Welcome to my-shell. A simple shell for beginners!");
    wrefresh(output_win);
}

void execute_greet(char *name)
{
    mvwprintw(output_win, ++line, 0, "Hello, %s", name ? name : "John Doe (please provide a name after `greet`)");
    wrefresh(output_win);
}

void execute_clear()
{
    line = -1;            // Move to the top of the window
    werase(output_win);   // Clear the window
    wrefresh(output_win); // Refresh to clear the screen
}

void execute_echo(char *args)
{
    if (!args)
    {
        mvwprintw(output_win, ++line, 0, "*cricket noises*");
    }
    else
    {
        mvwprintw(output_win, ++line, 0, "%s", args);
    }
    wrefresh(output_win);
}

void execute_time()
{
    // Get the current time
    time_t current_time = time(NULL);

    // Check if the time retrieval was successful
    if (current_time == (time_t)-1)
    {
        mvwprintw(output_win, ++line, 0, "Failed to get the current time.");
        wrefresh(output_win);
        return;
    }

    // Convert to local time format
    struct tm *local_time = localtime(&current_time);
    if (!local_time)
    {
        mvwprintw(output_win, ++line, 0, "Failed to convert to local time.");
        wrefresh(output_win);
        return;
    }

    // Format and print the time
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    mvwprintw(output_win, ++line, 0, "Current date/time: %s", time_str);
    wrefresh(output_win);
}