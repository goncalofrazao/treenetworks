#ifndef _VALIDATIONS_
#define _VALIDATIONS_

#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool valid_command_line_arguments(int argc, char *argv[]);
int already_in_network(char new_id[], char network_info[], app_t *me);
bool choose_new_id(app_t *me);
bool join_arguments(app_t *me);
bool djoin_arguments(app_t *me);
bool get_arguments(post_t *post);
bool node_copy(app_t *me, node_t *newnode);
bool extern_arguments(node_t *node);
bool valid_id(char *strid);
bool content_arguments(post_t *post);

#endif
