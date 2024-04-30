#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <winsock2.h> //for all socket programming
#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
#include <stdio.h>	  //for fprintf, perror
#include <unistd.h>	  //for close
#include <stdlib.h>	  //for exit
#include <string.h>	  //for memset
void OSInit(void)
{
	WSADATA wsaData;
	int WSAError = WSAStartup(MAKEWORD(2, 0), &wsaData);
	if (WSAError != 0)
	{
		fprintf(stderr, "WSAStartup errno = %d\n", WSAError);
		exit(-1);
	}
}
void OSCleanup(void)
{
	WSACleanup();
}
#define perror(string) fprintf(stderr, string ": WSA errno = %d\n", WSAGetLastError())
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
void OSInit(void) {}
void OSCleanup(void) {}
#endif

int initialization();
int execution(int internet_socket);
void cleanup(int internet_socket);

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("You must supply the remote IP address to this program.\n");
		return 1;
	}
	OSInit();
	while (1)
	{
		int internet_socket = initialization(argv[1]);
		int should_continue = execution(internet_socket);
		cleanup(internet_socket);
		if (!should_continue)
			break;
	}
	OSCleanup();
	return 0;
}

int initialization(const char *ip)
{
	// Get local address info.
	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address_result;
	memset(&internet_address_setup, 0, sizeof internet_address_setup);
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	int getaddrinfo_return = getaddrinfo(ip, "5555", &internet_address_setup, &internet_address_result);
	if (getaddrinfo_return != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
		exit(1);
	}

	// Bind to socket.
	int internet_socket = -1;
	struct addrinfo *internet_address_result_iterator = internet_address_result;
	while (internet_address_result_iterator != NULL)
	{
		internet_socket = socket(internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol);
		if (internet_socket == -1)
		{
			perror("socket");
		}
		else
		{
			int connect_return = connect(internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen);
			if (connect_return == -1)
			{
				perror("connect");
				close(internet_socket);
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	// Clean up addrinfo.
	freeaddrinfo(internet_address_result);

	// Did we fail?
	if (internet_socket == -1)
	{
		fprintf(stderr, "socket: no valid socket address found\n");
		exit(2);
	}

	// Return the found socket.
	return internet_socket;
}

int execution(int internet_socket)
{
	int ai_mode = 0;
	int lower_bound = 0;
	int higher_bound = 1000000;
	char buffer[1000];
	while (1)
	{
		// Get guess.
		int guess = -1;
		if (!ai_mode)
		{
			while (guess < 0 || guess > 1000000)
			{
				printf("Pick a number between 0 and 1 million, or type ai to let me guess: ");
				scanf("%s", buffer);
				if (strcmp(buffer, "ai") == 0)
				{
					ai_mode = 1;
					guess = (lower_bound + higher_bound) / 2;
					printf("Guessing %d...\n", guess);
					break;
				}
				guess = atoi(buffer);
			}
		}
		else
		{
			guess = (lower_bound + higher_bound) / 2;
			printf("Guessing %d...\n", guess);
		}

		// Send guess.
		int to_send = ntohl(guess);
		int sent = send(internet_socket, (char *)&to_send, sizeof(to_send), 0);
		if (sent == -1)
		{
			perror("send");
			return 0;
		}

		// Get answer.
		memset(buffer, 0, sizeof(buffer));
		int received = recv(internet_socket, buffer, sizeof(buffer) - 1, 0);
		if (received == -1)
		{
			perror("recv");
			return 0;
		}
		else
		{
			if (strcmp(buffer, "Hoger") == 0 || strcmp(buffer, "Higher") == 0)
			{
				printf("Hoger!\n");
				lower_bound = guess;
			}
			else if (strcmp(buffer, "Lager") == 0 || strcmp(buffer, "Lower") == 0)
			{
				printf("Lager!\n");
				higher_bound = guess;
			}
			else if (strcmp(buffer, "Correct") == 0)
			{
				printf("Correct!\n");
				printf("Type q to stop playing or anything else to continue: ");
				scanf("%s", buffer);
				return strcmp(buffer, "q");
			}
			else
			{
				printf("Server sent an invalid response: %s", buffer);
			}
		}
	}
}

void cleanup(int internet_socket)
{
	// Shut down and close socket.
	int shutdown_return = shutdown(internet_socket, SD_SEND);
	if (shutdown_return == -1)
	{
		perror("shutdown");
	}
	close(internet_socket);
}
