#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#define MESSAGE_SIZE 1024

char sentMessage[MESSAGE_SIZE];
char receivedMessage[MESSAGE_SIZE];

int userCount = 0;
int loggedInUserCount = 0;

typedef struct UserList
{
    int UserId;
    char *UserName;
    char *SecretKey;
	struct UserList *next;
}UserList;

typedef struct GroupList
{
	char *GroupName;
	int GroupMember[100];
	struct GroupList* next;
}GroupList;

struct UserList *Users = NULL;
struct GroupList *Groups = NULL;

void *ConnectionHandler(void *);
int findUserByName(char[]);
char* findUserById(int);
char* findUserSecretKey(int);
bool isUserorGroupExist(int, char[]);
void GetUsers();
char *generateRandomKey();
char *decryptMessage(int, char *, int);
char *encryptMessage(int, char *, int);
unsigned char swapNibbles(unsigned char);
char *XOR(char *, char *, int);
void CreateUserOrGroup(int, int, char[], char *);

int main()
{
	int *new_sock, pid;
	struct sockaddr_in server, client;
	int sin_size;
	int new_socket, sock;
	srand(time(NULL));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket error"); exit(1);
	}

	printf("\nServer started\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(5000);
	server.sin_addr.s_addr = INADDR_ANY;


	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) < 0) 
	{
		perror("Unable to bind"); exit(1);
	}

	listen(sock, 3);

	sin_size = sizeof(struct sockaddr_in);	
	
	while (new_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&sin_size))
	{
		if(userCount < 100)
		{
			printf("\nClient connected"); puts("");

			pthread_t sniffer_thread;
			new_sock = malloc(1);
			*new_sock = new_socket;

			if(pthread_create(&sniffer_thread, NULL,  ConnectionHandler, (void*) new_sock) < 0)
			{
			    perror("Could not create thread");
			    return 1;
			}
			userCount++;
		}
	}

	close(sock);
	return 0;
}

void *ConnectionHandler(void *socket_desc)
{
	int sender = *((int *)socket_desc); 

	while(true) 
    {		
    	int length;
        if((length = recv(sender, receivedMessage, MESSAGE_SIZE, 0)) <= 0)
        {
        	break;
        }
        receivedMessage[length] = '\0';

        if(strstr(receivedMessage, "login") == NULL)
        {
        	printf("%s ", findUserById(sender));
        	printf("sent a message as : ");
        	int i;
			for(i = 0; i < length; i++)
			{
				printf("%02X", receivedMessage[i]);
			}
			printf("\n%s", findUserById(sender));
			printf("'s message is decoded as : ");
        	strcpy(receivedMessage, decryptMessage(sender, receivedMessage, length));
        	printf("%s", receivedMessage);
        	puts("");
        }

		char *splitArray[200];
		char *split;
		split = strtok(receivedMessage, " ");

		int i = 0;
		while(split != NULL)
		{
			if(i > 2)
			{
				splitArray[i] = (char *) malloc(100);
			}
			splitArray[i] = split;
			split = strtok(NULL, " ");
			i++;
		}

		if(strcmp(splitArray[0], "login") == 0)
		{
			CreateUserOrGroup(0, sender, splitArray[1], NULL);
			send(sender, sentMessage, strlen(sentMessage), 0);
		}

		else if(strcmp(splitArray[0], "getusers") == 0)
		{
			GetUsers();
			printf("replying to %s as : %s", findUserById(sender), sentMessage);
			strcpy(sentMessage, encryptMessage(sender, sentMessage, strlen(sentMessage)));
			send(sender, sentMessage, strlen(sentMessage), 0);
		}

		else if(strcmp(splitArray[0], "alias") == 0)
		{
			if(splitArray[1] == NULL)
			{
				puts("Group name is needed");
				strcpy(sentMessage, "group name is needed");
				send(sender, sentMessage, strlen(sentMessage), 0);
			}
			else if(splitArray[2] == NULL)
			{
				puts("At least one member is needed");
				strcpy(sentMessage, "at least one member is needed");
				send(sender, sentMessage, strlen(sentMessage), 0);
			}
			else
			{
				CreateUserOrGroup(1, 0, splitArray[1], splitArray[2]);
				strcpy(sentMessage, encryptMessage(sender, sentMessage, strlen(sentMessage)));
				send(sender, sentMessage, strlen(sentMessage), 0);
			}
		}

		else if(isUserorGroupExist(0, splitArray[0]) == true && splitArray[1] != NULL)
		{
			int receiver = findUserByName(splitArray[0]);
			char* message = (char *) malloc(200);
			strcpy(message, "(");
			strcat(message, findUserById(sender));
			strcat(message, ") ");
			int j = 1;
			while(j < i)
			{
				strcat(message, splitArray[j]);
				strcat(message, " ");
				j++;
			}
			printf("replying to %s as : %s\n", findUserById(receiver), message);
			strcpy(sentMessage, encryptMessage(receiver, message, strlen(message)));
			send(receiver, sentMessage, strlen(sentMessage), 0);
		}

		else if(isUserorGroupExist(1, splitArray[0]) == true && splitArray[1] != NULL)
		{
			int receiver;
			char* message = (char *) malloc(200);
			strcpy(message, "(");
			strcat(message, findUserById(sender));
			strcat(message, ") ");
			int j = 1;
			while(j < i)
			{
				strcat(message, splitArray[j]);
				strcat(message, " ");
				j++;
			}
			strcpy(sentMessage, message);
			j = 0;

			GroupList *temp = Groups;

    		while(temp != NULL)
    		{
				if(strcmp(temp->GroupName, splitArray[0]) == 0)
				{
					while(temp->GroupMember[j] != 0)
					{
						receiver = temp->GroupMember[j];
						printf("replying to %s as : %s\n", findUserById(receiver), message);
						strcpy(sentMessage, encryptMessage(receiver, message, strlen(message)));
						send(receiver, sentMessage, strlen(sentMessage), 0);
						j++;
					}
					break;
				}
        		temp = temp->next;
    		}
		}

		fflush(stdout);
    }	

	free(socket_desc);
	return 0;
}

void CreateUserOrGroup(int type, int Id, char name[], char *members)
{
	if(isUserorGroupExist(0, name) == true)
	{
		puts("A user with same name is exists");
		strcpy(sentMessage, "a user with same name is exists");
	}

	else if(isUserorGroupExist(1, name) == true)
	{
		puts("A group with same name is exists");
		strcpy(sentMessage, "a group with same name is exists");
	}

	else
	{
		if(type == 0)
		{
			UserList *node = (UserList*)malloc(sizeof(UserList));
			node->UserId = Id;
			node->UserName = (char*) malloc(strlen(name));
			node->SecretKey = (char*) malloc(40*sizeof(unsigned char));
			strcpy(node->UserName, name);
			strcpy(node->SecretKey, generateRandomKey());
			node->next = NULL;

			if(Users == NULL)
			{
				Users = node;
			}

			else
			{
				UserList *temp = Users;
				
				while(true)
				{
					if(temp->next == NULL)
					{
						temp->next = node;
						break;
					}

					temp = temp->next;
				}
			}

			loggedInUserCount++;
			printf("\nClient logged in as %s", node->UserName);	
			printf("\nSecret key generated for %s as : %s", node->UserName, node->SecretKey);
			puts("");
			strcpy(sentMessage, node->SecretKey);
		}

		else if(type == 1)
		{
			GroupList *node = (GroupList*)malloc(sizeof(GroupList));
			node->GroupName = (char*) malloc(strlen(name));
			strcpy(node->GroupName, name);
			char* split = strtok(members, ",");
			int i = 0;
			node->next = NULL;

			while(split != NULL)
			{
				if(findUserByName(split) != 0)
				{
					node->GroupMember[i] = findUserByName(split);
					i++;
				}

				split = strtok(NULL, ",");
			}

			if(i != 0)
			{
				if(Groups == NULL)
				{
					Groups = node;
				}
				else
				{
					GroupList *temp = Groups;
					
					while(true)
					{
						if(temp->next == NULL)
						{
							temp->next = node;
							break;
						}

						temp = temp->next;
					}
				}

				printf("Group created as %s", node->GroupName);
				puts("");
				strcpy(sentMessage, "group created succesfully");
			}
		}
	}
}

int findUserByName(char name[])
{
	UserList *temp = Users;
	int userId = 0;

    while(temp != NULL)
    {
		if(strcmp(temp->UserName, name) == 0)
		{
			userId = temp->UserId;
			break;
		}
        temp = temp->next;
    }

	return userId;
}

char* findUserById(int id)
{
	UserList *temp = Users;
	char* userName;

    while(temp != NULL)
    {
		if(temp->UserId == id)
		{
			userName = (char*) malloc(strlen(temp->UserName));
			strcpy(userName, temp->UserName);
			break;
		}
        temp = temp->next;
    }

	return userName;
}

char* findUserSecretKey(int id)
{
	UserList *temp = Users;
	char* secretKey = (char*)malloc(40);

	while(temp != NULL)
	{
		if(temp->UserId == id)
		{
			secretKey = (char*) malloc(strlen(temp->SecretKey));
			strcpy(secretKey, temp->SecretKey);
			break;
		}
		temp = temp->next;
	}

	return secretKey;
}

bool isUserorGroupExist(int id, char name[])
{
	if(id == 0)
	{
    	UserList *temp = Users;
    	while(temp != NULL)
	    {
			if(strcmp(temp->UserName, name) == 0)
			{
				return true;
			}
	        temp = temp->next;
	    }	
	}
    else if(id == 1)
    {
    	GroupList *temp = Groups;
    	while(temp != NULL)
	    {
			if(strcmp(temp->GroupName, name) == 0)
			{
				return true;
			}
	        temp = temp->next;
	    }
    }

	return false;
}

void GetUsers()
{
	UserList *temp = Users;
	strcpy(sentMessage, "");
	int i = 0;

    while(temp != NULL)
    {
    	i++;
		strcat(sentMessage, temp->UserName);
		if(i != loggedInUserCount)
			strcat(sentMessage, ",");
        temp = temp->next;
    }
}

char *generateRandomKey()
{
	unsigned int randomNumber;
	int i;
	unsigned char* key = (char*) malloc(40*sizeof(unsigned char));
	unsigned char* randomCharacter = (char*) malloc(sizeof(unsigned char));
	strcpy(randomCharacter, "");
	strcpy(key, "");
	for(i = 0; i < 30; i++)
	{
		randomNumber = rand() % 94 + 33; 
		sprintf(randomCharacter, "%c", randomNumber);
		strcat(key, randomCharacter);
	}
	return key;
}

char *decryptMessage(int sender, char *message, int length)
{
	int i;
	char *secretKey = (char*)malloc(40);
	strcpy(secretKey, findUserSecretKey(sender));
	unsigned char *temp = (char*)malloc(2*sizeof(unsigned char));
	char *swappedMessage = (char*) malloc(length*2);
	char *xoredMessage = (char*) malloc(length*2);
	strcpy(swappedMessage, "");
	strcpy(xoredMessage, "");
    strcpy(xoredMessage, XOR(secretKey, message, length));

	for(i = 0; i < length; i++)
	{
        sprintf(temp, "%c", swapNibbles(xoredMessage[i]));
        strcat(swappedMessage, temp);
    }

    return swappedMessage;
}

char *encryptMessage(int receiver, char *message, int length)
{
	char *secretKey = (char*)malloc(40);
	strcpy(secretKey, findUserSecretKey(receiver));
	int i;
	char *swappedMessage = (char*)malloc(length*2);
	unsigned char *temp = (char*)malloc(2*sizeof(unsigned char));
	strcpy(swappedMessage, "");

	for(i = 0; i < length; i++)
	{
        sprintf(temp, "%c", swapNibbles(message[i]));
        strcat(swappedMessage, temp);
    }

    strcpy(swappedMessage, XOR(secretKey, swappedMessage, length));

    return swappedMessage;
}

unsigned char swapNibbles(unsigned char c)
{
     unsigned char temp1, temp2;

     temp1 = c & 0x0F;
     temp2 = c & 0xF0;
     temp1=temp1 << 4;
     temp2=temp2 >> 4;

     return(temp2|temp1); //adding the bits
}

char *XOR(char *key, char *message, int length)
{
	int i;
	int j = 0;
	char *cipheredMessage = (char*) malloc(length);

	for(i = 0; i < length; i++)
	{
		cipheredMessage[i] = key[j] ^ message[i];
		j++;
		if(j == strlen(key))
			j = 0;
	}

	return cipheredMessage;
}