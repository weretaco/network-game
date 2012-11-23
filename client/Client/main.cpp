#include "../../common/compiler.h"

#include <sys/types.h>

#ifdef WINDOWS
	#include <winsock2.h>
	#include <WS2tcpip.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <iostream>

#include <boost/lambda/lambda.hpp>

#include "../../common/message.h"

#ifdef WINDOWS
	#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

void error(const char *);

int main(int argc, char *argv[])
{
	int sock, n;
	struct sockaddr_in server, from;
	struct hostent *hp;
	char buffer[256];
	NETWORK_MSG msgTo, msgFrom;

	if (argc != 3) {
		cout << "Usage: server port" << endl;
		exit(1);
	}

	WORD wVersionRequested;
	WSADATA wsaData;
	int wsaerr;

	wVersionRequested = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequested, &wsaData);
	
	if (wsaerr != 0) {
		cout << "The Winsock dll not found." << endl;
		exit(1);
	}else
		cout << "The Winsock dll was found." << endl;
	
	sock= socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		error("socket");

	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp==0)
		error("Unknown host");

	memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	cout << "Please enter the message: ";
	cin.getline(msgTo.buffer, 256);
	socklen_t socklen = sizeof(server);

	n=sendMessage(&msgTo, sock, &server);
	if (n < 0)
		error("sendMessage");

	n = receiveMessage(&msgFrom, sock, &from);
	if (n < 0)
		error("receiveMessage");
	
	cout << "Got an ack: " << endl;
	cout << msgFrom.buffer << endl;

	closesocket(sock);

	WSACleanup();

	return 0;
}

// need to make a function like this that works on windows
void error(const char *msg)
{
    perror(msg);
	WSACleanup();
    exit(1);
}