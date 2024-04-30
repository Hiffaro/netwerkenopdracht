#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
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
void execution( int internet_socket );
void cleanup( int internet_socket );

int main( int argc, char * argv[] )
{
	OSInit();
	int internet_socket = initialization();
	execution( internet_socket );
	cleanup( internet_socket );
	OSCleanup();
	return 0;
}

int initialization()
{
	// Get local address info.
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	int getaddrinfo_return = getaddrinfo( "127.0.0.1", "5555", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	// Bind to socket.
	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			int connect_return = connect( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	// Clean up addrinfo.
	freeaddrinfo( internet_address_result );

	// Did we fail?
	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	// Return the found socket.
	return internet_socket;
}

void execution( int internet_socket )
{
	int playing = 0;
	int input = 0;
	int Received = 0;
	while (playing !=1){
		while (Received  != 3){
	
		if(Received = 1){
		printf("Hoger\n");
		}
		if(Received = 2){
		printf("Lager\n");
		}
	
		printf("pick a number between 0 and 1 mill\n");
		scanf("%i", &input);

		if(input >0 && input <1000000){

			int tosend = ntohl(input);
			char datasend[1000];
			itoa(tosend, datasend, 10);

			int number_of_bytes_send = 0;
			number_of_bytes_send = send( internet_socket, datasend, 16, 0 );
			if( number_of_bytes_send == -1 )
			{
			perror( "send\n" );
			}

	
			int number_of_bytes_received = 0;
			char buffer[1000];
			number_of_bytes_received = recv( internet_socket, buffer, ( sizeof buffer ) - 1, 0 );
			if( number_of_bytes_received == -1 )
			{
			perror( "recv\n" );
			}
			else
			{

			buffer[number_of_bytes_received] = '\0';
			Received = atoi(buffer);
			ntohl(Received);

			}
		}
		else
		{
		printf("pleas pick between 0 and 1Mill\n");
		}
	}
	printf("Correct\n");

	printf("press 0 to continue or press 1 to stop\n");
	scanf("%i", &playing);

	}
}

void cleanup( int internet_socket )
{
	//Step 3.2
	int shutdown_return = shutdown( internet_socket, SD_SEND );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}

	//Step 3.1
	close( internet_socket );
}
