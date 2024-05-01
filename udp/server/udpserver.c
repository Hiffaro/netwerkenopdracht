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
	#include <time.h>

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
	#include <time.h>

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

//defines
#define BUFFERSIZE 100

//function prototypes
int initialization();
int bufToInt( const char *buf );
void execution( int internetSocket );
void cleanup( int internetSocket );

//main
int main( int argc, char * argv[] ) {
	//init

	srand( time( NULL ) );
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
	int getaddrinfoReturn = getaddrinfo( NULL, "5555", &internetAddressSetup, &internetAddressResult );
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
	int serverRunning = 1;
	int gameRunning = 0;
	int winner = 1;
	int guess = -1;
	int currentWinningGuess = -1;
	int rndNum = 0;
	int timeOutInMilis = 8000;
	char buffer[BUFFERSIZE];
	struct sockaddr_storage clientInternetAddress;
	struct sockaddr_storage winningClientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof( clientInternetAddress );
	socklen_t winningClientInternetAddressLength = sizeof( winningClientInternetAddress );
	
	//init
	//setTimeOut( 2000, internetSocket );

	while( serverRunning != 0 ) {
		//receive data
		numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
		if( numberOfBytesReceived == -1 ) {
			//handle if timeout happens
			setTimeOut( 0, internetSocket );

			memset( buffer, '\0', BUFFERSIZE );
			strcpy( buffer, "you won?" );
			
			//send data
			numberOfBytesSend = sendto( internetSocket, buffer, strlen( buffer ), 0, ( struct sockaddr * )&winningClientInternetAddress, winningClientInternetAddressLength );
			if( numberOfBytesSend == -1 ) {
				perror( "sendto" );
			}

			int startTime = time( NULL );
			memset( buffer, '\0', BUFFERSIZE );
			strcpy( buffer, "you lost!" );

			while( ( time(NULL) - startTime ) <= 16 ) {
				numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
				numberOfBytesSend = sendto( internetSocket, buffer, strlen( buffer ), 0, ( struct sockaddr * )&clientInternetAddress, clientInternetAddressLength );

				if( ( struct sockaddr * )&clientInternetAddress == ( struct sockaddr * )&winningClientInternetAddress ) {
					winner = 0;
				}
			}

			if( winner ) {
				memset( buffer, '\0', BUFFERSIZE );
				strcpy( buffer, "you won!" );
			
				//send data
				numberOfBytesSend = sendto( internetSocket, buffer, strlen( buffer ), 0, ( struct sockaddr * )&winningClientInternetAddress, winningClientInternetAddressLength );
				if( numberOfBytesSend == -1 ) {
					perror( "sendto" );
				}
			}

			//reset all variables
			numberOfBytesReceived = 0;
			numberOfBytesSend = 0;
			serverRunning = 1;
			gameRunning = 0;
			winner = 1;
			guess = -1;
			currentWinningGuess = -1;
			rndNum = 0;
			timeOutInMilis = 8000;

			continue;
		} else {
			buffer[numberOfBytesReceived] = '\0';
			printf( "Received : \"%s\"\n", buffer );
			
			if( strcmp( buffer, "close" ) == 0 ) {
				serverRunning = 0;
				continue;
			}
		}

		//handle buffer and set guess
		guess = bufToInt( buffer );
		if( guess < 0 || guess > 99 ) {
			printf( "Invalid guess\n" );
			continue;
		} else if( gameRunning == 0 ) {
			//if there is no game running start a new one
			
			rndNum = rand() % 100;
			gameRunning = 1;
			currentWinningGuess = guess;
			memcpy( &winningClientInternetAddress, &clientInternetAddress, sizeof( clientInternetAddress ) );
			memcpy( &winningClientInternetAddressLength, &clientInternetAddressLength, sizeof( clientInternetAddressLength ) );
			setTimeOut( timeOutInMilis, internetSocket );
		} else if( gameRunning == 1 ) {
			//if there already is a game running handle the new guess
			if( abs( rndNum - currentWinningGuess ) > abs( rndNum - guess ) ) {
				memcpy( &winningClientInternetAddress, &clientInternetAddress, sizeof( clientInternetAddress ) );
				memcpy( &winningClientInternetAddressLength, &clientInternetAddressLength, sizeof( clientInternetAddressLength ) );
			}
			timeOutInMilis /= 2;
			setTimeOut( timeOutInMilis, internetSocket );
		}

		

		printf( "cycle\n" );
	}
}

//standart cleanup function to close the internet socket provided
void cleanup( int internetSocket ) {
	close( internetSocket );
}

//custom atoi for better performance
int bufToInt( const char *buf ) {
	uint32_t rslt = 0;

	for( uint8_t i = 0; buf[i] != '\0'; i++ ) {
		if( buf[i] >= '0' && buf[i] <= '9' ) {
			rslt *= 10;
			rslt += buf[i] - '0';
		} else {
			return -1;
		}
	}

	return rslt;
}
