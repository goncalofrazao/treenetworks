#include "validate.h"
#include "network.h"

#include <signal.h>

int main(int argc, char *argv[])
{
    if (!valid_command_line_arguments(argc, argv)) exit(1); // validate program invocation
    setbuf(stdout, NULL); // set stdout buffer off
    // ignore SIGPIPE
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        exit(1);
    }

    fd_set ready_sockets, current_sockets;
    // set stdin fd
    FD_ZERO(&current_sockets);
    FD_SET(0, &current_sockets);

    int out_fds;
    char buffer[BUFFER_SIZE];
    
    char name[128];
    // init app
    app_t me;
    me.connected = false;
    strcpy(me.self.ip, argv[1]);
    strcpy(me.self.port, argv[2]);
    me.self.buffer[0] = '\0';
    me.self.fd = -1;
    
    memmove(&me.ext, &me.self, sizeof(node_t));
    memmove(&me.bck, &me.self, sizeof(node_t));
    
    me.first_free_intern = 0;
    reset_expedition_list(&me);
    // save server info
    if (argc == 5) {
        strcpy(me.regIP, argv[3]);
        strcpy(me.regUDP, argv[4]);
    }
    else { // default server info
        strcpy(me.regIP, "193.136.138.142");
        strcpy(me.regUDP, "59000");
    }
    
    post_t post;
    files_t files;
    files.first_free_name = 0;
    // open tcp server
    if((me.self.fd = open_tcp_connection(me.self.port)) < 0) {
        printf("ERROR: OPENING TCP CONNECTION\n");
        exit(1);
    }

    queue_t queue;
    queue.head = 0;
    
    while (1)
    {
        ready_sockets = current_sockets; // reset sockets
        printf("\nCOMMAND: ");
        out_fds = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        switch (out_fds)
        {
        case -1:
            perror("\nselect");
            exit(1);
            break;
        
        default:
            // console input
            if (FD_ISSET(0, &ready_sockets)) {
                // read input
                fgets(buffer, BUFFER_SIZE, stdin);

                // commands
                if (sscanf(buffer, "join %s %s", me.net, me.self.id) == 2) {
                    if (!join_arguments(&me)) { // check arguments
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }
                    // join network
                    join(&me, &current_sockets);
                }
                else if (sscanf(buffer, "djoin %s %s %s %s %s", me.net, me.self.id, me.ext.id, me.ext.ip, me.ext.port) == 5) {
                    if (!djoin_arguments(&me)) { // check arguments
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }
                    if (djoin(&me, &current_sockets) > 0) { // connect new extern or just become available to connections
                        FD_SET(me.self.fd, &current_sockets);
                        printf("\nDJOIN");
                    }
                }
                else if (strcmp(buffer, "leave\n") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets); // clear file descriptors of extern and interns
                    me.first_free_intern = 0; // remove all interns
                    me.connected = false;
                    leave_network(&me); // inform server
                    reset_expedition_list(&me); // reser expedition list
                    memmove(&me.ext, &me.self, sizeof(node_t)); // reset extern and backup
                    memmove(&me.bck, &me.self, sizeof(node_t));
                    FD_CLR(me.self.fd, &current_sockets); // stop listening to connections
                }
                else if (strcmp(buffer, "st\n") == 0 || strcmp(buffer, "show topology\n") == 0) {
                    show_topology(&me);
                }
                else if (sscanf(buffer, "create %s", name) == 1) {
                    if (create_file(name, &files)) { // create file
                        printf("\nNEW FILE: %s", name);
                    }
                    else { // struct of files full
                        printf("\nERROR: CREATING FILE");
                    }
                }
                else if (strcmp(buffer, "sn\n") == 0 || strcmp(buffer, "show names\n") == 0) {
                    show_names(&files);
                }
                else if (sscanf(buffer, "delete %s", name) == 1) {
                    if (delete_file(name, &files)) { // delete file
                        printf("\nDELETED FILE: %s", name);
                    }
                    else {
                        printf("\nERROR : DELETING FILE");
                    }
                }
                else if (sscanf(buffer, "get %s %s", post.dest, post.name) == 2) {
                    strcpy(post.orig, me.self.id); // set origin from request to me
                    if (!get_arguments(&post)) { // validate arguments
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }
                    sprintf(buffer, "QUERY %s %s %s\n", post.dest, me.self.id, post.name); // fowared query message
                    forward_message(&me, &post, NULL, buffer);
                }
                else if (strcmp(buffer, "sr\n") == 0 || strcmp(buffer, "show routing\n") == 0) {
                    show_routing(&me);
                }
                else if (strcmp(buffer, "cr\n") == 0 || strcmp(buffer, "clear routing\n") == 0) {
                    reset_expedition_list(&me);
                }
                else if (strcmp(buffer, "exit\n") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets); // clear all extern and intern file descriptors
                    clear_file_descriptor(me.self.fd, &current_sockets); // clear my file descriptor
                    leave_network(&me); // inform network I am leaving
                    putchar(10);
                    exit(0);
                }
                else {
                    printf("\nCOULD NOT FIND THE COMMAND");
                }
            }
            if (FD_ISSET(me.self.fd, &ready_sockets)) { // new connection
                if (queue.head < 99) { // only accept if queue is not full
                    accept_tcp_connection(&me, &queue, &current_sockets);
                    printf("\nNEW CONNECTION");
                }
                else { // if queue is full stop listening to connections
                    FD_CLR(me.self.fd, &current_sockets);
                }
            }
            if (FD_ISSET(me.ext.fd, &ready_sockets) && me.ext.fd != me.self.fd) { // message from extern
                if (read_msg(&me.ext) < 0) { // extern left
                    printf("\nEXTERN LEFT: %s", me.ext.id);
                    clear_leaver(&me.ext, &me, &current_sockets); // clear leaver node
                    reconnect(&me, &current_sockets); // reconnect to network
                }
                else {
                    handle_buffer(&me.ext, &me, &files); // handle messages from extern buffer
                }
            }
            for (int i = 0; i < me.first_free_intern; i++) { // message from intern
                if (FD_ISSET(me.intr[i].fd, &ready_sockets)) {
                    if (read_msg(&me.intr[i]) < 0) { // intern left
                        printf("\nINTERN LEFT: %s", me.intr[i].id);
                        clear_leaver(&me.intr[i], &me, &current_sockets); // clear intern
                        remove_intern(i, &me); // remove intern
                    }
                    else {
                        handle_buffer(&me.intr[i], &me, &files); // process intern buffer messages
                    }
                }
            }
            for (int i = 0; i < queue.head; i++) { // check all new connections
                if (calculate_time(i, &queue) > 1500) { // remove connections that exceed time to send NEW
                    printf("\nERROR: NO NEW MESSAGE");
                    remove_connection_from_queue(i, &queue, &current_sockets, DELETE); // remove connection
                    reset_fd(me.self.fd, &current_sockets); // reset server new connections if case
                    continue;
                }
                else if (FD_ISSET(queue.queue[i].fd, &ready_sockets)) {
                    if (read_msg(&queue.queue[i]) < 0) { // connection disconnected
                        remove_connection_from_queue(i, &queue, &current_sockets, DELETE);
                    }
                    else { // process connection message and promote if case
                        promote_from_queue(&me, &queue, i, &current_sockets);
                    }
                    reset_fd(me.self.fd, &current_sockets); // reset server new connections if case
                }
            }
            
            break;
        }
    }
    
    return 0;
}
