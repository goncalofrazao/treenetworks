#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PORT "59000"
#define BUFFER_SIZE 1048576

int main(int argc, char *argv[]){

	if (argc != 2) {
		printf("usage: ./reg [net]\n");
		exit(1);
	}

	char ip[] = "tejo.tecnico.ulisboa.pt";
    int net_to_disconnect = atoi(argv[1]);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	char buffer[BUFFER_SIZE];
	char msg[128] = "UNREG";

	struct addrinfo hints, *res;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(ip, PORT, &hints, &res);

    for (int net = 0; net < 1000; net++)
    {
        if (net != net_to_disconnect) {
            continue;
        }
        for (int id = 0; id < 100; id++)
        {
            sprintf(msg, "REG %03d %02d 0.0.0.0 65535", net, id);
            printf("msg: %s\n", msg);
			sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
			recvfrom(fd, buffer, BUFFER_SIZE, 0, NULL, NULL);
			printf("%s\n", buffer);
        }
        
    }

	close(fd);
	freeaddrinfo(res);

	return 0;
}
