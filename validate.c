#include "validate.h"

/**
 * @brief validate ip
 * 
 * @param strip ip in string
 * @return true ip is valid
 * @return false ip is invalid
 */
bool valid_ip(char strip[])
{
    int ip[4];
    // parse ip values and check format
    if (sscanf(strip, "%d.%d.%d.%d", ip, ip + 1, ip + 2, ip + 3) != 4) {
        return false;
    }
    // validate ip values
    for (int *i = ip; i < ip + 4; i++) {
        if ((*i) > 255 || (*i) < 0) {
            return false;
        }
    }
    
    return true;
}
/**
 * @brief validate port
 * 
 * @param strport port in string
 * @return true port is valid
 * @return false port is invalid
 */
bool valid_port(char strport[])
{
    int port;
    // check if port is a number
    if (sscanf(strport, "%d", &port) != 1) {
        return false;
    }
    // check is port value is valid
    if (port < 0 || port > 65535) {
        return false;
    }

    return true;
}
/**
 * @brief check arguments from command line
 * 
 * @param argc number of arguments
 * @param argv arguments
 * @return true valid arguments
 * @return false invalid arguments
 */
bool valid_command_line_arguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 3: // default server ip and port, only validate program ip and port
        if (valid_ip(argv[1]) && valid_port(argv[2])) {
            return true;
        }
        else {
            return false;
        }
        break;
    
    case 5: // validate program ip and port and server ip and port
        if (valid_ip(argv[1]) && valid_port(argv[2]) && valid_ip(argv[3]) && valid_port(argv[4])) {
            return true;
        }
        else {
            return false;
        }
        break;
    // program only accepts 3 or 5 arguments
    default:
        return false;
        break;
    }
}
/**
 * @brief check if id is already registered in the network
 * 
 * @param new_id id to register
 * @param network_info network nodes list
 * @param me app_t* struct
 * @return -1 if invalid node list | 1 if node is reapeted and have choosen new node | 0 node not reapeted
 */
int already_in_network(char new_id[], char network_info[], app_t *me)
{
    char ip[16], port[6], id[3];
    char *c;
    // discard first line "NODESLIST NET"
    c = strtok(network_info, "\n");
    
    while ((c = strtok(NULL, "\n")) != NULL) {
        // invalid format
        if (sscanf(c, "%s %s %s", id, ip, port) != 3) {
            return -1;
        }
        // same ids
        if (strcmp(id, new_id) == 0) {
            printf("\nERROR: UNAVAILABLE ID");
            // pick new node
            if (!choose_new_id(me)) {
                printf("\nERROR: PICKING NEW NODE");
                break;
            }
            return 1;
        }
    }

    return 0;
}
/**
 * @brief pick new id
 * 
 * @param me app_t* struct
 * @return true picked id succesfully
 * @return false network full
 */
bool choose_new_id(app_t *me)
{
    int nodes_in_use[100];
    // init nodes in use
    for (int i = 0; i < 100; i++) {
        nodes_in_use[i] = 0;
    }
    // get node list
    char buffer[BUFFER_SIZE];
    if (ask_for_net_nodes(buffer, me) == -1) {
        return false;
    }
    
    char *c;
    char id[3];
    // register all ids in use
    c = strtok(buffer, "\n");
    while ((c = strtok(NULL, "\n")) != NULL) {
        if (sscanf(c, "%s %*s %*s", id) != 1) {
            return false;
        }
        nodes_in_use[atoi(id)] = 1;
    }
    // pick first node not in use
    for (int i = 1; i < 100; i++) {
        if (nodes_in_use[i] == 0) {
            printf("\nNEW ID: %02d", i);
            sprintf(me->self.id, "%02d", i);
            return true;
        }
    }

    printf("\nNETWORK FULL");
    return false;
}
/**
 * @brief check if net is valid
 * 
 * @param strnet net in string
 * @return true valid net
 * @return false invalid net
 */
bool valid_net(char *strnet)
{
    // convert to number
    int net = atoi(strnet);
    // check value and format of net
    if (strlen(strnet) != 3 || net < 0 || net > 999) {
        return false;
    }
    else {
        return true;
    }
}
/**
 * @brief check if id is valid
 * 
 * @param strid id in string
 * @return true valid id
 * @return false invalid id
 */
bool valid_id(char *strid)
{
    // convert id to int
    int id = atoi(strid);
    // check value and format of id
    if (strlen(strid) != 2 || id < 0 || id > 99) {
        return false;
    }
    else {
        return true;
    }
}
/**
 * @brief validate arguments from join command
 * 
 * @param me app_t* struct
 * @return true valid arguments
 * @return false invalid arguments
 */
bool join_arguments(app_t *me)
{
    if (valid_net(me->net) && valid_id(me->self.id) && me->connected == false) {
        return true;
    }
    else {
        return false;
    }
}
/**
 * @brief valdiate djoin command arguments
 * 
 * @param me app_t* struct
 * @return true valid arguments
 * @return false invalid arguments
 */
bool djoin_arguments(app_t *me)
{
    if (valid_net(me->net) && valid_id(me->self.id) && valid_id(me->ext.id) && valid_ip(me->ext.ip) && valid_port(me->ext.port) && me->connected == false) {
        return true;
    }
    else {
        return false;
    }
}
/**
 * @brief validate get command arguments
 * 
 * @param post postal card struct
 * @return true valid arguments
 * @return false invalid arguments
 */
bool get_arguments(post_t *post)
{
    if (valid_id(post->dest) && strcmp(post->orig, post->dest) != 0) {
        return true;
    }
    else {
        return false;
    }
}
/**
 * @brief check if node already exists in my neighborhood
 * 
 * @param me app_t* struct
 * @param newnode new node
 * @return true reapeted id
 * @return false new id
 */
bool node_copy(app_t *me, node_t *newnode)
{
    // compare with extern id
    if (strcmp(me->ext.id, newnode->id) == 0) {
        return true;
    }
    // compare with interns id
    for (int i = 0; i < me->first_free_intern; i++) {
        if (strcmp(me->intr[i].id, newnode->id) == 0) {
            return true;
        }
    }
    return false;
}
/**
 * @brief validate extern commands arguments
 * 
 * @param node extern information
 * @return true valid arguments
 * @return false invalid arguments
 */
bool extern_arguments(node_t *node)
{
    if (valid_id(node->id) && valid_ip(node->ip) && valid_port(node->port)) {
        return true;
    }
    else {
        return false;
    }
}
/**
 * @brief check postal card arguments
 * 
 * @param post postal card struct
 * @return true valid arguments
 * @return false invalid arguments
 */
bool content_arguments(post_t *post)
{
    if (valid_id(post->orig) && valid_id(post->dest) && strcmp(post->dest, post->orig) != 0) {
        return true;
    }
    else {
        return false;
    }
}
