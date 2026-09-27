#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 100
#define MAX_PATHS 100
#define MAX_COMMANDS 100

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

pid_t execute_external_command(
    char *command,
    char *paths[],
    int path_count
) {
    char *redirect_file = NULL;

    // Check redirection
    char *redirect_symbol = strchr(command, '>');

    if (redirect_symbol != NULL) {

        // More than one >
        if (strchr(redirect_symbol + 1, '>') != NULL) {
            print_error();
            return -1;
        }

        *redirect_symbol = '\0';

        char *file_part = redirect_symbol + 1;

        while (*file_part == ' ' || *file_part == '\t') {
            file_part++;
        }

        if (strlen(file_part) == 0) {
            print_error();
            return -1;
        }

        char *file_token = strtok(file_part, " \t");

        if (file_token == NULL) {
            print_error();
            return -1;
        }

        redirect_file = file_token;

        // Only one file after >
        if (strtok(NULL, " \t") != NULL) {
            print_error();
            return -1;
        }
    }

    // Parse command arguments
    char *args[MAX_ARGS];
    int argc = 0;

    char *token = strtok(command, " \t");

    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc] = token;
        argc++;

        token = strtok(NULL, " \t");
    }

    args[argc] = NULL;

    if (argc == 0) {
        print_error();
        return -1;
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
        print_error();
        return -1;
    }

    pid_t pid = fork();

    if (pid == 0) {

        if (redirect_file != NULL) {

            int fd = open(
                redirect_file,
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );

            if (fd < 0) {
                print_error();
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);

            close(fd);
        }

        execv(full_path, args);

        print_error();
        exit(1);

    } else if (pid < 0) {

        print_error();
        return -1;
    }

    return pid;
}

int main(int argc_main, char *argv_main[]) {

    char *line = NULL;
    size_t len = 0;

    FILE *input = stdin;
    int interactive = 1;

    // Incorrect number of arguments
    if (argc_main > 2) {
        print_error();
        exit(1);
    }

    // Batch mode
    if (argc_main == 2) {

        input = fopen(argv_main[1], "r");

        if (input == NULL) {
            print_error();
            exit(1);
        }

        interactive = 0;
    }

    // Initial search path
    char *paths[MAX_PATHS];
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

        // Split line by &
        char *commands[MAX_COMMANDS];
        int command_count = 0;

        char *command_token = strtok(line, "&");

        while (
            command_token != NULL &&
            command_count < MAX_COMMANDS
        ) {
            commands[command_count] = command_token;
            command_count++;

            command_token = strtok(NULL, "&");
        }

        // Store child PIDs
        pid_t child_pids[MAX_COMMANDS];
        int child_count = 0;

        for (int c = 0; c < command_count; c++) {

            char *command = commands[c];

            // Remove leading spaces
            while (*command == ' ' || *command == '\t') {
                command++;
            }

            // Skip empty command
            if (strlen(command) == 0) {
                continue;
            }

            // Make copy because strtok changes strings
            char command_copy[1024];
            strncpy(
                command_copy,
                command,
                sizeof(command_copy) - 1
            );

            command_copy[sizeof(command_copy) - 1] = '\0';

            // Parse temporary args for built-ins
            char *args[MAX_ARGS];
            int argc = 0;

            char *token = strtok(command_copy, " \t");

            while (
                token != NULL &&
                argc < MAX_ARGS - 1
            ) {
                args[argc] = token;
                argc++;

                token = strtok(NULL, " \t");
            }

            args[argc] = NULL;

            if (argc == 0) {
                continue;
            }

            // Built-in exit

            if (strcmp(args[0], "exit") == 0) {

                if (argc != 1) {
                    print_error();
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

            // Built-in cd

            if (strcmp(args[0], "cd") == 0) {

                if (argc != 2) {
                    print_error();
                    continue;
                }

                if (chdir(args[1]) != 0) {
                    print_error();
                }

                continue;
            }

            // Built-in path

            if (strcmp(args[0], "path") == 0) {

                for (int i = 0; i < path_count; i++) {
                    free(paths[i]);
                }

                path_count = 0;

                for (int i = 1; i < argc; i++) {

                    if (path_count < MAX_PATHS) {
                        paths[path_count] = strdup(args[i]);
                        path_count++;
                    }
                }

                continue;
            }

            // External command

            char external_copy[1024];

            strncpy(
                external_copy,
                command,
                sizeof(external_copy) - 1
            );

            external_copy[
                sizeof(external_copy) - 1
            ] = '\0';

            pid_t pid = execute_external_command(
                external_copy,
                paths,
                path_count
            );

            if (pid > 0) {
                child_pids[child_count] = pid;
                child_count++;
            }
        }

        // Wait only after all commands were started
        for (int i = 0; i < child_count; i++) {
            waitpid(child_pids[i], NULL, 0);
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