#include "network.h"
#include "validate.h"

/**
 * @brief write in buffer net nodelist
 * 
 * @param buffer to save message from server
 * @param me app_t* struct
 * @return -1 if fail to get list | 1 if get list sucessfully
 */
int ask_for_net_nodes(char buffer[], app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;
    // open socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;
    // set addr info struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    // send request to server
    sprintf(buffer, "NODES %s", me->net);
    if (sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    // setup timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    // read response from server
    if ((bytes_read = recvfrom(fd, buffer, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    buffer[bytes_read] = '\0';
    // free memory
    close(fd);
    freeaddrinfo(res);

    return 1;
}
/**
 * @brief read from fd socket to node buffer
 * 
 * @param sender node to read
 * @return -1 if fail to read or no space in buffer | 1 if successfully read to buffer
 */
int read_msg(node_t *sender)
{
    int bytes_read;
    char auxiliar_buffer[BUFFER_SIZE];
    // read to auxiliar buffer
    if ((bytes_read = read(sender->fd, auxiliar_buffer, BUFFER_SIZE - strlen(sender->buffer) - 1)) <= 0) {
        return -1;
    }
    auxiliar_buffer[bytes_read] = '\0';
    // move to node buffer
    memmove(&sender->buffer[strlen(sender->buffer)], auxiliar_buffer, bytes_read + 1);

    return 1;
}
/**
 * @brief write msg to fd socket
 * 
 * @param fd to write in
 * @param msg to write
 */
void write_msg(int fd, char msg[])
{
    int bytes_sent, bytes_to_send, bytes_writen;

    bytes_sent = 0;
    bytes_to_send = strlen(msg);
    // write until all bytes are writen or until it fails to write
    while (bytes_sent < bytes_to_send) {
        if ((bytes_writen = write(fd, &msg[bytes_sent], bytes_to_send - bytes_sent)) <= 0) {
            return;
        }
        bytes_sent += bytes_writen;
    }
}
/**
 * @brief add new node to newnode queue
 * 
 * @param me app_t* struct
 * @param queue of nodes waiting for new
 * @param current_sockets sockets on listen
 */
void accept_tcp_connection(app_t *me, queue_t *queue, fd_set *current_sockets)
{
    // accept connection
    if ((queue->queue[queue->head].fd = accept(me->self.fd, NULL, NULL)) < 0) {
        return;
    }
    // init timer, buffer, and set fd
    queue->queue[queue->head].timer = clock();
    queue->queue[queue->head].buffer[0] = '\0';
    FD_SET(queue->queue[queue->head].fd, current_sockets);
    queue->head++;
}
/**
 * @brief read new node info and add it to app_t struct
 * 
 * @param me app_t* struct
 * @param newnode to process new
 * @param msg received from node
 * @return -1 if invalid message | > 0 if added node succesfully
 */
int command_new(app_t *me, node_t *newnode, char msg[])
{
    newnode->buffer[0] = '\0';
    // parse information
    if(sscanf(msg, "NEW %s %s %s", newnode->id, newnode->ip, newnode->port) != 3) {
        return -1;
    }
    // validate information
    if (node_copy(me, newnode) || !extern_arguments(newnode)) {
        return -1;
    }
    // add node to app_t struct
    if (strcmp(me->ext.id, me->self.id) == 0) {
        printf("\nNEW EXTERN: %s", newnode->id);
        memmove(&me->ext, newnode, sizeof(node_t));
        memmove(&me->bck, &me->self, sizeof(node_t));
    }
    else {
        printf("\nNEW INTERN: %s", newnode->id);
        memmove(&me->intr[me->first_free_intern++], newnode, sizeof(node_t));
    }
    // reply confirmation of join
    sprintf(msg, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    write_msg(newnode->fd, msg);
    return newnode->fd;
}
/**
 * @brief open server tcp
 * 
 * @param port to open server
 * @return -1 if cannot open server | fd where server is opened
 */
int open_tcp_connection(char port[])
{
    int fd;
    struct addrinfo hints, *res;
    // open socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    // set addr info struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    // bind fd to port
    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        close(fd);
        return -1;
    }
    // listen
    if (listen(fd, 5) < 0) {
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
}
/**
 * @brief send reg message to server
 * 
 * @param me app_t* struct
 */
void join_network(app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;
    char msg[64];
    // open socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        return;
    }
    // set addr info struct      
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        return;
    }
    // request to reg
    sprintf(msg, "REG %s %s %s %s", me->net, me->self.id, me->self.ip, me->self.port);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    // set timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return;
    }
    // read confirmation of reg
    if ((bytes_read = recvfrom(fd, msg, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    msg[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);
    // check reg
    if (strcmp(msg, "OKREG") == 0) {
        printf("\nJOINED NETWORK");
    }
    else {
        printf("\nERROR: JOINING NETWORK");
    }
}
/**
 * @brief send unreg message to server
 * 
 * @param me app_t* struct
 */
void leave_network(app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;
    char msg[64];
    // open socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        return;
    }
    // set addir info struct      
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        return;
    }
    // send unreg request
    sprintf(msg, "UNREG %s %s", me->net, me->self.id);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    // set timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return;
    }
    // read confimation of unreg
    if ((bytes_read = recvfrom(fd, msg, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    msg[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);
    // check unreg
    if (strcmp(msg, "OKUNREG") == 0) {
        printf("\nLEFT NETWORK");
    }
    else {
        printf("\nERROR: LEAVING NETWORK");
    }
}
/**
 * @brief try to connect to backup and send NEW message
 * 
 * @param me app_t* struct
 * @return -1 if cant connect | fd of the socket of the connection to server node
 */
int request_to_connect_to_node(app_t *me)
{
    int fd;
    struct addrinfo hints, *res;
    char msg[64];
    // open socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    // set addr info struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(me->bck.ip, me->bck.port, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    // connect to server node
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    // send NEW message
    sprintf(msg, "NEW %s %s %s\n", me->self.id, me->self.ip, me->self.port);
    write_msg(fd, msg);
    
    freeaddrinfo(res);

    return fd;
}
/**
 * @brief remove intern from intern list
 * 
 * @param i position of the intern to remove
 * @param me app_t* struct
 */
void remove_intern(int i, app_t *me)
{
    // remove intern
    me->first_free_intern--;
    if (i < me->first_free_intern) {
        memmove(&me->intr[i], &me->intr[me->first_free_intern], sizeof(node_t));
    }
}
/**
 * @brief close and clear all fd's
 * 
 * @param me app_t* struct
 * @param sockets current listening sockets to clear
 */
void clear_all_file_descriptors(app_t *me, fd_set *sockets)
{
    // clear extern if node is not alone
    if (me->self.fd != me->ext.fd) {
        clear_file_descriptor(me->ext.fd, sockets);
    }
    // clear all interns
    for (int i = 0; i < me->first_free_intern; i++) {
        clear_file_descriptor(me->intr[i].fd, sockets);
    }
}
/**
 * @brief close and clear from fd_set struct fd
 * 
 * @param fd socket to close and clear
 * @param sockets fd_set struct to clear
 */
void clear_file_descriptor(int fd, fd_set *sockets)
{
    FD_CLR(fd, sockets); // clear
    close(fd); // close
}
/**
 * @brief connect to extern
 * 
 * @param me app_t* struct
 * @return -1 if fail to connect | 1 if connect succesfully
 */
int try_to_connect_to_network(app_t *me)
{
    // move extern to backup
    memmove(&me->bck, &me->ext, sizeof(node_t));
    // connect to backup
    if ((me->ext.fd = request_to_connect_to_node(me)) > 0) {
        printf("\nCONNECTING TO: %s", me->ext.id);
        return 1;
    }
    else {
        memmove(&me->ext, &me->self, sizeof(node_t));
        return -1;
    }
}
/**
 * @brief check if filename exists
 * 
 * @param files struct of files
 * @param filename filename to check
 * @return 1 if exists | 0 if does not exist
 */
int file_exists(files_t *files, char *filename)
{
    // check if filename exists
    for (int i = 0; i < files->first_free_name; i++) {
        if (strcmp(filename, files->names[i]) == 0) {
            return 1;
        }
    }
    return 0;
}
/**
 * @brief deletes a file if it exists
 * 
 * @param filename to delte
 * @param files struct of files
 * @return 1 if file deletd | 0 if there is not the file
 */
int delete_file(char *filename, files_t *files)
{
    for (int i = 0; i < files->first_free_name; i++) {
        if (strcmp(filename, files->names[i]) == 0) {
            // delete file
            if (i < files->first_free_name - 1) {
                strcpy(files->names[i], files->names[files->first_free_name - 1]);
            }
            files->first_free_name--;
            return 1;
        }
    }
    return 0;
}
/**
 * @brief show all filenames
 * 
 * @param files struct of files
 */
void show_names(files_t *files)
{
    for (int i = 0; i < files->first_free_name; i++) {
        printf(" --> FILE %02d : %s\n", i + 1, files->names[i]);
    }
}
/**
 * @brief create a filename
 * 
 * @param filename file to create
 * @param files struct of files
 * @return 1 if could create | 0 if file struct is full
 */
int create_file(char *filename, files_t *files)
{
    if (files->first_free_name < 100) {
        strcpy(files->names[files->first_free_name++], filename);
        return 1;
    }
    else {
        return 0;
    }
}
/**
 * @brief show topology
 * 
 * @param me app_t* struct
 */
void show_topology(app_t *me)
{
    // print my name, my extern name, my backup name and all my interns name if I am in network
    if (me->connected) {
        printf(" --> ME : %s\n", me->self.id);
        printf(" --> EXTERN : %s\n", me->ext.id);
        printf(" --> BACKUP : %s\n", me->bck.id);
        for (int i = 0; i < me->first_free_intern; i++) {
            printf(" --> INTERN %02d : %s\n", i + 1, me->intr[i].id);
        }
    }
}
/**
 * @brief send a msg to all nodes except for sender node
 * 
 * @param sender node
 * @param me app_t* struct
 * @param msg to send
 */
void send_to_all_except_to_sender(node_t *sender, app_t *me, char msg[])
{
    // send to ext if not the sender
    if (&me->ext != sender) {
        write_msg(me->ext.fd, msg);
    }
    // send to all interns that are not the sender
    for (int i = 0; i < me->first_free_intern; i++) {
        if (&me->intr[i] != sender) {
            write_msg(me->intr[i].fd, msg);
        }
    }
}
/**
 * @brief send msg to next if dest is in the expedition list, else send to all
 * 
 * @param me app_t* struct
 * @param post post header of msg
 * @param sender node
 * @param buffer to send
 */
void forward_message(app_t *me, post_t *post, node_t *sender, char buffer[])
{
    // send to next of expedition list
    if (me->expedition_list[atoi(post->dest)] != NULL) {
        printf("\nFORWARDING TO: %s", me->expedition_list[atoi(post->dest)]->id);
        write_msg(me->expedition_list[atoi(post->dest)]->fd, buffer);
    }
    // send to all
    else {
        printf("\nFORWARDING TO ALL");
        send_to_all_except_to_sender(sender, me, buffer);
    }
}
/**
 * @brief read and parse a msg received
 * 
 * @param msg received
 * @param sender of msg
 * @param me app_t* struct
 * @param files struct of files
 */
void process_command(char msg[], node_t *sender, app_t *me, files_t *files)
{
    char buffer[BUFFER_SIZE];
    post_t post;
    post.fd = sender->fd;
    char node_to_withdraw[4];
    node_t node;
    // EXTERN msg
    if (sscanf(msg, "EXTERN %s %s %s\n", node.id, node.ip, node.port) == 3) {
        // validate arguments
        if (!extern_arguments(&node)) {
            printf("\nERROR: INVALID EXTERN");
            return;
        }
        // parse and save information
        sscanf(msg, "EXTERN %s %s %s\n", me->bck.id, me->bck.ip, me->bck.port);
        printf("\nNEW BACKUP: %s", me->bck.id);
    }
    // WITHDRAW msg
    else if (sscanf(msg, "WITHDRAW %s\n", node_to_withdraw) == 1) {
        // validate node to withdraw
        if (!valid_id(node_to_withdraw)) {
            printf("\nERROR: INVALID WITHDRAW");
            return;
        }
        // clear expedition list position
        me->expedition_list[atoi(node_to_withdraw)] = NULL;
        // difuse msg through network
        send_to_all_except_to_sender(sender, me, msg);
    }
    // QUERY msg
    else if (sscanf(msg, "QUERY %s %s %s", post.dest, post.orig, post.name) == 3) {
        // validate post
        if (!content_arguments(&post)) {
            printf("\nERROR: INVALID QUERY");
            return;
        }
        // return answer if I am the destination
        if (strcmp(post.dest, me->self.id) == 0) {
            // check if content asked exists
            if (file_exists(files, post.name)) {
                sprintf(buffer, "CONTENT %s %s %s\n", post.orig, post.dest, post.name);
            }
            else {
                sprintf(buffer, "NOCONTENT %s %s %s\n", post.orig, post.dest, post.name);
            }
            // send answer
            write_msg(sender->fd, buffer);
            printf("\nRETURNED CONTENT");
        }
        // forward msg to destine
        else{
            forward_message(me, &post, sender, msg);
        }
        // update expedition list
        me->expedition_list[atoi(post.orig)] = sender;
    }
    // CONTENT msg
    else if (sscanf(msg, "CONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
        // validate post
        if (!content_arguments(&post)) {
            printf("\nERROR: INVALID CONTENT");
            return;
        }
        // If I am the destine
        if (strcmp(post.dest, me->self.id) == 0) {
            // add filename to struct of files if not reapeted
            if (file_exists(files, post.name) == 0) {
                create_file(post.name, files);
            }
            printf("\nCONTENT");
        }
        // forward msg if I am not the destine
        else {
            forward_message(me, &post, sender, msg);
        }
        // update expedition list
        me->expedition_list[atoi(post.orig)] = sender;
    }
    // NOCONTENT msg
    else if (sscanf(msg, "NOCONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
        // validate post
        if (!content_arguments(&post)) {
            printf("\nERROR: INVALID NOCONTENT");
            return;
        }
        // check if I am the destine
        if (strcmp(post.dest, me->self.id) == 0) {
            printf("\nNO CONTENT");
        }
        // forward msg if I am not the destine
        else {
            forward_message(me, &post, sender, msg);
        }
        // update expedition list
        me->expedition_list[atoi(post.orig)] = sender;
    }
    // UNKNOWN MESSAGE
    else {
        printf("\nWHY YOU DO THIS TO ME");
        write(sender->fd, "WHY YOU DO THIS TO ME\n", strlen("WHY YOU DO THIS TO ME\n"));
    }
}
/**
 * @brief delete all entries to leaver node from expedition list
 * 
 * @param leaver node
 * @param me app_t* struct
 */
void clear_leaver_node_from_expedition_list(node_t *leaver, app_t *me)
{
    me->expedition_list[atoi(leaver->id)] = NULL;
    for (int i = 0; i < 100; i++) {
        if (me->expedition_list[i] == leaver) {
            me->expedition_list[i] = NULL;
        }
    }
}
/**
 * @brief show routing
 * 
 * @param me app_t* struct
 */
void show_routing(app_t *me)
{
    for (int i = 0; i < 100; i++) {
        // print if expedition list position is not empty
        if (me->expedition_list[i] != NULL) {
            printf("\n --> %02d - %s\n", i, me->expedition_list[i]->id);
        }
    }
}
/**
 * @brief reset expedition list
 * 
 * @param me app_t* struct
 */
void reset_expedition_list(app_t *me)
{
    // reset all entries from expedition list
    for (int i = 0; i < 100; i++) {
        me->expedition_list[i] = NULL;
    }
}
/**
 * @brief count number of completed messages in a buffer
 * 
 * @param buffer to count
 * @return number of messages
 */
int count_messages(char buffer[])
{
    int counter = 0;
    for (char *i = buffer; *i != '\0'; i++) {
        // messages finish in \n
        if (*i == '\n') {
            counter++;
        }
    }
    return counter;
}
/**
 * @brief process all messages from a node buffer
 * 
 * @param sender node
 * @param me app_t* struct
 * @param files struct of files
 */
void handle_buffer(node_t *sender, app_t *me, files_t *files)
{
    char msg[BUFFER_SIZE], *c;
    int n_of_msgs;
    // count messages
    n_of_msgs = count_messages(sender->buffer);
    // process all messages
    for (int i = 0; i < n_of_msgs; i++) {
        c = strtok(i == 0 ? sender->buffer : NULL, "\n");
        strcpy(msg, c);
        strcpy(&msg[strlen(msg)], "\n\0");
        process_command(msg, sender, me, files);
    }
    // if there is uncompleted message, push it to the beggining of the buffer
    if ((c = strtok(n_of_msgs == 0 ? sender->buffer : NULL, "\0")) != NULL) {
        memmove(sender->buffer, c, strlen(c) + 1);
    }
    else {
        sender->buffer[0] = '\0';
    }
}
/**
 * @brief promote a intern to extern
 * 
 * @param me app_t* struct
 */
void promote_intern(app_t *me)
{
    char buffer[BUFFER_SIZE];
    // move it to extern position
    memmove(&me->ext, &me->intr[0], sizeof(node_t));
    printf("\nPROMOTED %s TO EXTERN", me->ext.id);
    // remove my new extern from interns list
    remove_intern(0, me);
    // update its and all my interns new backup
    sprintf(buffer, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    send_to_all_except_to_sender(NULL, me, buffer);
}
/**
 * @brief process a bad connection to backup
 * 
 * @param me app_t* struct
 */
void handle_bad_reconnect(app_t *me)
{
    // promote intern
    if (me->first_free_intern > 0) {
        promote_intern(me);
    }
    // reset extern and backup
    else {
        printf("\nALONE IN THE NETWORK");
        memmove(&me->ext, &me->self, sizeof(node_t));
        memmove(&me->bck, &me->self, sizeof(node_t));
    }
}
/**
 * @brief reconnect to backup
 * 
 * @param me app_t* struct
 * @param current_sockets current file descriptors activated
 * @return -1 if fail to connect to backup | fd of the socket of the connection to new extern
 */
int reconnect_to_backup(app_t *me, fd_set *current_sockets)
{
    char buffer[BUFFER_SIZE];
    memmove(&me->ext, &me->bck, sizeof(node_t));
    // request connection and send NEW msg
    printf("\nRECONNECT TO: %s", me->bck.id);
    if ((me->ext.fd = request_to_connect_to_node(me)) < 0) {
        printf("\nERROR: RECONNECTING");
        return -1;
    }
    // set timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (me->ext.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("\nERROR: SETING TIMEOUT");
        clear_file_descriptor(me->ext.fd, current_sockets);
        return -1;
    }
    // read response
    if (read_msg(&me->ext) < 0) {
        printf("\nERROR: READING");
        clear_file_descriptor(me->ext.fd, current_sockets);
        return -1;
    }
    // parse msg and update backup
    if (sscanf(me->ext.buffer, "EXTERN %s %s %s\n", me->bck.id, me->bck.ip, me->bck.port) != 3) {
        printf("\nERROR: WRONG MESSAGE");
        clear_file_descriptor(me->ext.fd, current_sockets);
        return -1;
    }

    me->ext.buffer[0] = '\0';
    printf("\nNEW BACKUP: %s", me->bck.id);
    // udpate interns new backup
    sprintf(buffer, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    send_to_all_except_to_sender(&me->ext, me, buffer);
    return me->ext.fd;
}
/**
 * @brief send WITHDRAW msg to all other nodes and clear leaver fd and from expedition list
 * 
 * @param leaver node
 * @param me app_t* struct
 * @param current_sockets current file descriptors activated
 */
void clear_leaver(node_t *leaver, app_t *me, fd_set *current_sockets)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "WITHDRAW %s\n", leaver->id);
    send_to_all_except_to_sender(leaver, me, buffer);
    clear_leaver_node_from_expedition_list(leaver, me);

    clear_file_descriptor(leaver->fd, current_sockets);
}
/**
 * @brief handle reconnect process
 * 
 * @param me app_t* struct
 * @param current_sockets current file descriptors activated
 */
void reconnect(app_t *me, fd_set *current_sockets)
{
    // try to reconnect
    if (strcmp(me->bck.id, me->self.id) != 0 && (me->ext.fd = reconnect_to_backup(me, current_sockets)) > 0) {
        FD_SET(me->ext.fd, current_sockets);
    }
    // handle fail to reconect
    else {
        handle_bad_reconnect(me);
    }
}
/**
 * @brief join network updating nodes list
 * 
 * @param me app_t* struct
 * @param current_sockets current file descriptors activated
 */
void join(app_t *me, fd_set *current_sockets)
{
    char buffer[BUFFER_SIZE];
    memmove(&me->ext, &me->self, sizeof(node_t));
    memmove(&me->bck, &me->self, sizeof(node_t));

    // ask for net nodes
    if (ask_for_net_nodes(buffer, me) < 0) {
        return;
    }
    // validate id or pick new one
    if (already_in_network(me->self.id, buffer, me) < 0) {
        printf("\nERROR: SERVER RESPONSE");
        return;
    }

    // ask for net nodes
    if (ask_for_net_nodes(buffer, me) < 0) {
        return;
    }
    // pick one node if exists
    if (sscanf(buffer, "%*s %*s %s %s %s", me->ext.id, me->ext.ip, me->ext.port) == 3) {
        if (djoin(me, current_sockets) < 0) {
            return;
        }
    }
    // start listening new connections
    FD_SET(me->self.fd, current_sockets);
    // update nodes list
    join_network(me);
}
/**
 * @brief connect to extern
 * 
 * @param me app_t* struct
 * @param current_sockets current file descriptors activated
 * @return -1 if fail to connect | 1 if connect succesfully
 */
int djoin(app_t *me, fd_set *current_sockets)
{
    // connet if it is not supposed to connect to myself
    if (strcmp(me->ext.id, me->self.id) != 0) {
        // connect
        if (try_to_connect_to_network(me) < 0) {
            printf("\nERROR: CONNECTING");
            return -1;
        }
        // set timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 500000;

        if (setsockopt (me->ext.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            printf("\nERROR: SETING TIMEOUT");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }
        // read response
        if (read_msg(&me->ext) < 0) {
            printf("\nERROR: READING");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }
        // parse extern msg
        if (sscanf(me->ext.buffer, "EXTERN %s %s %s\n", me->bck.id, me->bck.ip, me->bck.port) != 3) {
            printf("\nERROR: WRONG MESSAGE");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }

        me->ext.buffer[0] = '\0';
        FD_SET(me->ext.fd, current_sockets);
        printf("\nNEW BACKUP: %s", me->bck.id);
    }
    me->connected = true;
    return 1;
}
/**
 * @brief calculate time science connection is connected
 * 
 * @param i position of connection to calculate time
 * @param queue queue of connections
 * @return time in miliseconds
 */
int calculate_time(int i, queue_t *queue)
{
    clock_t diff = clock() - queue->queue[i].timer;
    return diff * 1000 / CLOCKS_PER_SEC;
}
/**
 * @brief remove connection from the queue
 * 
 * @param i position of the connection to remove
 * @param queue queue of connections
 * @param current_sockets current file descriptors activated
 * @param delete 1 if it is supposed to clear fd, else 0
 */
void remove_connection_from_queue(int i, queue_t *queue, fd_set *current_sockets, int delete)
{
    // clear node fd
    if (delete) {
        clear_file_descriptor(queue->queue[i].fd, current_sockets);
    }
    // remove node
    queue->head--;
    if (i < queue->head) {
        memmove(&queue->queue[i], &queue->queue[queue->head], sizeof(node_t));
    }
}
/**
 * @brief promote connection to node
 * 
 * @param me app_t* struct
 * @param queue queue of connections
 * @param i position of the connection to promote
 * @param current_sockets current file descriptors activated
 */
void promote_from_queue(app_t *me, queue_t *queue, int i, fd_set *current_sockets)
{
    char msg[BUFFER_SIZE], *c;
    int n_of_msgs, delete;
    n_of_msgs = count_messages(queue->queue[i].buffer);
    
    // read first message
    if (n_of_msgs > 0) {
        c = strtok(queue->queue[i].buffer, "\n");
        strcpy(msg, c);
        strcpy(&msg[strlen(msg)], "\n\0");
        // process NEW command
        delete = command_new(me, &queue->queue[i], msg) < 0 ? DELETE : NOT_DELETE;
        // remove connection from queue
        remove_connection_from_queue(i, queue, current_sockets, delete);
    }    
}
/**
 * @brief set file descriptor if it is not set
 * 
 * @param fd to set
 * @param current_sockets current file descriptors activated
 */
void reset_fd(int fd, fd_set *current_sockets)
{
    if (!FD_ISSET(fd, current_sockets)) {
        FD_SET(fd, current_sockets);
    }
}