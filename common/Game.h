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
#include "Projectile.h"

using namespace std;

class Game {
private:
   unsigned int id;
   string name;
   map<unsigned int, Player*> players;
   map<unsigned int, Projectile> projectiles;
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
   map<unsigned int, Projectile>& getProjectiles();
   int getBlueScore();
   int getRedScore();
   WorldMap* getMap();

   void setId(unsigned int id);
   void setBlueScore(int score);
   void setRedScore(int score);

   bool addPlayer(Player* p);
   bool removePlayer(unsigned int id);
   bool startPlayerMovement(unsigned int id, int x, int y);
   bool processPlayerMovement(Player* p, FLOAT_POSITION oldPos);
   int processFlagPickupRequest(Player* p);

   bool addProjectile(Projectile p);
   bool removeProjectile(unsigned int id);
};

#endif
