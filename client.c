#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_GROUP_LENGTH 50
#define MAX_MESSAGE_LENGTH 140
#define MAX_USERNAME_LENGTH 50
#define MAX_SERVER_RESPONSE_LENGTH 1000

// #define 10000MULTICAST_GROUP_IP "225.0.0.0"
// #define MULTICAST_GROUP_PORT 2000

#define SERVER_IP "192.168.1.6"
#define SERVER_PORT 12000

pthread_mutex_t chat_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct Group_Info_ {
	char *multicast_group_addr;
	int multicast_group_port;
} Group_Info;


void err_exit() {
	perror("chat_client");
	exit(1);
}

int validate_request(char *request) {
	char buffer[100];
	char *buff_ptr, *operation, *groupname;
	int c;
	
	memset(&buffer, 0, sizeof(buffer));
	strncpy(buffer, request, sizeof(buffer));
	printf("%s", buffer);
	buff_ptr = buffer;
	operation = strsep(&buff_ptr, " ");
	groupname = strsep(&buff_ptr, " ");

	if (strncmp(operation, "create", strlen("create")) != 0 && strncmp(operation, "join", strlen("join")) != 0 && 
			strncmp(operation, "get", strlen("get")) != 0 && strncmp(operation, "leave", strlen("leave"))) {
		return -1;
	}
	if (strlen(groupname) > MAX_GROUP_LENGTH) {
		return 0;
	}
	return 1;
}

void *listen_to_group(void *arg) {
	struct sockaddr_in addr;
	unsigned char ttl;
	int addr_len, sfd, rec_mess_len, reuse;
	struct ip_mreq mreq;
	char formatted_message[MAX_USERNAME_LENGTH + MAX_MESSAGE_LENGTH + 3];

	if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		err_exit();
	}

	reuse = 1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		err_exit();
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// addr.sin_port = htons(MULTICAST_GROUP_PORT);
	addr_len = sizeof(addr);

	if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		err_exit();
	}

	// mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP_IP);
	// mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ttl = 255;
	setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	if (setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
		err_exit();
	}
	
	while (1) {
		memset(formatted_message, 0, sizeof(formatted_message));
		if ((rec_mess_len = recvfrom(sfd, formatted_message, sizeof(formatted_message), 0, (struct sockaddr *) &addr, &addr_len)) == -1) {
			err_exit();
		}
		pthread_mutex_lock(&chat_lock);
		printf("\33[2K\r");
		printf("%s", formatted_message);
		printf(">>> ");
		fflush(stdout);
		pthread_mutex_unlock(&chat_lock);
	}
}

void join_group(char *name) {
	struct sockaddr_in group_addr;
	int sfd;
	unsigned char ttl;
	char message[MAX_MESSAGE_LENGTH + 1];
	char formatted_message[MAX_USERNAME_LENGTH + MAX_MESSAGE_LENGTH + 3];
	char *username = name;
	pthread_t tid;

	if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		err_exit();
	}
	
	memset(&group_addr, 0, sizeof(group_addr));
	group_addr.sin_family = AF_INET;
	// group_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP_IP);
	// goup_addr.sin_port = htons(MULTICAST_GROUP_PORT);

	ttl = 255;
	setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	pthread_create(&tid, NULL, listen_to_group, NULL);

	while (1) {
		memset(message, 0, sizeof(message));
		memset(formatted_message, 0, sizeof(formatted_message));
		pthread_mutex_lock(&chat_lock);
		printf(">>> ");
		fflush(stdout);
		pthread_mutex_unlock(&chat_lock);
		fgets(message, sizeof(message), stdin);
		snprintf(formatted_message, sizeof(formatted_message), "%s: %s", username, message);
		if (sendto(sfd, formatted_message, sizeof(formatted_message), 0, (struct sockaddr *) &group_addr, sizeof(group_addr)) == -1) {
			err_exit();
		}
	}
}

int main(int argc, char *argv[]) {
	struct sockaddr_in client_addr, server_addr;
	int sfd, valid_request;
	char username[MAX_USERNAME_LENGTH + 1];
	char request[strlen("create ") + MAX_GROUP_LENGTH + 1];
	char formatted_request[MAX_USERNAME_LENGTH + sizeof(request) + 1];
	char server_response[MAX_SERVER_RESPONSE_LENGTH + 1];
	Group_Info *info_from_server;

	if (argc != 2) {
		printf("Usage: ./chat_app username\n");
		exit(1);
	}
	if (strlen(argv[1]) > MAX_USERNAME_LENGTH) {
		printf("Username must be 50 characters or less.");
		exit(1);
	}
	
	printf("Welcome!\n");
	snprintf(username, sizeof(username), "%s", argv[1]);

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr.sin_port = htons(8000);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(SERVER_PORT);

	if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		err_exit();
	}

	if (bind(sfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1) {
		err_exit();
	}

	info_from_server = malloc(sizeof(Group_Info));

	while (1) {
		printf("Options:\n\tcreate <groupname>\n\tjoin <groupname>\n\tget groupnames\n");
		printf(">>> ");
		memset(&request, 0, sizeof(request));
		memset(&formatted_request, 0, sizeof(formatted_request));
		memset(&info_from_server, 0, sizeof(info_from_server));
		memset(&server_response, 0, sizeof(server_response));
		fgets(request, sizeof(request), stdin);
		if ((valid_request = validate_request(request)) == 1) {
			snprintf(formatted_request, sizeof(formatted_request), "%s %s", username, request);
			printf("%s\n", formatted_request);
			// TODO: handle sendto and recvfrom errors
			sendto(sfd, formatted_request, sizeof(formatted_request), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
			recvfrom(sfd, info_from_server, sizeof(info_from_server), 0, NULL, NULL);
			printf("%s:%d\n", (*info_from_server).multicast_group_addr, (*info_from_server).multicast_group_port);
		}
		else if (valid_request == 0) {
			printf("Group name is too long, must be 50 characters or less\n");
		}
		else if (valid_request == -1) {
			printf("Invalid command\n");
		}
	}
}
