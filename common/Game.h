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
#include "MessageProcessor.h"

using namespace std;

class Game {
private:
   unsigned int id;
   string name;
   map<unsigned int, Player*> players;
   map<unsigned int, Projectile> projectiles;
   WorldMap* worldMap;
   unsigned int blueScore;
   unsigned int redScore;
   unsigned int unusedProjectileId;
   MessageProcessor* msgProcessor;

public:
   Game();
   Game(string name, string filepath, MessageProcessor* msgProcessor);

   ~Game();

   string getName();
   int getNumPlayers();
   unsigned int getBlueScore();
   unsigned int getRedScore();
   WorldMap* getMap();

   void setId(unsigned int id);
   void setBlueScore(unsigned int score);
   void setRedScore(unsigned int score);

   void addObjectToMap(WorldMap::ObjectType objectType, int x, int y);

   map<unsigned int, Player*>& getPlayers();
   bool addPlayer(Player* p);
   bool removePlayer(unsigned int id);

   map<unsigned int, Projectile>& getProjectiles();
   bool addProjectile(Projectile p);
   bool removeProjectile(unsigned int id);

   bool startPlayerMovement(unsigned int id, int x, int y);
   bool processPlayerMovement(Player* p, FLOAT_POSITION oldPos);
   int processFlagPickupRequest(Player* p);
   void dealDamageToPlayer(Player* p, int damage);

   bool handleGameEvents();
   bool handlePlayerEvents(Player* p);

   void assignProjectileId(Projectile* p);
   void updateUnusedProjectileId();
};

#endif
