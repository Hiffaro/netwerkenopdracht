#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7 //select minimal legacy support, needed for inet_pton, inet_ntop
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <stdint.h>

	void setTimeOut( uint32_t timeOutInMilis, uint32_t socket ) {
        DWORD timeOut = timeOutInMilis;
        setsockopt( socket, SOL_SOCKET, SO_RCVTIMEO, ( const char* )&timeOut, sizeof( timeOut ) );
    }
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
#endif

int main( int argc, char * argv[] )
{
	//Initialization//

	WSADATA wsaData;
	WSAStartup( MAKEWORD(2,0), &wsaData );

	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address = NULL;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	getaddrinfo( "fe80::4659:7f0d:5209:2ee2", "24042", &internet_address_setup, &internet_address );

	int numberOfBytesReceived = 0;
    int timer = 16000;
	char input[20];
	int value = 0;
	int stop = 0;
	int internet_socket;
	internet_socket = socket( internet_address->ai_family, internet_address->ai_socktype, internet_address->ai_protocol );


	//Execution//

	while (stop != 1) 
	{
		printf("type in a number between 0 and 99 ");
		scanf("%s", input);
		
		if (strcmp(input, "stop") == 0){
			stop = 1;
			continue;
		}
		value = atoi(input);
		if(value >= 0 && value <100){

		sendto( internet_socket, input, strlen(input), 0, internet_address->ai_addr, internet_address->ai_addrlen );

		}
		else{
			printf("pick a number between 0 and 99 or type stop to end the game");
			continue;
		}
		
		char buffer[100] = {0};
		setTimeOut(timer, internet_socket);
		struct sockaddr_storage clientInternetAddress;
		socklen_t clientInternetAddressLength = sizeof(clientInternetAddress);
		numberOfBytesReceived = recvfrom( internet_socket, buffer, sizeof( buffer ) -1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
			if( numberOfBytesReceived == -1 ) {
				printf( "you lost?" );
			} else {
				buffer[numberOfBytesReceived] = '\0';
				//printf( "Received : \"%s\"\n", buffer );//
				printf("you won\n");
			}


		

		printf("want to play a other game type \"continue\" otherwise type \"stop\"\n");

		while( stop != 1 ) 
		{
			scanf("%s", input);
			
			if (strcmp(input, "stop") == 0){
				stop = 1;
				continue;
			}
			else if 
			(strcmp(input, "continue") == 0){
				break;
			
			}
			printf("type \"stop\" or \"continue\"\n");
		}
	}

	//Clean up//
	freeaddrinfo( internet_address );
	close( internet_socket );
	WSACleanup();

	return 0;
}



