#include "Message.h"

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <sys/socket.h>
   #include <netinet/in.h>
#endif

#include <iostream>

using namespace std;

int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest)
{
   int ret =  sendto(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

   cout << "Send a message of type " << msg->type << endl;

   return ret;
}

int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest)
{
   socklen_t socklen = sizeof(struct sockaddr_in);

   // assume we don't care about the value of socklen
   int ret =  recvfrom(sock, (char*)msg, sizeof(NETWORK_MSG), 0, (struct sockaddr *)dest, &socklen);

   return ret;
}
