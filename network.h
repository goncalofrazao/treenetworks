#ifndef _NETWORK_
#define _NETWORK_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 4096
#define DELETE 1
#define NOT_DELETE 0

typedef struct node_t {
    char id[3];
    char ip[16];
    char port[6];
    int fd;
    clock_t timer;
    char buffer[BUFFER_SIZE];
} node_t;

typedef struct app_t {
    bool connected;
    char net[4];
    char regIP[16];
    char regUDP[6];
    node_t self;
    node_t ext;
    node_t bck;
    node_t intr[100];
    int first_free_intern;
    node_t *expedition_list[100];
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

typedef struct queue_t {
    node_t queue[100];
    int head;
} queue_t;

int ask_for_net_nodes(char buffer[], app_t *me);
int read_msg(node_t *sender);
void accept_tcp_connection(app_t *me, queue_t *queue, fd_set *current_sockets);
int open_tcp_connection(char port[]);
void leave_network(app_t *me);
void remove_intern(int i, app_t *me);
void clear_all_file_descriptors(app_t *me, fd_set *sockets);
void clear_file_descriptor(int fd, fd_set *sockets);
int delete_file(char *filename, files_t *files);
void show_names(files_t *files);
int create_file(char *filename, files_t *files);
void show_topology(app_t *me);
void forward_message(app_t *me, post_t *post, node_t *sender, char buffer[]);
void show_routing(app_t *me);
void reset_expedition_list(app_t *me);
void handle_buffer(node_t *sender, app_t *me, files_t *files);
void clear_leaver(node_t *leaver, app_t *me, fd_set *current_sockets);
void reconnect(app_t *me, fd_set *current_sockets);
void join(app_t *me, fd_set *current_sockets);
int djoin(app_t *me, fd_set *current_sockets);
int calculate_time(int i, queue_t *queue);
void remove_node_from_queue(int i, queue_t *queue, fd_set *current_sockets, int delete);
void promote_from_queue(app_t *me, queue_t *queue, int i, fd_set *current_sockets);
void reset_fd(int fd, fd_set *current_sockets);

#endif