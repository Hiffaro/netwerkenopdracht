#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7 // select minimal legacy support, needed for inet_pton, inet_ntop
#include <winsock2.h>				   //for all socket programming
#include <ws2tcpip.h>				   //for getaddrinfo, inet_pton, inet_ntop
#include <stdio.h>					   //for fprintf
#include <unistd.h>					   //for close
#include <stdlib.h>					   //for exit
#include <string.h>					   //for memset
#include <stdint.h>
#include <time.h>
#include <conio.h>

void setTimeOut(uint32_t timeOutInMilis, uint32_t socket)
{
	DWORD timeOut = timeOutInMilis;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeOut, sizeof(timeOut));
}
#else
#include <sys/socket.h> //for sockaddr, socket, socket
#include <sys/types.h>	//for size_t
#include <netdb.h>		//for getaddrinfo
#include <netinet/in.h> //for sockaddr_in
#include <arpa/inet.h>	//for htons, htonl, inet_pton, inet_ntop
#include <errno.h>		//for errno
#include <stdio.h>		//for fprintf, perror
#include <unistd.h>		//for close
#include <stdlib.h>		//for exit
#include <string.h>		//for memset
#endif

int guessing(int *stop, int internet_socket, struct addrinfo *internet_address);
void receiving(int internet_socket, char *buffer);
void read_key_into_buffer(char *buffer, int size, int *offset);
void clear_buffer(char *buffer, int size, int *offset);
int is_buffer_ready(char *buffer);

int main(int argc, char *argv[])
{
	// Input buffer.
	int offset = 0;
	char buffer[65535];
	clear_buffer(buffer, sizeof(buffer), &offset);

	// Socket setup.
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);
	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address = NULL;
	memset(&internet_address_setup, 0, sizeof internet_address_setup);
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	getaddrinfo("127.0.0.1", "5555", &internet_address_setup, &internet_address);
	int internet_socket;
	internet_socket = socket(internet_address->ai_family, internet_address->ai_socktype, internet_address->ai_protocol);

	// Execution.
	int in_game = 0;
	int awaiting_input = 0;
	while (1)
	{
		read_key_into_buffer(buffer, sizeof(buffer), &offset);
		if(!awaiting_input)
		{
			printf("Make a guess between 0 and 99: ");
			awaiting_input = 1;
		}
		else if(is_buffer_ready(buffer))
		{
			// parse buffer here.
		}

/*
		setTimeOut(500, internet_socket);
		do
		{

			if (guessing(&stop, internet_socket, internet_address))
			{
				continue;
			}
			if (startgame)
			{
				startgame = 0;
				starttimer = time(NULL);
			}
			memset(buffer, '\0', 100);

			receiving(internet_socket, buffer);

		} while ((time(NULL) - starttimer) < 16);
		startgame = 1;
		if (buffer[0] == '\0')
		{
			printf("you lost?\n");
		}
		printf("want to play a other game type \"continue\" otherwise type \"stop\"\n");

		while (stop != 1)
		{
			char input[20];
			scanf("%s", input);

			if (strcmp(input, "stop") == 0)
			{
				stop = 1;
				continue;
			}
			else if (strcmp(input, "continue") == 0)
			{
				break;
			}
			printf("type \"stop\" or \"continue\"\n");
		}
*/
	}


	// Clean up//
	freeaddrinfo(internet_address);
	close(internet_socket);
	WSACleanup();

	return 0;
}

int guessing(int *stop, int internet_socket, struct addrinfo *internet_address)
{

	char input[20];
	int value = 0;
	printf("type in a number between 0 and 99 \n ");
	scanf("%s", input);

	if (strcmp(input, "stop") == 0)
	{
		*stop = 0;
		return 1;
	}
	value = atoi(input);
	if (value >= 0 && value < 100)
	{

		sendto(internet_socket, input, strlen(input), 0, internet_address->ai_addr, internet_address->ai_addrlen);
	}
	else
	{
		printf("pick a number between 0 and 99 or type stop to end the game \n");
		return 1;
	}
	return 0;
}

void receiving(int internet_socket, char *buffer)
{

	struct sockaddr_storage clientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof(clientInternetAddress);
	int numberOfBytesReceived = recvfrom(internet_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientInternetAddress, &clientInternetAddressLength);
	if (numberOfBytesReceived != -1)
	{
		buffer[numberOfBytesReceived] = '\0';
		printf("Received : \"%s\"\n", buffer);
		printf("error");
	}
}

void read_key_into_buffer(char *buffer, int size, int *offset)
{
	if(_kbhit()) 
	{
		buffer[*offset] = _getch();
		printf("%c", buffer[*offset]);
		*offset++;
		if(*offset == size-1)
		{
			buffer[*offset] = '\n';
		}
	}
}

void clear_buffer(char *buffer, int size, int *offset)
{
	memset(buffer, 0, size);
	*offset = 0;
}

int is_buffer_ready(char *buffer)
{
	return strchr(buffer, '\n') != NULL;
}