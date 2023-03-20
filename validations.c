#include "validations.h"

bool valid_ip(char strip[])
{
    int ip[4];

    if (sscanf(strip, "%d.%d.%d.%d", ip, ip + 1, ip + 2, ip + 3) != 4) {
        return false;
    }

    for (int *i = ip; i < ip + 4; i++) {
        if ((*i) > 255 || (*i) < 0) {
            return false;
        }
    }
    
    return true;
}

bool valid_port(char strport[])
{
    int port;

    if (sscanf(strport, "%d", &port) != 1) {
        return false;
    }

    if (port < 0 || port > 65535) {
        return false;
    }

    return true;
}

bool valid_command_line_arguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 3:
        if (valid_ip(argv[1]) && valid_port(argv[2])) {
            return true;
        }
        else {
            return false;
        }
        break;
    
    case 5:
        if (valid_ip(argv[1]) && valid_port(argv[2]) && valid_ip(argv[3]) && valid_port(argv[4])) {
            return true;
        }
        else {
            return false;
        }
        break;

    default:
        return false;
        break;
    }
}

int already_in_network(char new_id[], char network_info[])
{
    char ip[16], port[6], id[3];
    char *c;

    c = strtok(network_info, "\n");

    while ((c = strtok(NULL, "\n")) != NULL) {
        if (sscanf(c, "%s %s %s", id, ip, port) != 3) {
            return -1;
        }
        if (strcmp(id, new_id) == 0) {
            return 1;
        }
    }

    return 0;
}

void choose_new_id(app_t *me)
{
    
}