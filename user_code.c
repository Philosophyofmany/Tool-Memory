#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void profile_user_code(char *user_program) {
    printf("Executing user program: %s\n", user_program);

    // Execute user program
    char command[256];
    snprintf(command, sizeof(command), "%s", user_program);

    int result = system(command);
    if (result == -1) {
        printf("Failed to execute program: %s\n", user_program);
    } else {
        printf("User program executed successfully.\n");
    }
}
