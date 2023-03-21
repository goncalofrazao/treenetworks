#include "network.h"

int ask_for_net_nodes(char buffer[], app_t *me)
{
    struct addrinfo hints, *res;
    int fd, bytes_read;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(me->regIP, me->regUDP, &hints, &res) != 0) {
        close(fd);
        return -1;
    }
    
    sprintf(buffer, "NODES %s", me->net);
    if (sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    if ((bytes_read = recvfrom(fd, buffer, BUFFER_SIZE, 0, NULL, NULL)) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }
    buffer[bytes_read] = '\0';
    
    close(fd);
    freeaddrinfo(res);

    return 1;
}
