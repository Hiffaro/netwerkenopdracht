//includes
#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
    #include <stdint.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <unistd.h>

    //windows needs wsaData functionality

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

	void setTimeOut( uint32_t timeOutInMilis, uint32_t socket ) {
		DWORD timeOut = timeOutInMilis;
		setsockopt( socket, SOL_SOCKET, SO_RCVTIMEO, ( const char* )&timeOut, sizeof( timeOut ) );
	}

	#define perror( string ) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
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

	int OSInit( void ) {}
	int OSCleanup( void ) {}

	void setTimeOut( uint32_t timeOutInMilis, uint32_t socket ) {
		struct timeval tv;

		uint32_t sec = timoutInMilis / 1000;
		uint32_t usec = ( timeOutInMilis % 1000 ) * 1000;

		tv.tv_sec = sec;
		tv.tv_usec = usec;
		setsockopt( socket, SOL_SOCKET, SO_RCVTIMEO, ( const char* )&tv, sizeof( tv ) );
	}
#endif

//enums and structs

//function prototypes

int initialization();
void execution( int internetSocket );
void cleanup( int internetSocket );

//main

int main( int argc, char * argv[] ) {
	//init

	OSInit();
	int internetSocket = initialization();

	//execution

	execution( internetSocket );

	//closing

	cleanup( internetSocket );
	OSCleanup();

	return 0;
}

//function definitions

int initialization() {
    //variables
	struct addrinfo internetAddressSetup;
	struct addrinfo *internetAddressResult;
	memset( &internetAddressSetup, 0, sizeof internetAddressSetup );
	internetAddressSetup.ai_family = AF_UNSPEC;
	internetAddressSetup.ai_socktype = SOCK_DGRAM;
	internetAddressSetup.ai_flags = AI_PASSIVE;
	int getaddrinfoReturn = getaddrinfo( NULL, "24042", &internetAddressSetup, &internetAddressResult );
    //if something went wrong with the getaddrinfo function exit
	if( getaddrinfoReturn != 0 ) {
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfoReturn ) );
		exit( 1 );
	}
	int internetSocket = -1;
	struct addrinfo *internetAddressResultIterator = internetAddressResult;

    //get valid internetSocket
	while( internetAddressResultIterator != NULL ) {
		internetSocket = socket( internetAddressResultIterator->ai_family, internetAddressResultIterator->ai_socktype, internetAddressResultIterator->ai_protocol );
		if( internetSocket == -1 ) {
			perror( "socket" );
		} else {
			int bindReturn = bind( internetSocket, internetAddressResultIterator->ai_addr, internetAddressResultIterator->ai_addrlen );
			if( bindReturn == -1 ) {
				close( internetSocket );
				perror( "bind" );
			} else {
				break;
			}
		}
		internetAddressResultIterator = internetAddressResultIterator->ai_next;
	}

    //free addrinfo before exiting
	freeaddrinfo( internetAddressResult );

    //exit if no valid socket was found
	if( internetSocket == -1 ) {
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

    //return internetsocket that is being used by the server from now on
	return internetSocket;
}

//execution fase, this function contains all logic and functions
void execution( int internetSocket ) {
    //variables
	int numberOfBytesReceived = 0;
	int numberOfBytesSend = 0;
	int running = 1;
	char buffer[1000];
	struct sockaddr_storage clientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof( clientInternetAddress );
	
	//init
	setTimeOut( 2000, internetSocket );

	while( running != 0 ) {
		//receive data
		numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
		if( numberOfBytesReceived == -1 ) {
			perror( "recvfrom" );
		} else {
			buffer[numberOfBytesReceived] = '\0';
			printf( "Received : \"%s\"\n", buffer );
			
			if( strcmp( buffer, "close" ) == 0 ) {
				running = 0;
			}
			
			//send data
			numberOfBytesSend = sendto( internetSocket, "Hello UDP world!", 16, 0, ( struct sockaddr * )&clientInternetAddress, clientInternetAddressLength );
			if( numberOfBytesSend == -1 ) {
				perror( "sendto" );
			}
		}
	}
}

//standart cleanup function to close the internet socket provided
void cleanup( int internetSocket ) {
	close( internetSocket );
}
