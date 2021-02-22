#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
//Global Variables
#define MAX_CLIENTS 30
#define MAX_GROUP_CAP 10
#define MAX_GROUPS 10
#define MAX_BUFFER_SIZE 2048
static int clientCounter = 0;
static int uid = 10;
//Main two structures defining
typedef struct
{
	struct sockaddr_in address;
	int socket;
	int uid;
	char number[32];
} clientStruct;
typedef struct
{
	clientStruct *clientsInGroup[MAX_GROUP_CAP];
	char name[10];
	char password[10];
} groupStruct;
//init structs
clientStruct *clients[MAX_CLIENTS];
groupStruct *groups[MAX_GROUPS];
//Add client to server(adding into the struct array)
void insertClientServer(clientStruct *cl)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}
}
//create room(adding into the struct array)
void gcreateGroup(groupStruct *group)
{
	for (int i = 0; i < MAX_GROUPS; ++i)
	{
		if (!groups[i])
		{
			groups[i] = group;
			break;
		}
	}
}
//insert client to the group
void joinClientGroup(clientStruct *cl, int index)
{
	if (groups[index])
	{
		for (int i = 0; i < MAX_GROUP_CAP; i++)
		{
			if (!groups[index]->clientsInGroup[i])
			{
				groups[index]->clientsInGroup[i] = cl;
				break;
			}
		}
	}
}
//if user -exit groupname then remove from the group
void removeClientGroup(int uid, int index)
{
	int clientCnt = 0;
	for (int i = 0; i < MAX_GROUP_CAP; i++)
	{
		if (groups[index]->clientsInGroup[i])
		{
			if (groups[index]->clientsInGroup[i]->uid == uid)
			{
				groups[index]->clientsInGroup[i] = NULL;
				break;
			}
		}
	}
	//check how many user available in the group
	for (int i = 0; i < MAX_GROUP_CAP; i++)
	{
		if (groups[index]->clientsInGroup[i])
		{
			clientCnt++;
		}
	}
	// if group is empty then remove the group automatically
	if (clientCnt == 0)
	{
		groups[index] = NULL;
	}
}
//find id by using client id who is inside
int findGroupIndexByClientId(int uid)
{
	int index = -1;
	for (int i = 0; i < MAX_GROUPS; i++)
	{
		if (groups[i])
		{
			for (int j = 0; j < MAX_GROUP_CAP; j++)
			{
				if (groups[i]->clientsInGroup[j])
				{
					//if user inside that group
					if (groups[i]->clientsInGroup[j]->uid == uid)
					{
						index = i;
						break;
					}
				}
			}
		}
	}
	return index;
}
//send message to the other clients
void sendMsgOthers(char *s, int uid)
{
	int index = findGroupIndexByClientId(uid);
	for (int i = 0; i < MAX_GROUP_CAP; i++)
	{
		if (groups[index]->clientsInGroup[i])
		{
			if (groups[index]->clientsInGroup[i]->uid != uid)
			{
				if (write(groups[index]->clientsInGroup[i]->socket, s, strlen(s)) < 0)
				{
					break;
				}
			}
		}
	}
}
//find index of the group by using group name
int findGroupIndexByName(char *name)
{
	int index = -1;
	for (int i = 0; i < MAX_GROUPS; i++)
	{
		if (groups[i])
		{
			if (groups[i]->name)
			{
				if (strcmp(groups[i]->name, name) == 0)
				{
					index = i;
					break;
				}
			}
		}
	}
	return index;
}
//-join command for private chat with user find user id with user number
int findClientIndexByNumber(char *number)
{
	int index = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i])
		{
			if (clients[i]->number)
			{
				if (strcmp(clients[i]->number, number) == 0)
				{
					index = i;
					break;
				}
			}
		}
	}
	return index;
}

//send message to main clients screen
void sendMsg(char *s, int uid)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{
				if (send(clients[i]->socket, s, strlen(s), 0) < 0)
				{
					break;
				}
			}
		}
	}
}
//basically main part for server c. It includes main if else statements for program
void *handle_client(void *arg)
{
	//main variables for strtok,buffer,msg and structs
	char outputFromClient[MAX_BUFFER_SIZE];
	char password[MAX_BUFFER_SIZE];
	char number[32];
	char message[MAX_BUFFER_SIZE];
	char receivedAnything[MAX_BUFFER_SIZE];
	int controlflag = 0;
	clientCounter++;
	clientStruct *cli = (clientStruct *)arg;
	groupStruct *group = (groupStruct *)arg;
	char *argument;
	char *arguments[10];
	int argumentIndex;
	int groupIndex;
	//is number entered and received ? if its not received then error
	if (recv(cli->socket, number, 32, 0) <= 0)
	{
		printf("Didn't enter the number.\n");
		controlflag = 1;
	}
	else //received number
	{
		strcpy(cli->number, number);
		sprintf(outputFromClient, "%s connected.\n", cli->number);
		printf("%s", outputFromClient);
	}
	bzero(outputFromClient, MAX_BUFFER_SIZE); // clear buffer resetting
	//infinite loop until user -exit
	while (1)
	{
		if (controlflag)
		{
			break;
		}
		int receive = recv(cli->socket, outputFromClient, MAX_BUFFER_SIZE, 0);
		strcpy(receivedAnything, outputFromClient);
		if (receive > 0)
		{
			if (strlen(outputFromClient) > 0)
			{
				if (strncmp(outputFromClient, "-", 1) == 0)
				{
					argumentIndex = 0;
					argument = strtok(outputFromClient, " ");
					//take arguments until \n which means user finished input entering state
					while (argument != NULL)
					{
						arguments[argumentIndex] = argument;
						argumentIndex++;
						argument = strtok(NULL, "\n");
					}
					// group creation
					if (strcmp(arguments[0], "-gcreate") == 0 && arguments[1] != NULL)
					{
						char msg3[MAX_BUFFER_SIZE];
						strcpy(msg3, "Please create password : ");
						sendMsg(msg3, cli->uid);
						int receivePassword = recv(cli->socket, password, MAX_BUFFER_SIZE, 0);
						if (receivePassword > 0)
						{
							if (strlen(password) > 0)
							{
								groupStruct *privateGroup = (groupStruct *)malloc(sizeof(groupStruct));
								strcpy(privateGroup->name, arguments[1]);
								strcpy(privateGroup->password, password);
								gcreateGroup(privateGroup);
								groupIndex = findGroupIndexByName(privateGroup->name);
								joinClientGroup(cli, groupIndex);
								char infoMsg[MAX_BUFFER_SIZE];
								strcpy(infoMsg, "Welcome to ");
								strcat(infoMsg, privateGroup->name);
								strcat(infoMsg, "\n");
								sendMsg(infoMsg, cli->uid);
								bzero(password, MAX_BUFFER_SIZE);
							}
							else
							{
								char pswMsg[MAX_BUFFER_SIZE];
								strcpy(pswMsg, "Password length must be greater than 0 !!\n");
								sendMsg(pswMsg, cli->uid);
							}
						}
					}
					//sending message part
					else if (strcmp(arguments[0], "-send") == 0 && arguments[1] != NULL )
					{
						char msg[MAX_BUFFER_SIZE];
						strcpy(msg, "\t\t");
						strcat(msg, cli->number);
						strcat(msg, " : ");
						strcat(msg, arguments[1]);
						strcat(msg, "\n");
						sendMsgOthers(msg, cli->uid);
					}
					//left room part with controlling name
					else if (strcmp(arguments[0], "-exit") == 0 && arguments[1] != NULL)
					{
						groupIndex = findGroupIndexByClientId(cli->uid);
						char infoMsg[MAX_BUFFER_SIZE];
						strcpy(infoMsg, "You left chat\n");
						sendMsg(infoMsg, cli->uid);
						bzero(infoMsg, MAX_BUFFER_SIZE);
						strcpy(infoMsg, "\t\t");
						strcat(infoMsg, cli->number);
						strcat(infoMsg, " left from chat");
						strcat(infoMsg, "\n");
						sendMsgOthers(infoMsg, cli->uid);
						removeClientGroup(cli->uid, groupIndex);
					}
					//join the existing room
					else if (strcmp(arguments[0], "-join") == 0 && arguments[1] != NULL )
					{
						//enter_group(cli,arguments[1]);
						char msg3[MAX_BUFFER_SIZE];
						//if group exist
						if (findGroupIndexByName(arguments[1]) != -1)
						{
							//char password[MAX_BUFFER_SIZE];
							strcpy(msg3, "Enter group's password : ");
							sendMsg(msg3, cli->uid);
							int receivePassword = recv(cli->socket, password, MAX_BUFFER_SIZE, 0);
							//if entered pwd is matched
							if (strcmp(groups[findGroupIndexByName(arguments[1])]->password, password) == 0)
							{
								joinClientGroup(cli, findGroupIndexByName(arguments[1]));
								strcpy(msg3, "Welcome to ");
								strcat(msg3, groups[findGroupIndexByName(arguments[1])]->name);
								strcat(msg3, "\n");
								sendMsg(msg3, cli->uid);
								bzero(msg3, MAX_BUFFER_SIZE);
								strcpy(msg3, "\t\t");
								strcat(msg3, cli->number);
								strcat(msg3, " joined group\n");
								sendMsgOthers(msg3, cli->uid);
								bzero(password, MAX_BUFFER_SIZE);
							}

							//if pwd didnt match, reset buffers
							else
							{
								bzero(msg3, MAX_BUFFER_SIZE);
								bzero(password, MAX_BUFFER_SIZE);
								strcpy(msg3, "Password is incorrect.\n");
								sendMsg(msg3, cli->uid);
							}
						}
						//if private chat is available with username
						else if (findClientIndexByNumber(arguments[1]) != -1)
						{
							char msg3[MAX_BUFFER_SIZE];
							strcpy(msg3, "Please create password : ");
							sendMsg(msg3, cli->uid);
							int receivePassword = recv(cli->socket, password, MAX_BUFFER_SIZE, 0);
							if (receivePassword > 0)
							{
								if (strlen(password) > 0)
								{
									groupStruct *privateGroup = (groupStruct *)malloc(sizeof(groupStruct));
									strcpy(privateGroup->name, arguments[1]);
									strcpy(privateGroup->password, password);
									gcreateGroup(privateGroup);
									groupIndex = findGroupIndexByName(privateGroup->name);
									joinClientGroup(cli, groupIndex);
									char infoMsg[MAX_BUFFER_SIZE];
									strcpy(infoMsg, "Private chat with  ");
									strcat(infoMsg, privateGroup->name);
									strcat(infoMsg, "\n");
									sendMsg(infoMsg, cli->uid);
									bzero(password, MAX_BUFFER_SIZE);
								}
								else
								{
									char pswMsg[MAX_BUFFER_SIZE];
									strcpy(pswMsg, "Password length must be greater than 0 !!\n");
									sendMsg(pswMsg, cli->uid);
								}
							}
						}

						//if group is not exist
						else
						{
							strcpy(msg3, "There is no group or user with this name!\n");
							sendMsg(msg3, cli->uid);
						}
					}
					//print user's phone number as a info
					else if (strcmp(arguments[0], "-whoami") == 0)
					{
						char msg2[MAX_BUFFER_SIZE];
						strcpy(msg2, "Your number  : ");
						strcat(msg2, cli->number);
						strcat(msg2, "\n");
						sendMsg(msg2, cli->uid);
					}
				}
			}
		}
		//-exit statement.if receive is 0 which means we sent -exit from client but we checked it on client side line 44
		else if (receive == 0)
		{
			char msg[MAX_BUFFER_SIZE];
			strcpy(msg, cli->number);
			strcat(msg, " has left\n");
			printf("%s", msg);
			controlflag = 1;
		}
		bzero(outputFromClient, MAX_BUFFER_SIZE);
	}
	close(cli->socket);
	//remove client from array
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == cli->uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}
	free(cli);
	clientCounter--;
	return NULL;
}
int main(int argc, char **argv)
{
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv;
	struct sockaddr_in cli_addr;
	pthread_t servertid;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = INADDR_ANY;
	serv.sin_port = htons(3205);
	if (bind(listenfd, (struct sockaddr *)&serv, sizeof(serv)) < 0)
	{
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}
	if (listen(listenfd, 10) < 0)
	{
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}
	printf("Whatsapp Server Online\n");
	//main while from lab
	while (1)
	{
		char outputFromClient[MAX_BUFFER_SIZE];
		char name[32];
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);
		// Client init
		clientStruct *cli = (clientStruct *)malloc(sizeof(clientStruct));
		cli->address = cli_addr;
		cli->socket = connfd;
		cli->uid = uid++;
		insertClientServer(cli);
		pthread_create(&servertid, NULL, &handle_client, (void *)cli);
		sleep(1);
	}
	return 1;
}