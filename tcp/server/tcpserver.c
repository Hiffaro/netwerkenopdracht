#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <winsock2.h> //for all socket programming
#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
#include <stdio.h>	  //for fprintf, perror
#include <unistd.h>	  //for close
#include <stdlib.h>	  //for exit
#include <string.h>	  //for memset
#include <time.h>	  //for time( NULL )
#include <stdint.h>	  //for uintx_t variables
#include <process.h>

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
int connection(int internetSocket);
void execution(void *socket);
void closeClientSocket(int clientInternetSocket);
void cleanup(int internetSocket);

int main(int argc, char *argv[])
{

	// Initialization.
	srand(time(NULL));
	OSInit();

	// Bind to socket.
	int internetSocket = initialization();

	// Get connections and handle them.
	while (1)
	{
		int clientInternetSocket = connection(internetSocket);
		_beginthread(execution, 0, (void *)&clientInternetSocket);
	}

	// Clean up.
	cleanup(internetSocket);
	OSCleanup();
	return 0;
}

int initialization()
{
	struct addrinfo internetAddressSetup;
	struct addrinfo *internetAddressResult;
	memset(&internetAddressSetup, 0, sizeof(internetAddressSetup));
	internetAddressSetup.ai_family = AF_UNSPEC;
	internetAddressSetup.ai_socktype = SOCK_STREAM;
	internetAddressSetup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo(NULL, "5555", &internetAddressSetup, &internetAddressResult);
	if (getaddrinfo_return != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
		exit(-1);
	}

	int internetSocket = -1;
	struct addrinfo *internetAddressResultIterator = internetAddressResult;
	while (internetAddressResultIterator != NULL)
	{
		internetSocket = socket(internetAddressResultIterator->ai_family, internetAddressResultIterator->ai_socktype, internetAddressResultIterator->ai_protocol);
		if (internetSocket == -1)
		{
			perror("socket");
			exit(-1);
		}
		else
		{
			int bind_return = bind(internetSocket, internetAddressResultIterator->ai_addr, internetAddressResultIterator->ai_addrlen);
			if (bind_return == -1)
			{
				perror("bind");
				close(internetSocket);
				exit(-1);
			}
			else
			{
				int listen_return = listen(internetSocket, 1);
				if (listen_return == -1)
				{
					close(internetSocket);
					perror("listen");
					exit(-1);
				}
				else
				{
					break;
				}
			}
		}
		internetAddressResultIterator = internetAddressResultIterator->ai_next;
	}

	freeaddrinfo(internetAddressResult);

	if (internetSocket == -1)
	{
		fprintf(stderr, "socket: no valid socket address found\n");
		exit(-1);
	}

	return internetSocket;
}

int connection(int internetSocket)
{
	struct sockaddr_storage clientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof clientInternetAddress;
	int clientSocket = accept(internetSocket, (struct sockaddr *)&clientInternetAddress, &clientInternetAddressLength);
	if (clientSocket == -1)
	{
		perror("accept");
		close(internetSocket);
		exit(-1);
	}
	return clientSocket;
}

void execution(void *socket)
{
	int internetSocket = *(int *)socket;
	int numberOfBytesReceived = 0;
	int guess = 0;
	int toGuess = rand() % 1000000;
	printf("toGuess: %i\n", toGuess);
	uint8_t buffer[1000];

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		numberOfBytesReceived = recv(internetSocket, buffer, sizeof(buffer) - 1, 0);
		if (numberOfBytesReceived == -1)
		{
			perror("recv");
			break;
		}
		else
		{
			guess = *(uint32_t *)buffer;
			guess = ntohl(guess);
			printf("guess: %i\n", guess);

			memset(buffer, 0, sizeof(buffer));
			if (guess > toGuess)
			{
				strcpy(buffer, "Lager");
			}
			else if (guess < toGuess)
			{
				strcpy(buffer, "Hoger");
			}
			else if (guess == toGuess)
			{
				strcpy(buffer, "Correct");
			}

			// send response
			int number_of_bytes_send = 0;
			number_of_bytes_send = send(internetSocket, buffer, strlen(buffer), 0);
			if (number_of_bytes_send == -1)
			{
				perror("send");
				break;
			}

			if(guess == toGuess) break;
		}
	}

	closeClientSocket(internetSocket);
}

void closeClientSocket(int clientInternetSocket)
{
	int shutdownReturn = shutdown(clientInternetSocket, SD_RECEIVE);
	if (shutdownReturn == -1)
	{
		perror("shutdown");
	}

	close(clientInternetSocket);
}

void cleanup(int internetSocket)
{
	close(internetSocket);
}
