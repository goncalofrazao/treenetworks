#include "network.h"
#include "validate.h"

int ask_for_net_nodes(char buffer[], app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    
    sprintf(buffer, "NODES %s", me->net);
    if (sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    if ((bytes_read = recvfrom(fd, buffer, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    buffer[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);

    return 1;
}

int read_msg(node_t *sender)
{
    int bytes_read, out_fds;
    char auxiliar_buffer[BUFFER_SIZE];

    if ((bytes_read = read(sender->fd, auxiliar_buffer, BUFFER_SIZE - strlen(sender->buffer) - 1)) <= 0) {
        return -1;
    }
    auxiliar_buffer[bytes_read] = '\0';

    memmove(&sender->buffer[strlen(sender->buffer)], auxiliar_buffer, bytes_read + 1);

    return 1;
}

void write_msg(int fd, char msg[])
{
    int bytes_sent, bytes_to_send, bytes_writen;

    bytes_sent = 0;
    bytes_to_send = strlen(msg);
    
    while (bytes_sent < bytes_to_send) {    
        if ((bytes_writen = write(fd, &msg[bytes_sent], bytes_to_send - bytes_sent)) <= 0) {
            return;
        }
        bytes_sent += bytes_writen;
    }
}

int accept_tcp_connection(app_t *me)
{
    int bytes_read;
    char msg[BUFFER_SIZE];
    node_t newnode, ext;
    newnode.buffer[0] = '\0';

    if ((newnode.fd = accept(me->self.fd, NULL, NULL)) < 0) {
        return -1;
    }

    if (read_msg(&newnode) < 0) {
        close(newnode.fd);
        return -1;
    }
    
    if(sscanf(newnode.buffer, "NEW %s %s %s", newnode.id, newnode.ip, newnode.port) != 3) {
        close(newnode.fd);
        return -1;
    }
    newnode.buffer[0] = '\0';
    
    if (strcmp(me->ext.id, me->self.id) == 0) {
        printf("\nNEW EXTERN: %s", newnode.id);
        memmove(&me->ext, &newnode, sizeof(node_t));
        memmove(&me->bck, &me->self, sizeof(node_t));
    }
    else {
        printf("\nNEW INTERN: %s", newnode.id);
        memmove(&me->intr[me->first_free_intern++], &newnode, sizeof(node_t));
    }

    sprintf(msg, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    write_msg(newnode.fd, msg);

    return newnode.fd;
}

int open_tcp_connection(char port[])
{
    int fd;
    struct addrinfo hints, *res;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        close(fd);
        return -1;
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 5) < 0) {
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
}

void join_network(app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;
    char msg[64];

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        return;
    }
                        
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        return;
    }
    
    sprintf(msg, "REG %s %s %s %s", me->net, me->self.id, me->self.ip, me->self.port);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return;
    }

    if ((bytes_read = recvfrom(fd, msg, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    msg[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);

    if (strcmp(msg, "OKREG") == 0) {
        printf("\nJOINED NETWORK");
    }
    else {
        printf("\nERROR: JOINING NETWORK");
    }
}

void leave_network(app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;
    char msg[64];

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        return;
    }
                        
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        return;
    }
    
    sprintf(msg, "UNREG %s %s", me->net, me->self.id);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return;
    }

    if ((bytes_read = recvfrom(fd, msg, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        freeaddrinfo(res);
        return;
    }
    msg[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);

    if (strcmp(msg, "OKUNREG") == 0) {
        printf("\nLEFT NETWORK");
    }
    else {
        printf("\nERROR: LEAVING NETWORK");
    }
}

int request_to_connect_to_node(app_t *me)
{
    int fd;
    struct addrinfo hints, *res;
    char msg[64];
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(me->bck.ip, me->bck.port, &hints, &res) != 0) {
        close(fd);
        return -1;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    sprintf(msg, "NEW %s %s %s\n", me->self.id, me->self.ip, me->self.port);
    write_msg(fd, msg);
    
    freeaddrinfo(res);

    return fd;
}

void remove_intern(int i, app_t *me)
{
    me->first_free_intern--;
    if (i < me->first_free_intern) {
        memmove(&me->intr[i], &me->intr[me->first_free_intern], sizeof(node_t));
    }
}

void clear_all_file_descriptors(app_t *me, fd_set *sockets)
{
    if (me->self.fd != me->ext.fd) {
        clear_file_descriptor(me->ext.fd, sockets);
    }
    for (int i = 0; i < me->first_free_intern; i++) {
        clear_file_descriptor(me->intr[i].fd, sockets);
    }
}

void clear_file_descriptor(int fd, fd_set *sockets)
{
    FD_CLR(fd, sockets);
    close(fd);
}

void inform_all_interns(app_t *me, char msg[])
{
    for (int i = 0; i < me->first_free_intern; i++) {
        write_msg(me->intr[i].fd, msg);
    }
}

int try_to_connect_to_network(app_t *me, fd_set *current_sockets)
{
    memmove(&me->bck, &me->ext, sizeof(node_t));
    if ((me->ext.fd = request_to_connect_to_node(me)) > 0) {
        printf("\nCONNECTING TO: %s", me->ext.id);
        return 1;
    }
    else {
        memmove(&me->ext, &me->self, sizeof(node_t));
        return -1;
    }
}

int file_exists(files_t *files, char *filename)
{
    for (int i = 0; i < files->first_free_name; i++) {
        if (strcmp(filename, files->names[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int delete_file(char *filename, files_t *files)
{
    for (int i = 0; i < files->first_free_name; i++) {
        if (strcmp(filename, files->names[i]) == 0) {
            if (i < files->first_free_name - 1) {
                strcpy(files->names[i], files->names[files->first_free_name - 1]);
            }
            files->first_free_name--;
            return 1;
        }
    }
    return 0;
}

void show_names(files_t *files)
{
    for (int i = 0; i < files->first_free_name; i++) {
        printf(" --> FILE %02d : %s\n", i + 1, files->names[i]);
    }
}

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

void show_topology(app_t *me)
{
    printf(" --> ME : %s\n", me->self.id);
    printf(" --> EXTERN : %s\n", me->ext.id);
    printf(" --> BACKUP : %s\n", me->bck.id);
    for (int i = 0; i < me->first_free_intern; i++) {
        printf(" --> INTERN %02d : %s\n", i + 1, me->intr[i].id);
    }
}

void send_to_all_except_to_sender(node_t *sender, app_t *me, char msg[])
{
    if (&me->ext != sender) {
        write_msg(me->ext.fd, msg);
    }
    for (int i = 0; i < me->first_free_intern; i++) {
        if (&me->intr[i] != sender) {
            write_msg(me->intr[i].fd, msg);
        }
    }
}

void forward_message(app_t *me, post_t *post, node_t *sender, char buffer[])
{
    if (me->expedition_list[atoi(post->dest)] != NULL) {
        printf("\nFORWARDING TO: %s", me->expedition_list[atoi(post->dest)]->id);
        write_msg(me->expedition_list[atoi(post->dest)]->fd, buffer);
    }
    else {
        printf("\nFORWARDING TO ALL");
        send_to_all_except_to_sender(sender, me, buffer);
    }
}

void process_command(char msg[], node_t *sender, app_t *me, files_t *files)
{
    char buffer[BUFFER_SIZE];
    post_t post;
    post.fd = sender->fd;
    char node_to_withdraw[4];
    
    if (sscanf(msg, "EXTERN %s %s %s\n", me->bck.id, me->bck.ip, me->bck.port) == 3) {
        printf("\nNEW BACKUP: %s", me->bck.id);
    }
    else if (sscanf(msg, "WITHDRAW %s\n", node_to_withdraw) == 1) {
        me->expedition_list[atoi(node_to_withdraw)] = NULL;
        send_to_all_except_to_sender(sender, me, msg);
    }
    else if (sscanf(msg, "QUERY %s %s %s", post.dest, post.orig, post.name) == 3) {
        if (strcmp(post.dest, me->self.id) == 0) {
            if (file_exists(files, post.name)) {
                sprintf(buffer, "CONTENT %s %s %s\n", post.orig, post.dest, post.name);
            }
            else {
                sprintf(buffer, "NOCONTENT %s %s %s\n", post.orig, post.dest, post.name);
            }
            write_msg(sender->fd, buffer);
            printf("\nRETURNED CONTENT");
        }
        else{
            forward_message(me, &post, sender, msg);
        }
        me->expedition_list[atoi(post.orig)] = sender;
    }
    else if (sscanf(msg, "CONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
        if (strcmp(post.dest, me->self.id) == 0) {
            printf("\nCONTENT");
        }
        else {
            forward_message(me, &post, sender, msg);
        }
        me->expedition_list[atoi(post.orig)] = sender;
    }
    else if (sscanf(msg, "NOCONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
        if (strcmp(post.dest, me->self.id) == 0) {
            printf("\nNO CONTENT");
        }
        else {
            forward_message(me, &post, sender, msg);
        }
        me->expedition_list[atoi(post.orig)] = sender;
    }
    else {
        printf("\nWHY YOU DO THIS TO ME");
        write(sender->fd, "WHY YOU DO THIS TO ME\n", strlen("WHY YOU DO THIS TO ME\n"));
    }
}

void clear_leaver_node_from_expedition_list(node_t *leaver, app_t *me)
{
    me->expedition_list[atoi(leaver->id)] = NULL;
    for (int i = 0; i < 100; i++) {
        if (me->expedition_list[i] == leaver) {
            me->expedition_list[i] = NULL;
        }
    }
}

void show_routing(app_t *me)
{
    for (int i = 0; i < 100; i++) {
        if (me->expedition_list[i] != NULL) {
            printf("\n --> %02d - %s\n", i, me->expedition_list[i]->id);
        }
    }
}

void reset_expedition_list(app_t *me)
{
    for (int i = 0; i < 100; i++) {
        me->expedition_list[i] = NULL;
    }
}

int count_messages(char buffer[])
{
    int counter = 0;
    for (char *i = buffer; *i != '\0'; i++) {
        if (*i == '\n') {
            counter++;
        }
    }
    return counter;
}

void handle_buffer(node_t *sender, app_t *me, files_t *files)
{
    char msg[BUFFER_SIZE], *c;
    int n_of_msgs;
    n_of_msgs = count_messages(sender->buffer);
    
    for (int i = 0; i < n_of_msgs; i++) {
        c = strtok(i == 0 ? sender->buffer : NULL, "\n");
        strcpy(msg, c);
        strcpy(&msg[strlen(msg)], "\n\0");
        process_command(msg, sender, me, files);
    }
    
    if ((c = strtok(n_of_msgs == 0 ? sender->buffer : NULL, "\0")) != NULL) {
        memmove(sender->buffer, c, strlen(c) + 1);
    }
    else {
        sender->buffer[0] = '\0';
    }
}

void promote_intern(app_t *me)
{
    char buffer[BUFFER_SIZE];
    memmove(&me->ext, &me->intr[0], sizeof(node_t));
    printf("\nPROMOTED %s TO EXTERN", me->ext.id);
    sprintf(buffer, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    inform_all_interns(me, buffer);
    remove_intern(0, me);
}

int reconnect_to_backup(app_t *me)
{
    char buffer[BUFFER_SIZE];
    memmove(&me->ext, &me->bck, sizeof(node_t));

    printf("\nRECONNECT TO: %s", me->bck.id);
    if ((me->ext.fd = request_to_connect_to_node(me)) < 0) {
        printf("\nERROR: RECONNECTING");
        if (me->first_free_intern > 0) {
            promote_intern(me);
        }
        else {
            printf("\nI AM SO LONELY, PLS CALL TOMAS GLORIA TO SUCK MY DICK");
            memmove(&me->ext, &me->self, sizeof(node_t));
        }
        return -1;
    }

    sprintf(buffer, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    inform_all_interns(me, buffer);
    return me->ext.fd;
}

void clear_leaver(node_t *leaver, app_t *me, fd_set *current_sockets)
{
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "WITHDRAW %s\n", leaver->id);
    send_to_all_except_to_sender(leaver, me, buffer);
    clear_leaver_node_from_expedition_list(leaver, me);

    clear_file_descriptor(leaver->fd, current_sockets);
}

void reconnect(app_t *me, fd_set *current_sockets)
{
    if (strcmp(me->bck.id, me->self.id) != 0 && (me->ext.fd = reconnect_to_backup(me)) > 0) {
        FD_SET(me->ext.fd, current_sockets);
    }
    else if (me->first_free_intern > 0) {
        promote_intern(me);
    }
    else {
        printf("\nI AM SO LONELY, PLS CALL TOMAS GLORIA TO SUCK MY DICK");
        memmove(&me->ext, &me->self, sizeof(node_t));
    }
}

void join(app_t *me, fd_set *current_sockets)
{
    char buffer[BUFFER_SIZE];
    memmove(&me->ext, &me->self, sizeof(node_t));
    memmove(&me->bck, &me->self, sizeof(node_t));

    // ask for net nodes
    if (ask_for_net_nodes(buffer, me) < 0) {
        return;
    }

    if (already_in_network(me->self.id, buffer, me) < 0) {
        printf("\nERROR: SERVER RESPONSE");
        return;
    }

    // ask for net nodes
    if (ask_for_net_nodes(buffer, me) < 0) {
        return;
    }

    if (sscanf(buffer, "%*s %*s %s %s %s", me->ext.id, me->ext.ip, me->ext.port) == 3) {
        if (djoin(me, current_sockets) < 0) {
            return;
        }
    }

    FD_SET(me->self.fd, current_sockets);
    join_network(me);
}

int djoin(app_t *me, fd_set *current_sockets)
{
    if (strcmp(me->ext.id, me->self.id) != 0) {
        if (try_to_connect_to_network(me, current_sockets) < 0) {
            printf("\nERROR: CONNECTING");
            return -1;
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 500000;

        if (setsockopt (me->ext.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            printf("\nERROR: SETING TIMEOUT");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }

        if (read_msg(&me->ext) < 0) {
            printf("\nERROR: READING");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }

        if (sscanf(me->ext.buffer, "EXTERN %s %s %s\n", me->bck.id, me->bck.ip, me->bck.port) != 3) {
            printf("\nERROR: WRONG MESSAGE");
            clear_file_descriptor(me->ext.fd, current_sockets);
            memmove(&me->ext, &me->self, sizeof(node_t));
            return -1;
        }
        
        FD_SET(me->ext.fd, current_sockets);
        printf("\nNEW BACKUP: %s", me->bck.id);
    }
    return 1;
}
