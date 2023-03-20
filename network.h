#ifndef _NETWORK_
#define _NETWORK_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct node_t {
    char id[3];
    char ip[16];
    char port[6];
    int fd;
} node_t;

typedef struct app_t {
    char net[4];
    char regIP[16];
    char regUDP[6];
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

#endif