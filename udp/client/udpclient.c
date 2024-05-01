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
#include <synchapi.h>

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
	char netbuf[256];
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

	// Prepare space for receiving.
	struct sockaddr_storage client_ia;
	socklen_t client_ia_length = sizeof(client_ia);

	// Execution.
	int in_game = 0;
	int awaiting_input = 0;
	int guess = -1;
	clock_t start;
	int timer_running = 0;
	while (1)
	{
		// Populate the input and parse.
		read_key_into_buffer(buffer, sizeof(buffer), &offset);
		if(!awaiting_input)
		{
			printf("Make a guess between 0 and 99, or type quit to quit: ");
			awaiting_input = 1;
		}
		else if(strcmp(buffer, "quit") == 0)
		{
			break;
		}
		else if(is_buffer_ready(buffer))
		{
			int handled = sscanf(buffer, "%d", &guess);
			if(handled != 1 || guess < 0 || guess > 99)
			{
				guess = -1;
			}
			clear_buffer(buffer, sizeof(buffer), &offset);
			awaiting_input = 0;
		}

		// Send guess if given.
		if(!awaiting_input && guess != -1)
		{
			// Convert to string.
			memset(netbuf, 0, sizeof(netbuf));
			sprintf(netbuf, "%d", guess);

			// Send over socket.
			sendto(internet_socket, netbuf, strlen(netbuf), 0, internet_address->ai_addr, internet_address->ai_addrlen);
			
			// Start the 16s timeout.
			start = clock();
			timer_running = 1;

			// Just a very small sleep to allow the server to react for maximum usability.
			Sleep(10);
		}

		// Get a response?
		int timeout = 0;
		setsockopt(internet_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof timeout);
		struct timeval tv;
		fd_set readfds;
		FD_ZERO(&readfds);
        FD_SET(internet_socket, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
		int available = select(internet_socket + 1, &readfds, NULL, NULL, &tv); 
		if(available == 1)
		{
			int count = recvfrom(internet_socket, netbuf, sizeof(netbuf) - 1, 0, (struct sockaddr *)&client_ia, &client_ia_length);
			if(strcmp(netbuf, "You won !") == 0 || strcmp(netbuf, "You Won !") == 0) timer_running = 0;
			if(guess == -1) printf("\n%s\nMake a guess between 0 and 99, or type quit to quit: %s", netbuf, buffer);
			else            printf("%s\n", netbuf);
		}

		// Check for timeout.
		if(timer_running) 
		{
			clock_t now = clock();
			if((now - start) / CLOCKS_PER_SEC > 16.0)
			{
				if(awaiting_input) printf("\nYou Lost ?\n");
				else               printf("You Lost ?\n");
				timer_running = 0;
			}
		}

		// Always reset guess when we get here.
		guess = -1;
	}

	// Clean up.
	freeaddrinfo(internet_address);
	close(internet_socket);
	WSACleanup();
	return 0;
}

void read_key_into_buffer(char *buffer, int size, int *offset)
{
	if(_kbhit()) 
	{
		buffer[*offset] = _getche();
		*offset += 1;
		if(*offset == size-1)
		{
			buffer[*offset] = '\r';
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
	char *ready = strchr(buffer, '\r');
	if(ready == NULL) return 0;
	printf("\n");
	*ready = '\n';
	return 1;
}