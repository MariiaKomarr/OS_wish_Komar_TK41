#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

        // debug output
        for (int i = 0; i < argc; i++) {
            printf("args[%d] = %s\n", i, args[i]);
        }
    }

    free(line);

    return 0;
}