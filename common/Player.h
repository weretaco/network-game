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
#include "WorldMap.h"

using namespace std;

class Player {
public:
   Player();
   Player(const Player& p);
   Player(string name, string password);

   ~Player();

   void serialize(char* buffer);
   void deserialize(char* buffer);

   void setId(int id);
   void setAddr(sockaddr_in addr);

   bool move(WorldMap *map);

   void takeFlag(int flag, WorldMap *map);
   void dropFlag(int flag, WorldMap *map);

   int id;
   string name;
   string password;
   sockaddr_in addr;
   FLOAT_POSITION pos;
   POSITION target;
   unsigned long long timeLastUpdated;

   int team; // 0 is blue, 1 is red
   bool hasBlueFlag;
   bool hasRedFlag;
};

#endif
