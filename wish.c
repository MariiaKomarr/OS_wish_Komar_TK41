#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc_main, char *argv_main[]) {
    char *line = NULL;
    size_t len = 0;

    FILE *input = stdin;
    int interactive = 1;

    // Check command-line arguments
    if (argc_main > 2) {
        printf("An error has occurred\n");
        exit(1);
    }

    // Batch mode
    if (argc_main == 2) {
        input = fopen(argv_main[1], "r");

        if (input == NULL) {
            printf("An error has occurred\n");
            exit(1);
        }

        interactive = 0;
    }

    // Initial search path
    char *paths[100];
    int path_count = 1;

    paths[0] = strdup("/bin");

    while (1) {

        // Print prompt only in interactive mode
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        // Read command
        if (getline(&line, &len, input) == -1) {
            break;
        }

        // Remove newline character
        line[strcspn(line, "\n")] = '\0';

        // Skip empty input
        if (strlen(line) == 0) {
            continue;
        }

        // Parse command into arguments
        char *args[100];
        int argc = 0;

        char *token = strtok(line, " \t");

        while (token != NULL && argc < 99) {
            args[argc] = token;
            argc++;

            token = strtok(NULL, " \t");
        }

        args[argc] = NULL;

        // If nothing was parsed
        if (argc == 0) {
            continue;
        }

        // Built-in exit command
        if (strcmp(args[0], "exit") == 0) {

            if (argc != 1) {
                printf("An error has occurred\n");
                continue;
            }

            for (int i = 0; i < path_count; i++) {
                free(paths[i]);
            }

            if (!interactive) {
                fclose(input);
            }

            free(line);
            exit(0);
        }

        // Built-in path command
        if (strcmp(args[0], "path") == 0) {

            // Free old paths
            for (int i = 0; i < path_count; i++) {
                free(paths[i]);
            }

            path_count = 0;

            // Store new paths
            for (int i = 1; i < argc; i++) {
                paths[path_count] = strdup(args[i]);
                path_count++;
            }

            continue;
        }

        // Built-in cd command
        if (strcmp(args[0], "cd") == 0) {

            if (argc != 2) {
                printf("An error has occurred\n");
                continue;
            }

            if (chdir(args[1]) != 0) {
                printf("An error has occurred\n");
            }

            continue;
        }

        // Search executable in paths
        char full_path[256];
        int command_found = 0;

        for (int i = 0; i < path_count; i++) {

            snprintf(
                full_path,
                sizeof(full_path),
                "%s/%s",
                paths[i],
                args[0]
            );

            if (access(full_path, X_OK) == 0) {
                command_found = 1;
                break;
            }
        }

        // Command not found
        if (!command_found) {
            printf("An error has occurred\n");
            continue;
        }

        // Create child process
        pid_t pid = fork();

        if (pid == 0) {

            // Child process
            execv(full_path, args);

            // execv returns only if something went wrong
            printf("An error has occurred\n");
            exit(1);

        } else if (pid > 0) {

            // Parent waits for child
            wait(NULL);

        } else {

            // fork failed
            printf("An error has occurred\n");
        }
    }

    // Free allocated paths
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }

    // Close batch file
    if (!interactive) {
        fclose(input);
    }

    free(line);

    return 0;
}