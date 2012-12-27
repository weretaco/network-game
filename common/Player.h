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
   Player();
   Player(const Player& p);
   Player(string name, string password);
   Player(string name, sockaddr_in addr); // this will be deleted

   ~Player();

   void serialize(char* buffer);
   void deserialize(char* buffer);

   void setId(int id);
   void setAddr(sockaddr_in addr);

   int id;
   string name;
   string password;
   sockaddr_in addr;
   PLAYER_POS pos;
};

#endif
