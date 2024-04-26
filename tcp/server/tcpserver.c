#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset

	void OSInit( void ) {
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 ) {
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}

	void OSCleanup( void ) {
		WSACleanup();
	}

	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
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
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

int initialization();
int connection( int internetSocket );
void execution( int internetSocket );
void cleanup( int internetSocket, int clientInternetSocket );

int main( int argc, char * argv[] ) {
	//Initialization

	OSInit();

	int internetSocket = initialization();

	//Connection

	int clientInternetSocket = connection( internetSocket );

	//Execution

	execution( clientInternetSocket );

	//Clean up

	cleanup( internetSocket, clientInternetSocket );
	OSCleanup();

	return 0;
}

int initialization() {
	//Step 1.1
	struct addrinfo internetAddressSetup;
	struct addrinfo * internetAddressResult;
	memset( &internetAddressSetup, 0, sizeof( internetAddressSetup ) );
	internetAddressSetup.ai_family = AF_UNSPEC;
	internetAddressSetup.ai_socktype = SOCK_STREAM;
	internetAddressSetup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "24042", &internetAddressSetup, &internetAddressResult );
	if( getaddrinfo_return != 0 ) {
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internetSocket = -1;
	struct addrinfo * internetAddressResultIterator = internetAddressResult;
	while( internetAddressResultIterator != NULL ) {
		//Step 1.2
		internetSocket = socket( internetAddressResultIterator->ai_family, internetAddressResultIterator->ai_socktype, internetAddressResultIterator->ai_protocol );
		if( internetSocket == -1 ) {
			perror( "socket" );
		} else {
			//Step 1.3
			int bind_return = bind( internetSocket, internetAddressResultIterator->ai_addr, internetAddressResultIterator->ai_addrlen );
			if( bind_return == -1 ) {
				perror( "bind" );
				close( internetSocket );
			} else {
				//Step 1.4
				int listen_return = listen( internetSocket, 1 );
				if( listen_return == -1 ) {
					close( internetSocket );
					perror( "listen" );
				} else {
					break;
				}
			}
		}
		internetAddressResultIterator = internetAddressResultIterator->ai_next;
	}

	freeaddrinfo( internetAddressResult );

	if( internetSocket == -1 ) {
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internetSocket;
}

int connection( int internetSocket ) {
	//Step 2.1
	struct sockaddr_storage clientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof clientInternetAddress;
	int clientSocket = accept( internetSocket, (struct sockaddr *) &clientInternetAddress, &clientInternetAddressLength );
	if( clientSocket == -1 ) {
		perror( "accept" );
		close( internetSocket );
		exit( 3 );
	}
	return clientSocket;
}

void execution( int internetSocket ) {
	//Step 3.1
	int numberOfBytesReceived = 0;
	char buffer[1000];
	numberOfBytesReceived = recv( internetSocket, buffer, ( sizeof buffer ) - 1, 0 );
	if( numberOfBytesReceived == -1 ) {
		perror( "recv" );
	} else {
		buffer[numberOfBytesReceived] = '\0';
		printf( "Received : %s\n", buffer );
	}

	//Step 3.2
	int number_of_bytes_send = 0;
	number_of_bytes_send = send( internetSocket, "Hello TCP world!", 16, 0 );
	if( number_of_bytes_send == -1 ) {
		perror( "send" );
	}
}

//code used to shutdown the connection and close the internetSockets
void cleanup( int internetSocket, int clientInternetSocket ) {
	int shutdownReturn = shutdown( clientInternetSocket, SD_RECEIVE );
	if( shutdownReturn == -1 ) {
		perror( "shutdown" );
	}
    
	close( clientInternetSocket );
	close( internetSocket );
}
