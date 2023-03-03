#ifndef _COMMANDS_
#define _COMMANDS_

typedef enum {
    join,
    leave,
    UNKOWN,
} CMD;

typedef struct {
    CMD command;
    char args[2][32];
} CAAS;


void get_command(char buffer[], CAAS* command);

#endif