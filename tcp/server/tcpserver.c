#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h> //for time( NULL )
	#include <stdint.h> //for uintx_t variables

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

enum opcodes {
	serverRunning,
	serverStopped
};

int initialization();
int connection( int internetSocket );
void execution( int internetSocket, enum opcodes *code );
void closeClientSocket( int clientInternetSocket );
void cleanup( int internetSocket );

int main( int argc, char * argv[] ) {
	//variables
	int running = 1;
	enum opcodes opcode = serverRunning;
	int clientInternetSocket = 0;

	//Initialization

	srand( time( NULL ) );
	OSInit();

	int internetSocket = initialization();

	//basic gameloop
	while( opcode == serverRunning) {
		//Connection

		clientInternetSocket = connection( internetSocket );

		//Execution

		execution( clientInternetSocket, &opcode );
		closeClientSocket( clientInternetSocket );
	}

	//Clean up

	cleanup( internetSocket );
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
	int getaddrinfo_return = getaddrinfo( NULL, "5555", &internetAddressSetup, &internetAddressResult );
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

void execution( int internetSocket, enum opcodes *code ) {
	//Step 3.1
	int numberOfBytesReceived = 0;
	int guess = 0;
	int game = 1;
	
	int toGuess = rand() % 1000000;
	printf( "toGuess: %i\n", toGuess );
	uint8_t buffer[1000];

	while( game == 1 ) {
		memset( buffer, '\0', 1000 );

		numberOfBytesReceived = recv( internetSocket, buffer, sizeof( buffer ) - 1, 0 );
		if( numberOfBytesReceived == -1 ) {
			perror( "recv" );
			game = 0;
			break;
		} else {
			printf( "buffer: %i\n", buffer );
			guess = *( uint32_t * )buffer;
			printf( "guess: %i\n", guess );
			guess = ntohl( guess );
			printf( "guess: %i\n", guess );

			if( guess < toGuess ) {
				strcpy( buffer, "Lager" );
			} else if( guess > toGuess ) {
				strcpy( buffer, "Hoger" );
			} else if( guess == toGuess ) {
				strcpy( buffer, "Correct" );
				game = 0;
			} else if( guess == -1 ) {
				game = 0;
				*code = serverStopped;
			}
				
			//send response
			int number_of_bytes_send = 0;
			number_of_bytes_send = send( internetSocket, buffer, strlen( buffer ), 0 );
			if( number_of_bytes_send == -1 ) {
				perror( "send" );
				game = 0;
				break;
			}
		}
	}
}

//code used to shutdown the connection
void closeClientSocket( int clientInternetSocket ) {
	int shutdownReturn = shutdown( clientInternetSocket, SD_RECEIVE );
	if( shutdownReturn == -1 ) {
		perror( "shutdown" );
	}
    
	close( clientInternetSocket );
}

//code used to close the internetSockets
void cleanup( int internetSocket ) {
	close( internetSocket );
}
