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

        // Built-in exit command
        if (strcmp(line, "exit") == 0) {
            free(line);
            exit(0);
        }

        printf("You entered: %s\n", line);
    }

    free(line);

    return 0;
}