#ifndef _VALIDATIONS_
#define _VALIDATIONS_

#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool valid_command_line_arguments(int argc, char *argv[]);
int already_in_network(char new_id[], char network_info[]);
bool choose_new_id(app_t *me);

#endif