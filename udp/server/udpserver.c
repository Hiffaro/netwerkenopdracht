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

//enums
enum states {
	idle,
	running,
	timeout
};

//function prototypes
int initialization();
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
	int numberToGuess = 0;
	int winningGuess = 0;
	int noWinner = 0;
	int guess = 0;
	int timeOutInMilis = 8000;
	clock_t timeZero = 0;
	char buffer[BUFFERSIZE];
	struct sockaddr_storage clientInternetAddress;
	struct sockaddr_storage winningClientInternetAddress;
	socklen_t clientInternetAddressLength = sizeof( clientInternetAddress );
	socklen_t winningClientInternetAddressLength = sizeof( winningClientInternetAddress );
	enum states state = idle;
	
	//init
	//setTimeOut( 2000, internetSocket );

	while( serverRunning != 0 ) {
		switch ( state ) {
			case idle:
				printf( "idle\n" );
				//receive data
				numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
				if( numberOfBytesReceived == -1 ) {
					perror( "recvfrom" );
				} else {
					buffer[numberOfBytesReceived] = '\0';
					printf( "Received : \"%s\"\n", buffer );
					numberToGuess = rand() % 100;
					numberOfBytesSend = sendto( internetSocket, "you won?", 8, 0, ( struct sockaddr * )&clientInternetAddress, clientInternetAddressLength );
					if( numberOfBytesSend == -1 ) {
						perror( "sendto" );
					}
					winningClientInternetAddress = clientInternetAddress;
					winningClientInternetAddressLength = clientInternetAddressLength;
					winningGuess = atoi( buffer );
					state = running;
				}
				break;
			case running:
				printf( "running\n" );
				setTimeOut( timeOutInMilis, internetSocket );
				//receive data
				numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
				if( numberOfBytesReceived == -1 ) {
					state = timeout;
					timeZero = clock();
					perror( "recvfrom" );
				} else {
					buffer[numberOfBytesReceived] = '\0';
					printf( "Received : \"%s\"\n", buffer );
					guess = atoi( buffer );
					if( abs( numberToGuess - guess ) < abs( numberToGuess - winningGuess ) ) {
						numberOfBytesSend = sendto( internetSocket, "you won?", 8, 0, ( struct sockaddr * )&clientInternetAddress, clientInternetAddressLength );
						if( numberOfBytesSend == -1 ) {
							perror( "sendto" );
						}
						winningClientInternetAddress = clientInternetAddress;
						winningClientInternetAddressLength = clientInternetAddressLength;
						winningGuess = guess;
					}
					timeOutInMilis /= 2;
				}

				break;
			case timeout:
				printf( "timeout\n" );
				setTimeOut( 10, internetSocket );
				numberOfBytesReceived = recvfrom( internetSocket, buffer, sizeof( buffer ) - 1, 0, ( struct sockaddr * )&clientInternetAddress, &clientInternetAddressLength );
				if( numberOfBytesReceived == -1 ) {
					perror( "recvfrom" );
				} else {
					buffer[numberOfBytesReceived] = '\0';
					printf( "Received : \"%s\"\n", buffer );
					numberOfBytesSend = sendto( internetSocket, "you lost!", 9, 0, ( struct sockaddr * )&clientInternetAddress, clientInternetAddressLength );
					if( numberOfBytesSend == -1 ) {
						perror( "sendto" );
					}
					if( ( struct sockaddr * )&clientInternetAddress == ( struct sockaddr * )&winningClientInternetAddress ) {
						noWinner = 1;
					}
				}

				if( (clock() - timeZero ) / CLOCKS_PER_SEC > 16 ) {
					setTimeOut( 0, internetSocket );
					if( noWinner == 0 ) {
						numberOfBytesSend = sendto( internetSocket, "you won!", 8, 0, ( struct sockaddr * )&winningClientInternetAddress, winningClientInternetAddressLength );
						if( numberOfBytesSend == -1 ) {
							perror( "sendto" );
						}
					}
					winningGuess = 0;
					noWinner = 0;
					guess = 0;
					timeOutInMilis = 8000;
					state = idle;
				}
				break;
		}
	}
}

//standart cleanup function to close the internet socket provided
void cleanup( int internetSocket ) {
	close( internetSocket );
}
