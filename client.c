#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#define MESSAGE_SIZE 1024

char sentMessage[MESSAGE_SIZE];
char receivedMessage[MESSAGE_SIZE];
char* SecretKey;
int isLoggedIn;

void *GetMessage(void *);
char *encryptMessage(char *, int);
char *decryptMessage(char *, int);
unsigned char swapNibbles(unsigned char);
char *XOR(char *, char *, int);

int main()
{
	int sock, *new_sock;
	struct hostent *host;
	struct sockaddr_in server_addr;
	host = gethostbyname("127.0.0.1");

	isLoggedIn = 0;
	SecretKey = (char*) malloc(40);
	strcpy(SecretKey, "");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
		perror("Socket error");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5000);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("Connect"); exit(1);
	}

	else
	{
		printf("Connected to server");
		puts("");
	}

	pthread_t sniffer_thread;
    new_sock = malloc(1);
    *new_sock = sock;

    if(pthread_create(&sniffer_thread, NULL, GetMessage, (void *) new_sock) < 0)
    {
        perror("could not create thread");
        return 1;
    }

	while(true)
	{
		gets(sentMessage);

		if (strcmp(sentMessage, "q") == 0 || strcmp(sentMessage, "Q") == 0)
        {
			send(sock, sentMessage, strlen(sentMessage), 0);
			break;
       	}
       	else
       	{
       		if(strstr(sentMessage, "login") != NULL)
			{
				if(isLoggedIn == 0)
				{
					send(sock, sentMessage, strlen(sentMessage), 0);
				}

				else if(isLoggedIn == 1)
				{
					puts("server's reply : You are already logged in.");
				}
			}
			else if(strstr(sentMessage, "login") == NULL)
			{
				if(isLoggedIn == 0)
				{
					puts("server's reply : You need to log in first.");
				}

				else if(isLoggedIn == 1)
				{
					strcpy(sentMessage, encryptMessage(sentMessage, strlen(sentMessage)));
					send(sock, sentMessage, strlen(sentMessage), 0);
				}
			}
       	}
	}	
    close(sock);
	return 0;
}

void *GetMessage(void *socket_desc)
{
	int sock = *((int *)socket_desc); 
	int bytes_recieved;
	SecretKey = (char *) malloc(40);

	while(true)
	{
		if((bytes_recieved = recv(sock, receivedMessage, MESSAGE_SIZE, 0)) < 0)
		{
			return 0;
		}
		receivedMessage[bytes_recieved] = '\0';
		
		if(strstr(sentMessage, "login") != NULL)
		{
			if(strcmp(receivedMessage, "a user with same name is exists") == 0 || strcmp(receivedMessage, "a group with same name is exists") == 0)
			{
				printf("server's reply : ");
				puts(receivedMessage);
			}
			else
			{
				isLoggedIn = 1;
				strcpy(SecretKey, receivedMessage);
				puts("server's reply : login successful");
				strcpy(sentMessage, "");
			}
		}
			
		else if(strstr(sentMessage, "login") == NULL)
		{
			char* message = (char*) malloc(bytes_recieved);
			printf("server's reply : ");
			int i;
			for(i = 0; i < strlen(receivedMessage); i++)
			{
				printf("%02X", receivedMessage[i]);
			}

			strcpy(message, decryptMessage(receivedMessage, bytes_recieved));
			printf("\nserver's reply decoded as : %s", message);
			puts("");
		}			
	}
	
    free(socket_desc);
	return 0;
}

char *decryptMessage(char *message, int length)
{
	int i;
	unsigned char *temp = (char*)malloc(2*sizeof(unsigned char));
	char *swappedMessage = (char*) malloc(length*2);
	char *xoredMessage = (char*) malloc(length*2);
	strcpy(swappedMessage, "");
	strcpy(xoredMessage, "");
    strcpy(xoredMessage, XOR(SecretKey, message, length));

	for(i = 0; i < length; i++)
	{
        sprintf(temp, "%c", swapNibbles(xoredMessage[i]));
        strcat(swappedMessage, temp);
    }

    return swappedMessage;
}

char *encryptMessage(char *message, int length)
{
	int i;
	char *swappedMessage = (char*)malloc(length*2);
	unsigned char *temp = (char*)malloc(2*sizeof(unsigned char));
	strcpy(swappedMessage, "");

	for(i = 0; i < length; i++)
	{
        sprintf(temp, "%c", swapNibbles(message[i]));
        strcat(swappedMessage, temp);
    }

    strcpy(swappedMessage, XOR(SecretKey, swappedMessage, length));

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
		{
			j = 0;
		}
	}

	return cipheredMessage;
}