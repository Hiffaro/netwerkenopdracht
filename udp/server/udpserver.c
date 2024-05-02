// includes
#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unistd.h>
#include <time.h>

// windows needs wsaData functionality

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

void setTimeOut(uint32_t timeOutInMilis, uint32_t socket)
{
	DWORD timeOut = timeOutInMilis;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeOut, sizeof(timeOut));
}

#define perror(string) fprintf(stderr, string ": WSA errno = %d\n", WSAGetLastError())
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int OSInit(void) {}
int OSCleanup(void) {}

void setTimeOut(uint32_t timeOutInMilis, uint32_t socket)
{
	struct timeval tv;

	uint32_t sec = timoutInMilis / 1000;
	uint32_t usec = (timeOutInMilis % 1000) * 1000;

	tv.tv_sec = sec;
	tv.tv_usec = usec;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}
#endif

// defines
#define BUFFERSIZE 100

// enums
enum states
{
	idle,
	running,
	overtime
};

// function prototypes
int initialization();
void execution(int internetSocket);
void cleanup(int internetSocket);
int available(int sock);

// main
int main(int argc, char *argv[])
{
	// init
	srand(time(NULL));
	OSInit();
	int internetSocket = initialization();

	// execution
	execution(internetSocket);

	// closing
	cleanup(internetSocket);
	OSCleanup();
	return 0;
}

int initialization()
{
	// variables
	struct addrinfo internetAddressSetup;
	struct addrinfo *internetAddressResult;
	memset(&internetAddressSetup, 0, sizeof internetAddressSetup);
	internetAddressSetup.ai_family = AF_UNSPEC;
	internetAddressSetup.ai_socktype = SOCK_DGRAM;
	internetAddressSetup.ai_flags = AI_PASSIVE;
	int getaddrinfoReturn = getaddrinfo(NULL, "5555", &internetAddressSetup, &internetAddressResult);
	// if something went wrong with the getaddrinfo function exit
	if (getaddrinfoReturn != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfoReturn));
		exit(1);
	}
	int internetSocket = -1;
	struct addrinfo *internetAddressResultIterator = internetAddressResult;

	// get valid internetSocket
	while (internetAddressResultIterator != NULL)
	{
		internetSocket = socket(internetAddressResultIterator->ai_family, internetAddressResultIterator->ai_socktype, internetAddressResultIterator->ai_protocol);
		if (internetSocket == -1)
		{
			perror("socket");
		}
		else
		{
			int bindReturn = bind(internetSocket, internetAddressResultIterator->ai_addr, internetAddressResultIterator->ai_addrlen);
			if (bindReturn == -1)
			{
				close(internetSocket);
				perror("bind");
			}
			else
			{
				break;
			}
		}
		internetAddressResultIterator = internetAddressResultIterator->ai_next;
	}

	// free addrinfo before exiting
	freeaddrinfo(internetAddressResult);

	// exit if no valid socket was found
	if (internetSocket == -1)
	{
		fprintf(stderr, "socket: no valid socket address found\n");
		exit(2);
	}

	// return internetsocket that is being used by the server from now on
	return internetSocket;
}

void execution(int internetSocket)
{
	int toGuess = 0;
	int winningGuess = 0;
	int guess = 0;
	double timeout = 8.0;
	clock_t timeZero = 0;
	char buffer[BUFFERSIZE];
	char outbuf[BUFFERSIZE];
	struct sockaddr_storage clientInternetAddress;
	struct sockaddr_storage winningClientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof(clientInternetAddress);
	socklen_t winningClientInternetAddressLength = sizeof(winningClientInternetAddress);
	enum states state = idle;

	// Open logfile.
	FILE* logfile = fopen("udpserver.log", "w");
	if(logfile == NULL)
	{
		printf("Could not open log file for writing.\n");
		return;
	}

	while (1)
	{
		// Clean data.
		memset(buffer, 0, sizeof(buffer));
		memset(outbuf, 0, sizeof(outbuf));

		// Do we have data waiting?
		if (available(internetSocket))
		{
			// Get it.
			int recv = recvfrom(internetSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientInternetAddress, &clientInternetAddressLength);
			if (recv == -1)
				continue;
			buffer[recv] = '\0';
			printf("Received: \"%s\"\n", buffer);
			fprintf(logfile, "Received: \"%s\"\n", buffer);
			guess = atoi(buffer);

			// Depending on state...
			if (state == idle)
			{
				printf("Handling data in idle state...\n");
				fprintf(logfile, "Handling data in idle state...\n");
				// Make a new random number.
				toGuess = rand() % 100;

				// They're automatically the winner.
				printf("New best guess!\n");
				fprintf(logfile, "New best guess!\n");
				strcpy(outbuf, "You Won ?");
				int sent = sendto(internetSocket, outbuf, strlen(outbuf), 0, (struct sockaddr *)&clientInternetAddress, clientInternetAddressLength);
				if (sent == -1)
					continue;
				winningGuess = guess;
				winningClientInternetAddress = clientInternetAddress;
				winningClientInternetAddressLength = clientInternetAddressLength;

				// Start the timer.
				timeZero = clock();
				timeout = 8.0;

				// Switch state.
				printf("Transitioning to running state...\n");
				fprintf(logfile, "Transitioning to running state...\n");
				state = running;
			}
			else if(state == running)
			{
				printf("Handling data in running state...\n");
				fprintf(logfile, "Handling data in running state...\n");

				// Reset and halve the timer.
				timeZero = clock();
				timeout /= 2.0;

				// Is the guess closer?
				if(abs(guess - toGuess) < abs(winningGuess - toGuess))
				{
					printf("New best guess!\n");
					fprintf(logfile, "New best guess!\n");
					strcpy(outbuf, "You Won ?");
					int sent = sendto(internetSocket, outbuf, strlen(outbuf), 0, (struct sockaddr *)&clientInternetAddress, clientInternetAddressLength);
					if (sent == -1)
						continue;
					winningGuess = guess;
					winningClientInternetAddress = clientInternetAddress;
					winningClientInternetAddressLength = clientInternetAddressLength;
				}
			}
			else if(state == overtime)
			{
				printf("Handling data in overtime state...\n");
				fprintf(logfile, "Handling data in overtime state...\n");
				// Doesn't matter what they said, it's a loss.
				strcpy(outbuf, "You Lost !");
				int sent = sendto(internetSocket, outbuf, strlen(outbuf), 0, (struct sockaddr *)&clientInternetAddress, clientInternetAddressLength);
				if (sent == -1)
					continue;
			}
		}

		clock_t now = clock();
		if((now - timeZero) / CLOCKS_PER_SEC > timeout && state != idle)
		{
			if(state == overtime)
			{
				printf("Transitioning to idle state...\n");
				fprintf(logfile, "Transitioning to idle state...\n");
				state = idle;
			}
			else if(state == running)
			{
				printf("Running state timeout. Announcing winner...\n");
				fprintf(logfile, "Running state timeout. Announcing winner...\n");
				memset(outbuf, 0, sizeof(outbuf));
				strcpy(outbuf, "You Won !");
				int sent = sendto(internetSocket, outbuf, strlen(outbuf), 0, (struct sockaddr *)&winningClientInternetAddress, winningClientInternetAddressLength);
				printf("Transitioning to overtime state...\n");
				fprintf(logfile, "Transitioning to overtime state...\n");
				state = overtime;
				timeout = 16.0;
				timeZero = clock();
			}
		}
	}

	fclose(logfile);
}

// standart cleanup function to close the internet socket provided
void cleanup(int internetSocket)
{
	close(internetSocket);
}

int available(int sock)
{
	int timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof timeout);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	int available = select(sock + 1, &readfds, NULL, NULL, &tv);
	return available == 1;
}