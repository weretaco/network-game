#ifndef _GAME_H
#define _GAME_H

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <netinet/in.h>
#endif

#include <string>
#include <map>

#include "Player.h"

using namespace std;

class Game {
private:
   int id;
   string name;
   map<int, Player*> players;

public:
   Game();
   Game(string name);

   ~Game();

   int getNumPlayers();

   void setId(int id);
   bool addPlayer(Player* p);
   bool removePlayer(int id);
};

#endif
