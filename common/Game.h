#ifndef _GAME_H
#define _GAME_H

#include "Compiler.h"

#include <string>
#include <map>

#ifdef WINDOWS
   #define WIN32_LEAN_AND_MEAN
#endif

#include "Player.h"
#include "WorldMap.h"

using namespace std;

class Game {
private:
   unsigned int id;
   string name;
   map<unsigned int, Player*> players;
   WorldMap* worldMap;
   int blueScore;
   int redScore;

public:
   Game();
   Game(string name, string filepath);

   ~Game();

   string getName();
   int getNumPlayers();
   map<unsigned int, Player*>& getPlayers();
   int getBlueScore();
   int getRedScore();
   WorldMap* getMap();

   void setId(unsigned int id);
   bool addPlayer(Player* p);
   bool removePlayer(unsigned int id);
   void setBlueScore(int score);
   void setRedScore(int score);
};

#endif
