#include "validate.h"

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

int already_in_network(char new_id[], char network_info[], app_t *me)
{
    char ip[16], port[6], id[3];
    char *c;

    c = strtok(network_info, "\n");

    while ((c = strtok(NULL, "\n")) != NULL) {
        if (sscanf(c, "%s %s %s", id, ip, port) != 3) {
            return -1;
        }
        if (strcmp(id, new_id) == 0) {
            printf("\nERROR: UNAVAILABLE ID");
            if (!choose_new_id(me)) {
                printf("\nERROR: PICKING NEW NODE");
                break;
            }
            return 1;
        }
    }

    return 0;
}

bool choose_new_id(app_t *me)
{
    int nodes_in_use[100];
    for (int i = 0; i < 100; i++) {
        nodes_in_use[i] = 0;
    }

    char buffer[BUFFER_SIZE];
    if (ask_for_net_nodes(buffer, me) == -1) {
        return false;
    }
    
    char *c;
    char id[3];
    
    c = strtok(buffer, "\n");
    while ((c = strtok(NULL, "\n")) != NULL) {
        if (sscanf(c, "%s %*s %*s", id) != 1) {
            return false;
        }
        nodes_in_use[atoi(id)] = 1;
    }

    for (int i = 0; i < 100; i++) {
        if (nodes_in_use[i] == 0) {
            printf("\nNEW ID: %02d", i);
            sprintf(me->self.id, "%02d", i);
            return true;
        }
    }

    printf("\nNETWORK FULL");
    return false;
}

bool valid_net(char *strnet)
{
    int net = atoi(strnet);
    if (strlen(strnet) != 3 || net < 0 || net > 999) {
        return false;
    }
    else {
        return true;
    }
}

bool valid_id(char *strid)
{
    int id = atoi(strid);
    if (strlen(strid) != 2 || id < 0 || id > 99) {
        return false;
    }
    else {
        return true;
    }
}

bool join_arguments(app_t *me)
{
    if (valid_net(me->net) && valid_id(me->self.id) && me->connected == false) {
        return true;
    }
    else {
        return false;
    }
}

bool djoin_arguments(app_t *me)
{
    if (valid_net(me->net) && valid_id(me->self.id) && valid_id(me->ext.id) && valid_ip(me->ext.ip) && valid_port(me->ext.port) && me->connected == false) {
        return true;
    }
    else {
        return false;
    }
}

bool get_arguments(post_t *post)
{
    if (valid_id(post->dest) && strcmp(post->orig, post->dest) != 0) {
        return true;
    }
    else {
        return false;
    }
}

bool node_copy(app_t *me, node_t *newnode)
{
    if (strcmp(me->ext.id, newnode->id) == 0) {
        return true;
    }
    for (int i = 0; i < me->first_free_intern; i++) {
        if (strcmp(me->intr[i].id, newnode->id) == 0) {
            return true;
        }
    }
    return false;
}

bool extern_arguments(node_t *node)
{
    if (valid_id(node->id) && valid_ip(node->ip) && valid_port(node->port)) {
        return true;
    }
    else {
        return false;
    }
}

bool content_arguments(post_t *post)
{
    if (valid_id(post->orig) && valid_id(post->dest) && strcmp(post->dest, post->orig) != 0) {
        return true;
    }
    else {
        return false;
    }
}
