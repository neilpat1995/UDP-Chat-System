# UDP-Chat-System
This project implements a UDP-based multi-user chat system, consisting of the following features:
1. Creating groups in which to chat.
2. Joining existing groups to chat.
3. Retrieving information about all existing groups.
4. Adding friends.
5. Viewing all friends.
6. Searching for friends (i.e. the groups they are in).

It is composed of two parts:
1. A server (server.c) that maintains information about each group, such as the current user list.
2. A client that can be used to perform the above functionality.

Intra-group communication is achieved through multicasting, by assigning a unique multicast IP address to each group (with the port kept the same). 

The system can be run as follows:
1. Clone the repository: `git clone https://github.com/neilpat1995/UDP-Chat-System.git`
2. Change directories to the project directory: `cd UDP-Chat-System`
3. Compile the client: `gcc -o client client.c -pthread` and the server: `gcc -o server server.c -pthread`
4. Run the server: `./server` and the client: `./client <username>`
5. Interfacing on the client-side, you will see a menu of commands you can use to perform the above functionality of the system. For example, run "create group1", create a new client and run "join group1", then write messages between the two users and you should see the messages appear on your terminal.
