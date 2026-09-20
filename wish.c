#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char *line = NULL;
    size_t len = 0;

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

        // Built-in exit command
        if (strcmp(line, "exit") == 0) {
            free(line);
            exit(0);
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

        // Create child process
        pid_t pid = fork();

        if (pid == 0) {

            char path[256];

            snprintf(path, sizeof(path), "/bin/%s", args[0]);

            execv(path, args);

            // execv returns only if an error occurs
            printf("An error has occurred\n");
            exit(1);

        } else if (pid > 0) {
            // Parent process

            wait(NULL);

        } else {
            // fork failed

            printf("An error has occurred\n");
        }
    }

    free(line);

    return 0;
}