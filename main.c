#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "requests.h"
#include "commands.h"
#include "command_line.h"

int main(int argc, char *argv[])
{
    char buffer[MAX_LINE];
    CAAS command;

    fd_set current_fds, ready_fds;
    
    // validate command-line arguments
    if (!valid_command_line(argc, argv))
    {
        printf("USAGE: ./cot IP TCP regIP regUDP\n");
        exit(1);
    }

    // initialize current set
    FD_ZERO(&current_fds);
    FD_SET(0, &current_fds);

    // deal with interruptions
    
    while(fgets(buffer, MAX_LINE, stdin))
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
    
    // close fds
    
}
