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


#define MAX_GROUP_MEMBERS 5
#define MAX_GROUP_NAME_SIZE 51
#define MAX_USERNAME_SIZE 21
#define MAX_GROUPS 50

#define MAX_MESSAGE_SIZE 101
#define PORT 10000 //Right now, used for the server (scroll all the way to main)
#define MULTICAST_PORT 2000
#define MULTICAST_START_ADDR "235.0.0."

typedef struct Chat_Group_ {
	char group_name[MAX_GROUP_NAME_SIZE];
	char* multicast_group_addr;
	int multicast_group_port;	//always set to MULTICAST_PORT
	char users[MAX_GROUP_MEMBERS][MAX_USERNAME_SIZE];
} Chat_Group;

typedef struct Group_Info_ {
	char* multicast_group_addr;
	int multicast_group_port;
} Group_Info;

Chat_Group CURRENT_CHAT_GROUPS[MAX_GROUPS];
int NUM_CURRENT_GROUPS = 0;


void err_exit(char* msg) {
	perror(msg);
	exit(1);
}

/* Finds first available slot in CURRENT_CHAT_GROUPS for new group */
int get_free_group_index(Chat_Group groups[]) {
	int i = 0;
	while (i < MAX_GROUPS) {
		if (&CURRENT_CHAT_GROUPS[i] == NULL) {
			return i;
		}
		i++;
	}
	return -1;
}

/* Helper function to handle create group requests. */
void process_create_request(int sockfd, char* user, char* group, struct sockaddr_in sockaddr) {
	/* Check if we have space for new group */
	if (NUM_CURRENT_GROUPS == MAX_GROUPS) {
		err_exit("Error in creating group: max group number reached.");
	}

	/* Check if group name already exists */
	int i = 0;
	while (i < MAX_GROUPS) {
		if (strcmp(CURRENT_CHAT_GROUPS[i].group_name, group) == 0) {
			err_exit("Error in creating group: a group with the specified name already exists.");
		}
		i++;
	}

	/* Create new group */
	int new_group_index = get_free_group_index(CURRENT_CHAT_GROUPS);
	strncpy(CURRENT_CHAT_GROUPS[i].group_name, group, MAX_GROUP_NAME_SIZE);
	strncpy(CURRENT_CHAT_GROUPS[i].users[0], user, MAX_USERNAME_SIZE);
	CURRENT_CHAT_GROUPS[i].multicast_group_port = MULTICAST_PORT;

	sprintf(CURRENT_CHAT_GROUPS[i].multicast_group_addr, "%s%d", MULTICAST_START_ADDR, i);
	printf("Created group name: %s\n", CURRENT_CHAT_GROUPS[i].group_name);
	printf("Created group user: %s\n", CURRENT_CHAT_GROUPS[i].users[0]);
	printf("Created group IP: %s\n", CURRENT_CHAT_GROUPS[i].multicast_group_addr);
	printf("Created group port: %d\n", CURRENT_CHAT_GROUPS[i].multicast_group_port);

	NUM_CURRENT_GROUPS++;

	Group_Info info;
	strcpy(info.multicast_group_addr, CURRENT_CHAT_GROUPS[i].multicast_group_addr);
	info.multicast_group_port = MULTICAST_PORT;

	if (sendto(sockfd, &info, sizeof(info), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) == -1) {
		err_exit("Error sending datagram to client from create group.");
	}
}


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
		err_exit("Error in joining group: invalid group name.");
	}

	//Send client the multicast IP address and port.
	Group_Info info;
	memset(&info, '\0', sizeof(info));
	info.multicast_group_addr = CURRENT_CHAT_GROUPS[group_index].multicast_group_addr;
	info.multicast_group_port = CURRENT_CHAT_GROUPS[group_index].multicast_group_port;

	printf("Group IP: %s\n", info.multicast_group_addr);
	printf("Group port: %d\n", info.multicast_group_port);


	ssize_t multicast_info_sent_bytes = 0;
	if ((multicast_info_sent_bytes = sendto(sockfd, &info, sizeof(info), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < sizeof(info)) {
		err_exit("Error in joining group: cannot send all multicast info in sendto().");
	}
}
/*
void remove_group_user(char** group_members, char* user) {
	int index;
	char* curr_user;
	for (index = 0; index < strlen(group_members); index += ) {
		if ((strcmp(group_members), user) == 0) {
			memset(*group_members)
		}
	}
}
*/
void process_getnames_request(int sockfd, struct sockaddr_in cli) {
	char buf[MAX_GROUPS][MAX_GROUP_NAME_SIZE];
	int i;
	int next_group_index = 0;
	for (i = 0; i < NUM_CURRENT_GROUPS; i++) {
		if (&CURRENT_CHAT_GROUPS[i] != NULL) {
			strcpy(&buf[next_group_index][0], CURRENT_CHAT_GROUPS[i].group_name);
			//buf[next_group_index] = CURRENT_CHAT_GROUPS[i].group_name;
			next_group_index++;
		}
		i++;
	}
	if ((sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &cli, sizeof(cli))) == -1) {
		err_exit("Error in sendto() for get names");
	}
}
/*
void process_leave_request(int sockfd, char* user, char* group, struct sockaddr_in sockaddr) {
	int i;
	for (i = 0; i < MAX_GROUPS; i++) {
		if (strcmp(CURRENT_CHAT_GROUPS[i].group_name, group) == 0) {
			if (remove_group_user(CURRENT_CHAT_GROUPS[i], user) == 0) {
				sendto(sockfd, "success", strlen("success"), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
				return;
			}
		}
	}
	sendto(sockfd, "failure", strlen("failure"), 0, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}
*/

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
		char* parameter = strtok(NULL, " ");

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