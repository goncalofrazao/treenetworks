#ifndef _REQUESTS_
#define _REQUESTS_

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define MAX_LINE 1024
typedef struct addrinfo AI;

void tcp_request(char sendline[], char recvline[], AI *addrinfo);
void udp_request(char sendline[], char recvline[], AI *addrinfo);

#endif