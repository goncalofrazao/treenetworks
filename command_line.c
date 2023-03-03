#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_line.h"

/**
 * @brief check if valid ip
 * 
 * @param ipstr ip in string
 * @return true valid ip
 * @return false invalid ip
 */
bool valid_ip(char *ipstr)
{
    // check lenght
    int iplen = strlen(ipstr);
    if (iplen < 6 || iplen > 15)
    {
        return false;
    }

    // check format
    int ip[4];
    if(sscanf(ipstr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4)
    {
        return false;
    }

    // check ip values
    for (int i = 0; i < 4; i++)
    {
        if (ip[i] < 0 || ip[i] > 255)
        {
            return false;
        }
    }
    
    return true;
}

bool valid_port(char *portstr)
{
    // convert string to integer
    int port = atoi(portstr);

    // check port value
    if (port < 0 || port > 65535)
    {
        return false;
    }

    return true;
}

/**
 * @brief Validate command line arguments
 * 
 * @param argc
 * @param argv
 * @return true if correct call
 * @return false if incorrect call
 */
bool valid_command_line(int argc, char *argv[])
{
    // validate number of arguments
    if (argc != 3 && argc != 5)
    {
        return false;
    }

    // validate arguments
    if (!valid_ip(argv[1]) || !valid_port(argv[2]))
    {
        return false;
    }
    if (argc == 5 && (!valid_ip(argv[3]) || !valid_port(argv[4])))
    {
        return false;
    }

    return true;
}
