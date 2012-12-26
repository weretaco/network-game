#ifndef _PLAYER_H
#define _PLAYER_H

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <netinet/in.h>
#endif

#include <string>

#include "Common.h"

using namespace std;

class Player {
public:
   Player(string name, string password);
   Player(string name, sockaddr_in addr); // this will be deleted
   ~Player();

   void serialize(char* buffer);
   void deserialize(char* buffer);

   void setAddr(sockaddr_in addr);

   string name;
   string password;
   sockaddr_in addr;
   PLAYER_POS pos;
};

#endif
