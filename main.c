#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "requests.h"
#include "commands.h"

int main(int argc, char *argv[])
{
    char buffer[MAX_LINE];
    CAAS command;
    while (fgets(buffer, MAX_LINE, stdin))
    {
        get_command(buffer, &command);
        switch (command.command)
        {
        case join:
            /* code */
            printf("JOIN\n");
            break;

        case leave:
            /* code */
            printf("LEAVE\n");
            break;

        default:
            /* code */
            printf("Invalid command\n");
            break;
        }
    }
    
}
