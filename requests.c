#include "requests.h"

/**
 * @brief TCP request
 * @param sendline request message
 * @param recvline request answer
 * @param addrinfo request address info
 */

void tcp_request(char sendline[], char recvline[], AI *addrinfo)
{
    int fd;

    // open socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        exit(1);
    }

    // open tcp connection
    if (connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
    {
        exit(1);
    }

    // write message request
    if (write(fd, sendline, strlen(sendline)) < 0)
    {
        exit(1);
    }

    // read request response
    if (read(fd, recvline, MAX_LINE) < 0)
    {
        exit(1);
    }

    // close socket
    close(fd);
}

/**
 * @brief UDP request
 * @param sendline request message
 * @param recvline request answer
 * @param addrinfo request address info
 */

void udp_request(char sendline[], char recvline[], AI *addrinfo)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // open socket
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        exit(1);
    }

    // send UDP request
    if (sendto(fd, sendline, strlen(sendline), 0, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
    {
        exit(1);
    }

    // receive UDP answer to request
    if (recvfrom(fd, recvline, MAX_LINE, 0, (struct sockaddr*) &addr, &addrlen) < 0)
    {
        exit(1);
    }

    // close socket
    close(fd);
}
