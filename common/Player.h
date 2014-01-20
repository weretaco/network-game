#ifndef _PLAYER_H
#define _PLAYER_H

#include <string>
#include <map>

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
#elif defined LINUX
   #include <netinet/in.h>
#endif

#include "WorldMap.h"

using namespace std;

//forward declaration
class Game;

class Player {
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

   void setId(unsigned int id);
   void setAddr(sockaddr_in addr);
   void setClass(PlayerClass c);

   void serialize(char* buffer);
   void deserialize(char* buffer);

   bool updateTarget(const Player* targetPlayer);
   bool move(WorldMap *map);
   void takeDamage(int damage);

   void takeFlag(unsigned int flag, WorldMap* map);
   void dropFlag(unsigned int flag, WorldMap* map);

   unsigned int id;
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
   unsigned int targetPlayer;
   bool isDead;

   int playerClass;
   int maxHealth;
   int health;
   int attackType;
   int damage;
   int range;
   unsigned long long attackCooldown;
   int team; // 0 is blue, 1 is red
   bool hasBlueFlag;
   bool hasRedFlag;

   Game* currentGame;
};

#endif
