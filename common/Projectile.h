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
   unsigned int id;
   POSITION pos;
   unsigned int target;
   int speed;
   int damage;
   unsigned long long timeLastUpdated;

   Projectile();
   Projectile(const Projectile& p);
   Projectile(int x, int y, int targetId, int damage);

   ~Projectile();

   void setId(unsigned int id);

   void serialize(char* buffer);
   void deserialize(char* buffer);

   // returns true if the projectile reached the target and should be deleted
   bool move(map<unsigned int, Player*>& mapPlayers);
};

#endif
