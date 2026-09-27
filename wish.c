#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc_main, char *argv_main[]) {
    char *line = NULL;
    size_t len = 0;

    FILE *input = stdin;
    int interactive = 1;

    if (argc_main > 2) {
        printf("An error has occurred\n");
        exit(1);
    }

    if (argc_main == 2) {
        input = fopen(argv_main[1], "r");

        if (input == NULL) {
            printf("An error has occurred\n");
            exit(1);
        }

        interactive = 0;
    }

    char *paths[100];
    int path_count = 1;

    paths[0] = strdup("/bin");

    while (1) {

        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        if (getline(&line, &len, input) == -1) {
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0) {
            continue;
        }

        // Redirection parsing

        char *redirect_file = NULL;

        char *redirect_symbol = strchr(line, '>');

        if (redirect_symbol != NULL) {

            // Check if there is more than one >
            if (strchr(redirect_symbol + 1, '>') != NULL) {
                printf("An error has occurred\n");
                continue;
            }

            // Split command and file name
            *redirect_symbol = '\0';

            char *file_part = redirect_symbol + 1;

            // Remove spaces before file name
            while (*file_part == ' ' || *file_part == '\t') {
                file_part++;
            }

            if (strlen(file_part) == 0) {
                printf("An error has occurred\n");
                continue;
            }

            // Parse file part
            char *file_token = strtok(file_part, " \t");

            if (file_token == NULL) {
                printf("An error has occurred\n");
                continue;
            }

            redirect_file = file_token;

            // There must be only one file after >
            if (strtok(NULL, " \t") != NULL) {
                printf("An error has occurred\n");
                continue;
            }
        }

        // Parse command

        char *args[100];
        int argc = 0;

        char *token = strtok(line, " \t");

        while (token != NULL && argc < 99) {
            args[argc] = token;
            argc++;

            token = strtok(NULL, " \t");
        }

        args[argc] = NULL;

        if (argc == 0) {
            printf("An error has occurred\n");
            continue;
        }

        // Built-in exit

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

        // Built-in path

        if (strcmp(args[0], "path") == 0) {

            for (int i = 0; i < path_count; i++) {
                free(paths[i]);
            }

            path_count = 0;

            for (int i = 1; i < argc; i++) {
                paths[path_count] = strdup(args[i]);
                path_count++;
            }

            continue;
        }

        // Built-in cd

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

        // Search executable

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

        // Fork

        pid_t pid = fork();

        if (pid == 0) {

            // Redirection

            if (redirect_file != NULL) {

                int fd = open(
                    redirect_file,
                    O_WRONLY | O_CREAT | O_TRUNC,
                    0644
                );

                if (fd < 0) {
                    printf("An error has occurred\n");
                    exit(1);
                }

                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);

                close(fd);
            }

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

    if (!interactive) {
        fclose(input);
    }

    free(line);

    return 0;
}