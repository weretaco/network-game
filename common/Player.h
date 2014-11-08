#ifndef _PLAYER_H
#define _PLAYER_H

#include <string>
#include <map>

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
#elif defined LINUX
   #include <netinet/in.h>
#elif defined MAC
   #include <netinet/in.h>
#endif

#include "WorldMap.h"

using namespace std;

//forward declaration
class Game;

class Player {
private:
   unsigned int id;
   unsigned int targetPlayer;

public:

   enum PlayerClass {
      CLASS_NONE,
      CLASS_WARRIOR,
      CLASS_RANGER
   };

   enum AttackType {
      ATTACK_NONE,
      ATTACK_MELEE,
      ATTACK_RANGED
   };

   Player();
   Player(const Player& p);
   Player(string name, string password);

   ~Player();

   unsigned int getId();
   unsigned int getTargetPlayer();

   void setId(unsigned int id);
   void setTargetPlayer(unsigned int id);
   void setAddr(sockaddr_in addr);
   void setClass(PlayerClass c);

   void serialize(char* buffer);
   void deserialize(char* buffer);

   bool updateTarget(map<unsigned int, Player*>& players);
   bool move(WorldMap *map);
   void takeDamage(int damage);

   void takeFlag(unsigned int flag, WorldMap* map);
   void dropFlag(unsigned int flag, WorldMap* map);

   string name;
   string password;
   sockaddr_in addr;
   FLOAT_POSITION pos;
   POSITION target;
   unsigned long long timeLastUpdated;
   unsigned long long timeAttackStarted;
   unsigned long long timeDied;
   bool isChasing;
   bool isAttacking;
   bool isDead;

   PlayerClass playerClass;
   int maxHealth;
   int health;
   int attackType;
   int damage;
   int range;
   unsigned long long attackCooldown;
   int team; // 0 is none, 1 is blue, 2 is red
   bool hasBlueFlag;
   bool hasRedFlag;

   // permanent attributes
   unsigned int level;
   unsigned int experience;
   unsigned int honor;
   unsigned int wins;
   unsigned int losses;

   Game* currentGame;
};

#endif
