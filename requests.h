#ifndef _REQUESTS_
#define _REQUESTS_

void tcp_request(char sendline[], char recvline[], AI *addrinfo);
void udp_request(char sendline[], char recvline[], AI *addrinfo);

#endif