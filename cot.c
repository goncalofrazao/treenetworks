#include "validate.h"
#include "network.h"

int main(int argc, char *argv[])
{
    if (!valid_command_line_arguments(argc, argv)) exit(1);
    setbuf(stdout, NULL);

    fd_set ready_sockets, current_sockets;
    
    FD_ZERO(&current_sockets);
    FD_SET(0, &current_sockets);

    int out_fds, newfd, len;
    char buffer[BUFFER_SIZE], id[3], msg[BUFFER_SIZE];
    
    char *c, token[] = " \n\t";

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
                    strcpy(me.net, c);
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.self.id, c);

                    memmove(&me.ext, &me.self, sizeof(node_t));
                    memmove(&me.bck, &me.self, sizeof(node_t));

                    // ask for net nodes
                    if (ask_for_net_nodes(buffer, &me) < 0) {
                        break;
                    }

                    if ((len = already_in_network(me.self.id, buffer)) == -1) {
                        printf("\nSERVER ANSWER IN WRONG FORMAT");
                        break;
                    }
                    else if (len == 1) {
                        printf("\nERROR: UNAVAILABLE ID");
                        if (!choose_new_id(&me)) {
                            printf("\nERROR: PICKING NEW NODE");
                            break;
                        }
                    }

                    // ask for net nodes
                    if (ask_for_net_nodes(buffer, &me) < 0) {
                        break;
                    }

                    if (sscanf(buffer, "%*s %*s %s %s %s", me.ext.id, me.ext.ip, me.ext.port) == 3) {
                        try_to_connect_to_network(&me, &current_sockets);
                    }

                    join_network(&me);
                    
                }
                else if (strcmp(c, "djoin") == 0) {
                    c = strtok(NULL, token);
                    if (c == NULL) break;
                    strcpy(me.net, c);
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
                    leave_network(&me);
                    reset_expedition_list(&me);
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
                    forward_message(&me, &post, NULL, buffer);
                }
                else if (strcmp(c, "sr") == 0) {
                    show_routing(&me);
                }
                else if (strcmp(c, "exit") == 0) {
                    clear_all_file_descriptors(&me, &current_sockets);
                    leave_network(&me);
                    putchar(10);
                    exit(0);
                }
            }
            if (FD_ISSET(me.self.fd, &ready_sockets)) {
                if ((newfd = accept_tcp_connection(&me)) > 0) {
                    FD_SET(newfd, &current_sockets);
                }
                else{
                    printf("\nCOULD NOT ACCEPT NEW NODE");
                }
            }
            if (FD_ISSET(me.ext.fd, &ready_sockets) && me.ext.fd != me.self.fd) {
                if (read_msg(&me.ext) < 0) {

                    sprintf(buffer, "WITHDRAW %s\n", me.ext.id);
                    send_to_all_except_to_sender(&me.ext, &me, buffer);
                    clear_leaver_node_from_expedition_list(&me.ext, &me);

                    clear_file_descriptor(me.ext.fd, &current_sockets);
                    printf("\nEXTERN LEFT: %s", me.ext.id);

                    if (strcmp(me.bck.id, me.self.id) != 0) {
                        memmove(&me.ext, &me.bck, sizeof(node_t));

                        printf("\nRECONNECT TO: %s", me.bck.id);
                        if ((me.ext.fd = request_to_connect_to_node(&me)) < 0) {
                            printf("\nFAILED TO RECONNECT");
                            break;
                        }
                        FD_SET(me.ext.fd, &current_sockets);

                        sprintf(buffer, "EXTERN %s %s %s\n", me.ext.id, me.ext.ip, me.ext.port);
                        inform_all_interns(&me, buffer);
                    }
                    else if (me.first_free_intern > 0) {
                        memmove(&me.ext, &me.intr[0], sizeof(node_t));
                        printf("\nPROMOTED %s TO EXTERN", me.ext.id);
                        sprintf(buffer, "EXTERN %s %s %s\n", me.ext.id, me.ext.ip, me.ext.port);
                        inform_all_interns(&me, buffer);
                        remove_intern(0, &me);
                    }
                    else {
                        printf("\nI AM SO LONELY, CALL TOMAS GLORIA PLEASE TO SUCK MY DICK");
                        memmove(&me.ext, &me.self, sizeof(node_t));
                    }
                }
                else {
                    handle_buffer(&me.ext, &me, &files);
                }
            }
            for (int i = 0; i < me.first_free_intern; i++) {
                if (FD_ISSET(me.intr[i].fd, &ready_sockets)) {
                    if (read_msg(&me.intr[i]) < 0) {

                        sprintf(buffer, "WITHDRAW %s\n", me.intr[i].id);
                        send_to_all_except_to_sender(&me.intr[i], &me, buffer);
                        clear_leaver_node_from_expedition_list(&me.intr[i], &me);
                        
                        clear_file_descriptor(me.intr[i].fd, &current_sockets);
                        remove_intern(i, &me);
                        
                        printf("\nINTERN LEFT: %s", me.intr[i].id);
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
