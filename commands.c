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

void execute_clear(int *history_index, int *index, int *scroll_offset)
{
    line = -1;           // Reset the line to the top
    *history_index = -1; // Reset history index
    *index = 0;          // Reset index
    *scroll_offset = -1; // Reset scroll_offset
    for (int i = 0; i < MAX_SCROLLBACK; i++)
        memset(scroll_his.data[i], 0, LINE_LENGTH);
    scroll_his.length = 0;
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

void execute_bin(char *input)
{
    // Tokenize the input string into command and arguments
    char *args[MAX_ARGS];
    int i = 0;
    char *token = strtok(input, " \t\n");

    while (token && i < MAX_ARGS - 1)
    {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        adjust_window();
        mvwprintw(output_win, line, 0, "Error: failed to create pipe.");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) // Fork failed
    {
        adjust_window();
        mvwprintw(output_win, line, 0, "Error: fork failed.");
        return;
    }
    else if (pid == 0) // Child process
    {
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        execvp(args[0], args);
        _exit(127); // Exit child if execvp fails
    }
    else // Parent process
    {
        close(pipefd[1]); // Close write end in parent
        char buffer[BUFFER_SIZE];
        FILE *fp = fdopen(pipefd[0], "r");

        if (fp != NULL)
        {
            while (fgets(buffer, sizeof(buffer), fp))
            {
                // Remove trailing newline if it exists
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n')
                {
                    buffer[len - 1] = '\0';
                }

                adjust_window();
                mvwprintw(output_win, line, 0, "%s", buffer);
                add_to_scroll_history(&scroll_his, buffer);
                wrefresh(output_win);
            }
            fclose(fp);
        }
        close(pipefd[0]);

        // Wait for child process to finish
        waitpid(pid, NULL, 0);
    }
}
