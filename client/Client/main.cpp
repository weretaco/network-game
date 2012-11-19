/* UDP client in the internet domain */
#include <sys/types.h>

#include <winsock2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <boost/lambda/lambda.hpp>

#include "../../common/message.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void error(const char *);

int boost_main()
{
    using namespace boost::lambda;
    typedef istream_iterator<int> in;

    for_each(in(cin), in(), cout << (_1 * 3) << " " );

	return 0;
}

int main(int argc, char *argv[])
{
   int sock, n;
   int length;
   struct sockaddr_in server, from;
   struct hostent *hp;
   char buffer[256];
   
	if (argc != 3) {
		printf("Usage: server port\n");
		exit(1);
	}

	WORD wVersionRequested;
	WSADATA wsaData;
	int wsaerr;

	wVersionRequested = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequested, &wsaData);
	
	if (wsaerr != 0) {
		printf("The Winsock dll not found.\n");
		exit(1);
	}else
		printf("The Winsock dll was found.\n");
	
	sock= socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("socket");

	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp==0) error("Unknown host");

	memcpy((char *)&server.sin_addr,
			(char *)hp->h_addr,
			hp->h_length);
   server.sin_port = htons(atoi(argv[2]));
   length=sizeof(struct sockaddr_in);
   printf("Please enter the message: ");
   memset(buffer, 0, 256);
   fgets(buffer,255,stdin);
   n=sendto(sock,buffer,
            strlen(buffer),0,(const struct sockaddr *)&server,length);
	if (n < 0) error("Sendto");
	n = recvfrom(sock,buffer,256,0,(struct sockaddr *)&from, &length);
	if (n < 0) error("recvfrom");

	buffer[n] = '\0';
	cout << "Got an ack: " << endl;
	cout << buffer << endl;

	closesocket(sock);

	WSACleanup();

	return 0;
}

/*
int sendMessage(short type, string contents, int sock, struct sockaddr_in *dest)
{
	NETWORK_MSG msg;

	msg.type = type;
	strcpy(msg.buffer, contents.c_str());

	return sendto(sock, (char*)&msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, sizeof(dest));
}
*/

void error(const char *msg)
{
    perror(msg);
	WSACleanup();
    exit(1);
}