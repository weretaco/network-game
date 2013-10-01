#ifndef _PROJECTILE_H
#define _PROJECTILE_H

#include <string>
#include <map>

#include "Common.h"
#include "WorldMap.h"
#include "Player.h"

using namespace std;

class Projectile {
public:

   Projectile();
   Projectile(const Projectile& p);
   Projectile(int x, int y, int targetId, int damage);

   ~Projectile();

   void setId(int id);

   void serialize(char* buffer);
   void deserialize(char* buffer);

   // returns true if it reached the target and should be deleted
   bool move(map<unsigned int, Player*>& mapPlayers);

   /*
    * target should become a Player*. When this object gets serialized, the player's id should be sent.
    * Deserialization in this case might be tricky since it will require a playerMap to turn the id into a Plauyer*
    */

   int id;
   POSITION pos;
   int target;
   int speed;
   int damage;
   unsigned long long timeLastUpdated;
};

#endif
