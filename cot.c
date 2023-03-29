#include "validate.h"
#include "network.h"

#include <signal.h>

int main(int argc, char *argv[])
{
    if (!valid_command_line_arguments(argc, argv)) exit(1);
    setbuf(stdout, NULL);
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        exit(1);
    }

    fd_set ready_sockets, current_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(0, &current_sockets);

    int out_fds, newfd, len;
    char buffer[BUFFER_SIZE], msg[BUFFER_SIZE];
    
    char token[] = " \n\t", name[128];

    app_t me;
    strcpy(me.self.ip, argv[1]);
    strcpy(me.self.port, argv[2]);
    me.self.buffer[0] = '\0';
    me.self.fd = -1;
    
    memmove(&me.ext, &me.self, sizeof(node_t));
    memmove(&me.bck, &me.self, sizeof(node_t));
    
    me.first_free_intern = 0;
    reset_expedition_list(&me);

    if (argc == 5) {
        strcpy(me.regIP, argv[3]);
        strcpy(me.regUDP, argv[4]);
    }
    else {
        strcpy(me.regIP, "193.136.138.142");
        strcpy(me.regUDP, "59000");
    }
    
    post_t post;
    files_t files;
    files.first_free_name = 0;

    if((me.self.fd = open_tcp_connection(me.self.port)) < 0) {
        printf("ERROR: OPENING TCP CONNECTION\n");
        exit(1);
    }

    int bytes_read;
    
    while (1)
    {
        ready_sockets = current_sockets;
        printf("\nCOMMAND: ");
        out_fds = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        switch (out_fds)
        {
        case -1:
            perror("\nselect");
            exit(1);
            break;
        
        default:
            // check for console input
            if (FD_ISSET(0, &ready_sockets)) {
                // read input
                fgets(buffer, BUFFER_SIZE, stdin);

                // handle join
                if (sscanf(buffer, "join %s %s", me.net, me.self.id) == 2) {
                    if (!join_arguments(&me)) {
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }

                    join(&me, &current_sockets);
                }
                else if (sscanf(buffer, "djoin %s %s %s %s %s", me.net, me.self.id, me.ext.id, me.ext.ip, me.ext.port) == 5) {
                    if (!djoin_arguments(&me)) {
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }
                    if (djoin(&me, &current_sockets) > 0) {
                        FD_SET(me.self.fd, &current_sockets);
                        printf("\nDJOIN");
                    }
                }
                else if (strcmp(buffer, "leave\n") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets);
                    me.first_free_intern = 0;
                    leave_network(&me);
                    reset_expedition_list(&me);
                    memmove(&me.ext, &me.self, sizeof(node_t));
                    memmove(&me.bck, &me.self, sizeof(node_t));
                    FD_CLR(me.self.fd, &current_sockets);
                }
                else if (strcmp(buffer, "st\n") == 0 || strcmp(buffer, "show topology\n") == 0) {
                    show_topology(&me);
                }
                else if (sscanf(buffer, "create %s", name) == 1) {
                    if (create_file(name, &files)) {
                        printf("\nNEW FILE: %s", name);
                    }
                    else {
                        printf("\nERROR: CREATING FILE");
                    }
                }
                else if (strcmp(buffer, "sn\n") == 0 || strcmp(buffer, "show names\n") == 0) {
                    show_names(&files);
                }
                else if (sscanf(buffer, "delete %s", name) == 1) {
                    if (delete_file(name, &files)) {
                        printf("\nDELETED FILE: %s", name);
                    }
                    else {
                        printf("\nERROR : DELETING FILE");
                    }
                }
                else if (sscanf(buffer, "get %s %s", post.dest, post.name) == 2) {
                    strcpy(post.orig, me.self.id);
                    if (!get_arguments(&post)) {
                        printf("\nERROR: INVALID ARGUMENTS");
                        break;
                    }
                    sprintf(buffer, "QUERY %s %s %s\n", post.dest, me.self.id, post.name);
                    forward_message(&me, &post, NULL, buffer);
                }
                else if (strcmp(buffer, "sr\n") == 0 || strcmp(buffer, "show routing\n") == 0) {
                    show_routing(&me);
                }
                else if (strcmp(buffer, "cr\n") == 0 || strcmp(buffer, "clear routing\n") == 0) {
                    reset_expedition_list(&me);
                }
                else if (strcmp(buffer, "exit\n") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets);
                    clear_file_descriptor(me.self.fd, &current_sockets);
                    leave_network(&me);
                    putchar(10);
                    exit(0);
                }
                else {
                    printf("\nCOULD NOT FIND THE COMMAND");
                }
            }
            if (FD_ISSET(me.self.fd, &ready_sockets)) {
                if ((newfd = accept_tcp_connection(&me)) > 0) {
                    FD_SET(newfd, &current_sockets);
                }
                else{
                    printf("\nERROR: ACCEPTING NODE");
                }
            }
            if (FD_ISSET(me.ext.fd, &ready_sockets) && me.ext.fd != me.self.fd) {
                if (read_msg(&me.ext) < 0) {
                    printf("\nEXTERN LEFT: %s", me.ext.id);
                    clear_leaver(&me.ext, &me, &current_sockets);
                    reconnect(&me, &current_sockets);
                }
                else {
                    handle_buffer(&me.ext, &me, &files);
                }
            }
            for (int i = 0; i < me.first_free_intern; i++) {
                if (FD_ISSET(me.intr[i].fd, &ready_sockets)) {
                    if (read_msg(&me.intr[i]) < 0) {
                        printf("\nINTERN LEFT: %s", me.intr[i].id);
                        clear_leaver(&me.intr[i], &me, &current_sockets);
                        remove_intern(i, &me);
                    }
                    else {
                        handle_buffer(&me.intr[i], &me, &files);
                    }
                }
            }
            
            break;
        }
    }
    
    return 0;
}
