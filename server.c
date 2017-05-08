/*
UDP-based server for a multi-user chat system. 
*/

#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_GROUP_MEMBERS 10
#define MAX_GROUP_NAME_SIZE 51
#define MAX_USERNAME_SIZE 21
#define MAX_GROUPS 50

#define MAX_MESSAGE_SIZE 101
#define PORT 12000 //Right now, used for the server (scroll all the way to main)
#define MULTICAST_PORT 2000
#define MULTICAST_START_ADDR "235.0.0."

typedef struct Chat_Group_ {
	char group_name[MAX_GROUP_NAME_SIZE];
	char multicast_group_addr[16];
	int multicast_group_port;	//always set to MULTICAST_PORT
	char users[MAX_GROUP_MEMBERS][MAX_USERNAME_SIZE];
} Chat_Group;

typedef struct Group_Info_ {
	char multicast_group_addr[16];
	int multicast_group_port;
} Group_Info;

Chat_Group CURRENT_CHAT_GROUPS[MAX_GROUPS];
int NUM_CURRENT_GROUPS = 0;


void err_exit(char* msg) {
	perror(msg);
	exit(1);
}

void send_error_msg(char* msg, int sockfd, struct sockaddr_in sockaddr) {
		int retval;
		if ((retval = sendto(sockfd, msg, sizeof(msg), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) == -1) {
			printf("sendto() failed to send %s\n", msg);
		}
		else if (retval == 0) {
			printf("sendto() sent 0 bytes of %s\n", msg);
		}
}

/* Finds first available slot in CURRENT_CHAT_GROUPS for new group */
int get_free_group_index(Chat_Group groups[]) {
	int i = 0;
	while (i < MAX_GROUPS) {
		if (strncmp(CURRENT_CHAT_GROUPS[i].group_name, "", 1) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/* Finds first available slot in CURRENT_CHAT_GROUPS for new group */
int get_free_user_index(Chat_Group *group) {
	int i = 0;
	while (i < MAX_GROUP_MEMBERS) {
		printf("user is %s\n",group->users[i]);
		if (strncmp(group->users[i], "", 1) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/* Helper function to handle create group requests. */
/*
Responses:
1. ERR_MAX_GROUPS- group can't be created since max group count is reached.
2. ERR_SAME_NAME- group with same name already exists
*/

void process_create_request(int sockfd, char* user, char* group, struct sockaddr_in sockaddr) {
	/* Check if we have space for new group */
	if (NUM_CURRENT_GROUPS == MAX_GROUPS) {
		send_error_msg("ERR_MAX_GROUPS", sockfd, sockaddr);
		return;
	}

	/* Check if group name already exists */
	int i = 0;
	while (i < MAX_GROUPS) {
		if (strcmp(CURRENT_CHAT_GROUPS[i].group_name, group) == 0) {
			send_error_msg("ERR_SAME_NAME", sockfd, sockaddr);
			return;
		}
		i++;
	}

	/* Create new group */
	int j = get_free_group_index(CURRENT_CHAT_GROUPS);
	printf("Index of new group: %d\n", j);
	strncpy(CURRENT_CHAT_GROUPS[j].group_name, group, sizeof(group)+1);
	printf("New group name: %s\n", CURRENT_CHAT_GROUPS[j].group_name);
	strncpy(CURRENT_CHAT_GROUPS[j].users[0], user, sizeof(user)+1);
	CURRENT_CHAT_GROUPS[j].multicast_group_port = MULTICAST_PORT;
	char buffer[16];
	memset(&buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s%d", MULTICAST_START_ADDR, j);
	strcpy(CURRENT_CHAT_GROUPS[j].multicast_group_addr, buffer);

	printf("%s\n", CURRENT_CHAT_GROUPS[j].multicast_group_addr);

	printf("Created group name: %s\n", CURRENT_CHAT_GROUPS[j].group_name);
	printf("Created group user: %s\n", CURRENT_CHAT_GROUPS[j].users[0]);
	printf("Created group IP: %s\n", CURRENT_CHAT_GROUPS[j].multicast_group_addr);
	printf("Created group port: %d\n", CURRENT_CHAT_GROUPS[j].multicast_group_port);

	NUM_CURRENT_GROUPS++;


	char message[100]; 
	memset(&message, 0, sizeof(message));
	sprintf(message, "%s %d", CURRENT_CHAT_GROUPS[j].multicast_group_addr, MULTICAST_PORT);
//	Group_Info info;
//	memset(&info, 0, sizeof(info));
//	info.multicast_group_addr = (char *) malloc(sizeof(CURRENT_CHAT_GROUPS[j].multicast_group_addr));
//	strcpy(info.multicast_group_addr, CURRENT_CHAT_GROUPS[j].multicast_group_addr);
//	strncpy(info.multicast_group_addr, CURRENT_CHAT_GROUPS[j].multicast_group_addr, sizeof(info.multicast_group_addr));
//	info.multicast_group_port = MULTICAST_PORT;

	printf("%s\n", message);

	int response_err;
	if ((response_err = sendto(sockfd, message, strlen(message) + 1, 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < sizeof(message)) {
		printf("Failed to send the entire datagram to the client.\n");
		return;
	}
}


/*
Responses:
1. ERR_GROUP_NOT_FOUND- group can't be found in the list of groups.
2. ERR_SAME_NAME- group with same name already exists

*/

void process_join_request(int sockfd, char* user, char* group, struct sockaddr_in sockaddr) {
	int i = 0;
	int group_index = -1;
	while (i < MAX_GROUPS) {
		if ((strcmp(CURRENT_CHAT_GROUPS[i].group_name, group)) == 0) {
			group_index = i;
			break;
		}
		i++;
	}

	if (group_index == -1) {
		send_error_msg("ERR_GROUP_NOT_FOUND", sockfd, sockaddr);
		return;
	}

	//Send client the multicast IP address and port.

	/*
	Group_Info info;
	memset(&info, '\0', sizeof(info));
	strcpy(info.multicast_group_addr, CURRENT_CHAT_GROUPS[group_index].multicast_group_addr);
	info.multicast_group_port = CURRENT_CHAT_GROUPS[group_index].multicast_group_port;
*/
	printf("Group index is %d\n",group_index);
	printf("Group IP: %s\n", CURRENT_CHAT_GROUPS[group_index].multicast_group_addr);
	printf("Group port: %d\n", MULTICAST_PORT);
	
	char message[100]; 
	memset(&message, 0, sizeof(message));
	sprintf(message, "%s %d", CURRENT_CHAT_GROUPS[group_index].multicast_group_addr, MULTICAST_PORT);

	int location = get_free_user_index(&CURRENT_CHAT_GROUPS[group_index]);
	printf("Open spot for user %s is: %d\n", user, location);
	strncpy(CURRENT_CHAT_GROUPS[group_index].users[location], user, sizeof(user)+1);
	printf("%s\n", CURRENT_CHAT_GROUPS[group_index].users[location]);

	ssize_t multicast_info_sent_bytes = 0;
	if ((multicast_info_sent_bytes = sendto(sockfd, message, strlen(message) + 1, 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < sizeof(message)) {
		printf("Error in joining group: cannot send all multicast info in sendto().\n");
	}
}

int remove_group_user(int i, char* user) {
	//strncpy(CURRENT_CHAT_GROUPS[j].users[0], user, sizeof(user)+1);
	//char users[MAX_GROUP_MEMBERS][MAX_USERNAME_SIZE];
	int index;
	for (index = 0; index < MAX_GROUP_MEMBERS; index++) {
		if (strcmp(CURRENT_CHAT_GROUPS[i].users[index], user) == 0) {
			memset(CURRENT_CHAT_GROUPS[i].users[index], 0, MAX_USERNAME_SIZE+1);
			return 0;
		}
	}
	return -1;
}

void process_getnames_request(int sockfd, struct sockaddr_in cli) {
	char* buf;
	if ((buf = malloc(strlen(CURRENT_CHAT_GROUPS[0].group_name) + 1)) == NULL) {	//initialize buffer by malloc()ing for first group name
		printf("Error in malloc() for group names buffer\n");
	}
	bzero(buf, strlen(buf));
	memcpy(buf, CURRENT_CHAT_GROUPS[0].group_name, strlen(CURRENT_CHAT_GROUPS[0].group_name));	//copy first group name to buffer
	//char buf[MAX_GROUPS][MAX_GROUP_NAME_SIZE];
	memcpy(buf + strlen(CURRENT_CHAT_GROUPS[0].group_name), "|", 1);

	printf("Buffer after malloc: %s\n", buf);
	int prev_malloc_size = strlen(CURRENT_CHAT_GROUPS[0].group_name) + 1;
	printf("Malloced size after 1st malloc: %d\n", prev_malloc_size);

	int i;
	int group_size;
	//int next_group_index = 1;
	for (i = 1; i < MAX_GROUPS; i++) {
		/*if (&CURRENT_CHAT_GROUPS[i] != NULL) {*/
		if (strncmp(CURRENT_CHAT_GROUPS[i].group_name, "", 1) != 0) {
			printf("Found non-empty group\n");
			group_size = strlen(CURRENT_CHAT_GROUPS[i].group_name) + 1; // + 1 to include newline separator
			if ((buf = realloc(buf, (prev_malloc_size + group_size))) == NULL) {
				printf("Error in realloc()\n");
				return;
			}
			memcpy((buf + prev_malloc_size), CURRENT_CHAT_GROUPS[i].group_name, strlen(CURRENT_CHAT_GROUPS[i].group_name));
			
			memcpy((buf + prev_malloc_size + group_size-1), "|", 1);
			
			printf("after memcpy: %s\n", buf);
			//strcpy(&buf[next_group_index][0], CURRENT_CHAT_GROUPS[i].group_name);
			//buf[next_group_index] = CURRENT_CHAT_GROUPS[i].group_name;
			//next_group_index++;
			prev_malloc_size += group_size;
			printf("New prev_malloc_size: %d\n", prev_malloc_size);
		}
	}
	if ((sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &cli, sizeof(cli))) == -1) {
		printf("Error in sendto() for sending response in get names\n");
	}
}

void process_leave_request(int sockfd, char* user, char* group, struct sockaddr_in sockaddr) {
	int i;
	for (i = 0; i < MAX_GROUPS; i++) {
		if (strcmp(CURRENT_CHAT_GROUPS[i].group_name, group) == 0) {
			if (remove_group_user(i, user) == 0) {
				sendto(sockfd, "success", strlen("success"), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
				return;
			}
		}
	}
	sendto(sockfd, "failure", strlen("failure"), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}


/* Main server processing- continuously accepts requests and handles in helper functions */
void func(int sockfd) {
	char buff[MAX_MESSAGE_SIZE];
	int clen;
	struct sockaddr_in cli;
	clen=sizeof(cli);
	for(;;) {
		bzero(&cli, sizeof(cli));
		bzero(buff,MAX_MESSAGE_SIZE);
		recvfrom(sockfd,buff,sizeof(buff),0,(struct sockaddr *)&cli,&clen);
		printf("From client %s To client",buff);

		char* username = strtok(buff, " ");
		char* req_type = strtok(NULL, " ");
		char* parameter = strtok(NULL, "\n");

		if (parameter == NULL) { 
			err_exit("Error: malformed input.");
		}

		if(strcmp(req_type, "get") == 0) {
			process_getnames_request(sockfd, cli);
		}

		else if(strcmp(req_type, "create") == 0) {
			process_create_request(sockfd, username, parameter, cli);
		}

		else if(strcmp(req_type, "join") == 0) {
			process_join_request(sockfd, username, parameter, cli);
		}
		/*
		else if(strcmp(req_type, "leave") == 0) {
			process_leave_request(sockfd, username, parameter, cli);
		}
		*/
		//TODO: HANDLE BETTER
		//Request is malformed
		/*else {
			err_exit("Error: invalid request.");
		}*/
	}
}



int main() {
	/* Initialization */
	bzero(&CURRENT_CHAT_GROUPS, sizeof(CURRENT_CHAT_GROUPS));
	//memset(CURRENT_CHAT_GROUPS, '\0', strlen(CURRENT_CHAT_GROUPS));

	int sockfd;
	struct sockaddr_in servaddr;
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd==-1) {
		printf("socket creation failed...\n");
		exit(0);
	}
		
	printf("Socket successfully created..\n");
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(PORT);

	if((bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)))!=0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	
	printf("Socket successfully binded..\n");
	func(sockfd);
	close(sockfd);
}
