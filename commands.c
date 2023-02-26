#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "commands.h"

bool valid_arguments(CAAS* command);

/**
 * @brief Get the command and parameters
 * 
 * @param buffer input line
 * @param command command saving struct
 */
void get_command(char buffer[], CAAS* command)
{
    char *c = strtok(buffer, " \n\t");

    // identify command and save arguments
    if (strcmp(c, "join") == 0)
    {
        command->command = join;

        // save arguments
        for (int i = 0; i < 2; i++)
        {
            c = strtok(NULL, " \n\t");
            // handle less arguments
            if (c == NULL)
            {
                command->command = UNKOWN;
                break;
            }
            
            strcpy(command->args[i], c);
        }
    }
    else if (strcmp(c, "leave") == 0)
    {
        command->command = leave;
    }
    else
    {
        command->command = UNKOWN;
    }

    // handle too many arguments case
    if (strtok(NULL, " \n\t") != NULL)
    {
        command->command = UNKOWN;
    }

    // validate arguments
    if (!valid_arguments(command))
    {
        command->command = UNKOWN;
    }
}

/**
 * @brief Validate arguments
 * 
 * @param command command saving struct
 * @return true valid arguments
 * @return false invalid arguments
 */
bool valid_arguments(CAAS* command)
{
    int net, id;

    // validate according to the command
    switch (command->command)
    {
    case join:
        net = atoi(command->args[0]);
        id = atoi(command->args[1]);
        if (strlen(command->args[0]) == 3 && strlen(command->args[1]) == 2 && net >= 0 && net < 1000 && id >= 0 && id < 100)
        {
            return true;
        }
        else
        {
            return false;
        }
        break;
    
    default:
        return true;
        break;
    }
}
