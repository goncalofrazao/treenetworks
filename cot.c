#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

typedef struct node_t {
    char id[3];
    char ip[16];
    char port[6];
    int fd;
} node_t;

typedef struct app_t {
    node_t self;
    node_t ext;
    node_t bck;
    node_t intr[100];
    int first_free_intern;
} app_t;

typedef struct post_t {
    char dest[3];
    char orig[3];
    char name[100];
    int fd;
} post_t;

typedef struct files_t {
    char names[100][100];
    int first_free_name;
} files_t;

int ask_for_net_nodes(char buffer[], char net[], char regIP[], char regUDP[]);
void join_network(char net[], app_t *me, char regIP[], char regUDP[]);
int request_to_connect_to_node(app_t *me);
int open_tcp_connection(char port[]);
int accept_tcp_connection(int server_fd, app_t *me);
void leave_network(char net[], app_t *me, char regIP[], char regUDP[]);
void clear_file_descriptor(int fd, fd_set *sockets);
void remove_intern(int i, app_t *me);
void clear_all_file_descriptors(app_t *me, fd_set *sockets);
void inform_all_interns(app_t *me, fd_set *current_sockets, char msg[]);
void try_to_connect_to_network(app_t *me, fd_set *current_sockets);
void show_topology(app_t *me);
int create_file(char *filename, files_t *files);
void show_names(files_t *files);
int delete_file(char *filename, files_t *files);
int file_exists(files_t *files, char *filename);
void send_to_all_except_to_sender(int sender, app_t *me, char buffer[]);
void forward_message(app_t *me, post_t *post, files_t *files, int expedition_list[], char buffer[]);
void process_command(char buffer[], node_t sender, app_t *me, files_t *files, int expedition_list[]);
void clear_leaver_node_from_expedition_list(node_t leaver, int expedition_list[]);
node_t get_id(int fd, app_t *me);
void show_routing(int expedition_list[], app_t *me);
void reset_expedition_list(int expedition_list[]);
void handle_buffer(char buffer[], fd_set *current_sockets, node_t sender, app_t *me, files_t *files, int expedition_list[]);

int main(int argc, char *argv[])
{
    if (argc != 5) exit(1);
    setbuf(stdout, NULL);

    fd_set ready_sockets, current_sockets;
    
    FD_ZERO(&current_sockets);
    FD_SET(0, &current_sockets);

    int out_fds, newfd, len;
    char buffer[BUFFER_SIZE], net[4], id[3], msg[BUFFER_SIZE];
    
    char *c, token[] = " \n\t";
    char *regIP = argv[3], *regUDP = argv[4];

    app_t me;
    strcpy(me.self.ip, argv[1]);
    strcpy(me.self.port, argv[2]);
    me.first_free_intern = 0;

    files_t files;
    files.first_free_name = 0;

    int expedition_list[100];
    for (int i = 0; i < 100; i++) {
        expedition_list[i] = -1;
    }
    post_t post;

    if((me.self.fd = open_tcp_connection(me.self.port)) < 0) {
        printf("could not open a tcp connection\n");
        exit(1);
    }
    FD_SET(me.self.fd, &current_sockets);

    int bytes_read;
    
    while (1)
    {
        ready_sockets = current_sockets;
        printf("\nCOMMAND: ");
        out_fds = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        switch (out_fds)
        {
        case -1:
            perror("select");
            exit(1);
            break;
        
        default:
            // check for console input
            if (FD_ISSET(0, &ready_sockets)) {
                // read input
                fgets(buffer, BUFFER_SIZE, stdin);
                
                // get command
                c = strtok(buffer, token);
                if (c == NULL) {
                    break;
                }
                // handle join
                if (strcmp(c, "join") == 0) {
                    
                    // join arguments
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(net, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.self.id, c);

                    memmove(&me.ext, &me.self, sizeof(node_t));
                    memmove(&me.bck, &me.self, sizeof(node_t));

                    // ask for net nodes
                    if (ask_for_net_nodes(buffer, net, regIP, regUDP) < 0) {
                        break;
                    }

                    if (sscanf(buffer, "%*s %*s %s %s %s", me.ext.id, me.ext.ip, me.ext.port) == 3) {
                        try_to_connect_to_network(&me, &current_sockets);
                    }
                    else {
                        join_network(net, &me, regIP, regUDP);
                    }
                }
                else if (strcmp(c, "djoin") == 0) {
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(net, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.self.id, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.ext.id, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.ext.ip, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.ext.port, c);

                    if (strcmp(me.ext.id, me.self.id) != 0) {
                        try_to_connect_to_network(&me, &current_sockets);
                    }

                }
                else if (strcmp(c, "leave") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets);
                    me.first_free_intern = 0;
                    leave_network(net, &me, regIP, regUDP);
                    reset_expedition_list(expedition_list);
                }
                else if (strcmp(c, "st") == 0) {
                    show_topology(&me);
                }
                else if (strcmp(c, "create") == 0) {
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    if (create_file(c, &files)) {
                        printf("\nNEW FILE: %s", c);
                    }
                    else {
                        printf("\nNOT POSSIBLE TO CREATE %s", c);
                    }
                }
                else if (strcmp(c, "sn") == 0) {
                    show_names(&files);
                }
                else if (strcmp(c, "delete") == 0) {
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    if (delete_file(c, &files)) {
                        printf("\nDELETED FILE: %s", c);
                    }
                    else {
                        printf("\nNOT POSSIBLE TO DELETE %s", c);
                    }
                }
                else if (strcmp(c, "get") == 0) {
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(post.dest, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(post.name, c);
                    sprintf(buffer, "QUERY %s %s %s\n", post.dest, me.self.id, post.name);
                    post.fd = expedition_list[atoi(post.dest)];
                    forward_message(&me, &post, &files, expedition_list, buffer);
                }
                else if (strcmp(c, "sr") == 0) {
                    show_routing(expedition_list, &me);
                }
                else if (strcmp(c, "exit") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets);
                    leave_network(net, &me, regIP, regUDP);
                    putchar(10);
                    exit(0);
                }
            }
            if (FD_ISSET(me.self.fd, &ready_sockets)) {
                if ((newfd = accept_tcp_connection(me.self.fd, &me)) > 0) {
                    FD_SET(newfd, &current_sockets);
                    printf("\nNEW INTERN: %s", me.intr[me.first_free_intern - 1].id);
                }
                else{
                    printf("\nCOULD NOT ACCEPT NEW NODE");
                }
            }
            if (FD_ISSET(me.ext.fd, &ready_sockets)) {
                if ((bytes_read = read(me.ext.fd, buffer, BUFFER_SIZE)) <= 0) {

                    sprintf(buffer, "WITHDRAW %s\n", me.ext.id);
                    send_to_all_except_to_sender(me.ext.fd, &me, buffer);
                    clear_leaver_node_from_expedition_list(me.ext, expedition_list);

                    clear_file_descriptor(me.ext.fd, &current_sockets);
                    printf("\nEXTERN LEFT: %s", me.ext.id);

                    if (strcmp(me.bck.id, me.self.id) != 0) {
                        memmove(&me.ext, &me.bck, sizeof(node_t));
                        leave_network(net, &me, regIP, regUDP);

                        printf("\nRECONNECT TO: %s", me.bck.id);
                        if ((me.ext.fd = request_to_connect_to_node(&me)) < 0) {
                            printf("\nFAILED TO RECONNECT");
                            break;
                        }
                        FD_SET(me.ext.fd, &current_sockets);
                        
                    }
                    else if (me.first_free_intern > 0) {
                        memmove(&me.ext, &me.intr[0], sizeof(node_t));
                        printf("\nPROMOTED %s TO EXTERN", me.ext.id);
                        sprintf(buffer, "EXTERN %s %s %s\n", me.ext.id, me.ext.ip, me.ext.port);
                        inform_all_interns(&me, &current_sockets, buffer);
                        remove_intern(0, &me);
                    }
                    else {
                        printf("\nI AM SO LONELY, CALL TOMAS GLORIA PLEASE TO SUCK MY DICK");
                        memmove(&me.ext, &me.self, sizeof(node_t));
                    }
                }
                else {
                    buffer[bytes_read] = '\0';
                    handle_buffer(buffer, &current_sockets, me.ext, &me, &files, expedition_list);
                }
            }
            for (int i = 0; i < me.first_free_intern; i++) {
                if (FD_ISSET(me.intr[i].fd, &ready_sockets)) {
                    if ((bytes_read = read(me.intr[i].fd, buffer, BUFFER_SIZE)) <= 0) {

                        sprintf(buffer, "WITHDRAW %s\n", me.intr[i].id);
                        send_to_all_except_to_sender(me.intr[i].fd, &me, buffer);
                        clear_leaver_node_from_expedition_list(me.intr[i], expedition_list);
                        
                        clear_file_descriptor(me.intr[i].fd, &current_sockets);
                        remove_intern(i, &me);
                        
                        printf("\nINTERN LEFT: %s", me.intr[i].id);
                    }
                    else {
                        buffer[bytes_read] = '\0';
                        handle_buffer(buffer, &current_sockets, me.intr[i], &me, &files, expedition_list);
                    }
                }
            }
            
            break;
        }
    }
    
    return 0;
}

void handle_buffer(char buffer[], fd_set *current_sockets, node_t sender, app_t *me, files_t *files, int expedition_list[])
{
    char msg[BUFFER_SIZE], *c;
    int len;
    c = strtok(buffer, "\n");
    do {
        strcpy(msg, c);
        len = strlen(msg);
        msg[len] = '\n';
        msg[len + 1] = '\0';
        if (sscanf(msg, "EXTERN %s %s %s", me->bck.id, me->bck.ip, me->bck.port) == 3) {
            printf("\nUPDATED BACKUP TO: %s", me->bck.id);
            sprintf(msg, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
            inform_all_interns(me, current_sockets, msg);
        }
        else {
            process_command(msg, sender, me, files, expedition_list);
        }    
    } while ((c = strtok(NULL, "\n")) != NULL);
}

void reset_expedition_list(int expedition_list[])
{
    for (int i = 0; i < 100; i++) {
        expedition_list[i] = -1;
    }
}

void show_routing(int expedition_list[], app_t *me)
{
    printf("\n\n\t ROUTES \n\n");
    for (int i = 0; i < 100; i++) {
        if (expedition_list[i] != -1) {
            printf("\n --> %02d - %s\n", i, get_id(expedition_list[i], me).id);
        }
    }
    printf("\n\n");
}

node_t get_id(int fd, app_t *me)
{
    node_t empty_node;
    empty_node.fd = -1;
    if (me->ext.fd == fd) {
        return me->ext;
    }
    for (int i = 0; i < me->first_free_intern; i++) {
        if (me->intr[i].fd == fd) {
            return me->intr[i];
        }
    }
    return empty_node;
}

void clear_leaver_node_from_expedition_list(node_t leaver, int expedition_list[])
{
    expedition_list[atoi(leaver.id)] = -1;
    for (int i = 0; i < 100; i++) {
        if (expedition_list[i] == leaver.fd) {
            expedition_list[i] = -1;
        }
    }
}

node_t get_node(char id[], app_t *me)
{
    node_t empty_node;
    empty_node.fd = -1;
    if (strcmp(id, me->ext.id) == 0) {
        return me->ext;
    }
    else {
        for (int i = 0; i < me->first_free_intern; i++) {
            if (strcmp(id, me->intr[i].id) == 0) {
                return me->intr[i];
            }
        }
    }
    return empty_node;
}

void process_command(char buffer[], node_t sender, app_t *me, files_t *files, int expedition_list[])
{
    post_t post;
    post.fd = sender.fd;
    char node_to_withdraw[4];
    if (sscanf(buffer, "WITHDRAW %s\n", node_to_withdraw) == 1) {
        expedition_list[atoi(node_to_withdraw)] = -1;
        send_to_all_except_to_sender(sender.fd, me, buffer);
    }
    else {
        if (sscanf(buffer, "QUERY %s %s %s", post.dest, post.orig, post.name) == 3) {
            if (strcmp(post.dest, me->self.id) == 0) {
                if (file_exists(files, post.name)) {
                    sprintf(buffer, "CONTENT %s %s %s\n", post.orig, post.dest, post.name);
                }
                else {
                    sprintf(buffer, "NOCONTENT %s %s %s\n", post.orig, post.dest, post.name);
                }
                write(post.fd, buffer, strlen(buffer));
                printf("\nRETURNED CONTENT");
            }
            else{
                forward_message(me, &post, files, expedition_list, buffer);
            }
            expedition_list[atoi(post.orig)] = post.fd;
        }
        else if (sscanf(buffer, "CONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
            if (strcmp(post.dest, me->self.id) == 0) {
                printf("\nFOUND CONTENT");
            }
            else {
                forward_message(me, &post, files, expedition_list, buffer);
            }
            expedition_list[atoi(post.orig)] = post.fd;
        }
        else if (sscanf(buffer, "NOCONTENT %s %s %s", post.dest, post.orig, post.name) == 3) {
            if (strcmp(post.dest, me->self.id) == 0) {
                printf("\nNO CONTENT");
            }
            else {
                forward_message(me, &post, files, expedition_list, buffer);
            }
            expedition_list[atoi(post.orig)] = post.fd;
        }
    }
}

void forward_message(app_t *me, post_t *post, files_t *files, int expedition_list[], char buffer[])
{
    if (expedition_list[atoi(post->dest)] != -1) {
        printf("\nFORWARDING MESSAGE TO: %02d", expedition_list[atoi(post->dest)]);
        write(expedition_list[atoi(post->dest)], buffer, strlen(buffer));
    }
    else {
        printf("\nFORWARDING MESSAGE TO ALL");
        send_to_all_except_to_sender(post->fd, me, buffer);
    }
}

void send_to_all_except_to_sender(int sender, app_t *me, char buffer[])
{
    if (me->ext.fd != sender) {
        write(me->ext.fd, buffer, strlen(buffer));
    }
    for (int i = 0; i < me->first_free_intern; i++) {
        if (me->intr[i].fd != sender) {
            write(me->intr[i].fd, buffer, strlen(buffer));
        }
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
    printf("\n\n\t NAMES \n\n");
    for (int i = 0; i < files->first_free_name; i++) {
        printf(" --> FILE %02d : %s\t \n\n", i, files->names[i]);
    }
    printf("\n\n");
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
    printf("\n\n\t TOPOLOGY \n\n");
    printf(" --> EXTERN : \n\t\tid : %s\n\t\tip : %s\n\t\tTCP : %s\n", me->ext.id, me->ext.ip, me->ext.port);
    printf(" --> BACKUP : \n\t\tid : %s\n\t\tip : %s\n\t\tTCP : %s\n", me->bck.id, me->bck.ip, me->bck.port);
    printf(" --> INTERNS : \n");
    for (int i = 0; i < me->first_free_intern; i++) {
        printf("\t\tid : %s\n\t\tip : %s\n\t\tTCP : %s\n\n", me->intr[i].id, me->intr[i].ip, me->intr[i].port);
    }
    printf("\n\n");
}

void try_to_connect_to_network(app_t *me, fd_set *current_sockets)
{
    memmove(&me->bck, &me->ext, sizeof(node_t));
    if ((me->ext.fd = request_to_connect_to_node(me)) > 0) {
        FD_SET(me->ext.fd, current_sockets);
        printf("\nSENT REQUEST TO CONNECT TO: %s", me->ext.id);
    }
    else {
        printf("\nNOT POSSIBLE TO CONNECT");
        memmove(&me->ext, &me->self, sizeof(node_t));
    }
}

void inform_all_interns(app_t *me, fd_set *current_sockets, char msg[])
{
    for (int i = 0; i < me->first_free_intern; i++) {
        if (write(me->intr[i].fd, msg, strlen(msg)) <= 0) {
            clear_file_descriptor(me->intr[i].fd, current_sockets);
            printf("\nINTERN LEFT: %s", me->intr[i].id);
            remove_intern(i, me);
        }
    }
}

void clear_all_file_descriptors(app_t *me, fd_set *sockets)
{
    clear_file_descriptor(me->ext.fd, sockets);
    for (int i = 0; i < me->first_free_intern; i++) {
        clear_file_descriptor(me->intr[i].fd, sockets);
    }
}

void clear_file_descriptor(int fd, fd_set *sockets)
{
    FD_CLR(fd, sockets);
    close(fd);
}

void remove_intern(int i, app_t *me)
{
    me->first_free_intern--;
    if (i < me->first_free_intern) {
        memmove(&me->intr[i], &me->intr[me->first_free_intern], sizeof(node_t));
    }
}

int ask_for_net_nodes(char buffer[], char net[], char regIP[], char regUDP[])
{
    struct addrinfo hints, *res;
    int fd, bytes_read;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(regIP, regUDP, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    
    sprintf(buffer, "NODES %s", net);
    if (sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen) < 0) {
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

void join_network(char net[], app_t *me, char regIP[], char regUDP[])
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
    if (getaddrinfo(regIP, regUDP, &hints, &res) != 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
        close(fd);
        return;
    }
    
    sprintf(msg, "REG %s %s %s %s", net, me->self.id, me->self.ip, me->self.port);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: CONNECTING TO NETWORK");
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

void leave_network(char net[], app_t *me, char regIP[], char regUDP[])
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
    if (getaddrinfo(regIP, regUDP, &hints, &res) != 0) {
        printf("\nERROR: LEAVING NETWORK");
        close(fd);
        return;
    }
    
    sprintf(msg, "UNREG %s %s", net, me->self.id);
    if (sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("\nERROR: LEAVING NETWORK");
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
    if (write(fd, msg, strlen(msg)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
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

    if (listen(fd, 99) < 0) {
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    return fd;
}

int accept_tcp_connection(int server_fd, app_t *me)
{
    int bytes_read;
    char msg[64];
    node_t newnode, ext;

    if ((newnode.fd = accept(server_fd, NULL, NULL)) < 0) {
        return -1;
    }

    if ((bytes_read = read(newnode.fd, msg, 64)) < 0) {
        close(newnode.fd);
        return -1;
    }
    msg[bytes_read] = '\0';

    sscanf(msg, "NEW %s %s %s", newnode.id, newnode.ip, newnode.port);
    
    if (strcmp(me->ext.id, me->self.id) == 0) {
        printf("\nPROMOTING %s TO EXTERN", newnode.id);
        memmove(&me->ext, &newnode, sizeof(node_t));
        memmove(&me->bck, &me->self, sizeof(node_t));
    }
    else {
        memmove(&me->intr[me->first_free_intern++], &newnode, sizeof(node_t));
    }

    sprintf(msg, "EXTERN %s %s %s\n", me->ext.id, me->ext.ip, me->ext.port);
    if (write(newnode.fd, msg, strlen(msg)) < 0) {
        close(newnode.fd);
        return -1;
    }

    return newnode.fd;
}
