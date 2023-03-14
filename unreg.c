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
		printf("usage: ./unreg [net]\n");
		exit(1);
	}

	char ip[] = "tejo.tecnico.ulisboa.pt";

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	char buffer[BUFFER_SIZE];
	char msg[128] = "UNREG";

	struct addrinfo hints, *res;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(ip, PORT, &hints, &res);

	sendto(fd, "LST", strlen("LST"), 0, res->ai_addr, res->ai_addrlen);

	recvfrom(fd, buffer, BUFFER_SIZE, 0, (void*) NULL, NULL);

	char token[] = " \n\t", *c;
	char net[10], id[10];
	
	strtok(buffer, token);
	
	while (1) {
		if ((c = strtok(NULL, token)) == NULL) break;
		strcpy(net, c);
		if ((c = strtok(NULL, token)) == NULL) break;
		strcpy(id, c);
		if (strtok(NULL, token) == NULL) break;
		if (strtok(NULL, token) == NULL) break;
		if (strcmp(net, argv[1]) == 0 || strcmp(argv[1], "all") == 0) {
			sprintf(msg, "UNREG %s %s", net, id);

			printf("%s\n", msg);
			sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);

			recvfrom(fd, msg, BUFFER_SIZE, 0, NULL, NULL);
			printf("%s\n", msg);
		}
	}

	close(fd);
	freeaddrinfo(res);

	return 0;
}
