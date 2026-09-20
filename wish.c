#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char *line = NULL;
    size_t len = 0;

    // Initial search path
    char *paths[100];
    int path_count = 1;

    paths[0] = strdup("/bin");

    while (1) {
        printf("wish> ");

        if (getline(&line, &len, stdin) == -1) {
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

        char *token = strtok(line, " ");

        while (token != NULL && argc < 99) {
            args[argc] = token;
            argc++;

            token = strtok(NULL, " ");
        }

        args[argc] = NULL;

        // Built-in exit command
        if (strcmp(args[0], "exit") == 0) {
            if (argc != 1) {
                printf("An error has occurred\n");
                continue;
            }

            free(paths[0]);
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

        if (!command_found) {
            printf("An error has occurred\n");
            continue;
        }

        // Create child process
        pid_t pid = fork();

        if (pid == 0) {
            execv(full_path, args);

            printf("An error has occurred\n");
            exit(1);

        } else if (pid > 0) {

            wait(NULL);

        } else {

            printf("An error has occurred\n");
        }
    }

    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }

    free(line);

    return 0;
}