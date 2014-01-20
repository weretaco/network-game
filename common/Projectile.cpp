#include "Projectile.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <cmath>

using namespace std;

Projectile::Projectile()
{
   this->id = 0;
   this->pos.x = 0;
   this->pos.y = 0;
   this->target = 0;
   this->speed = 0;
   this->damage = 0;
   this->timeLastUpdated = 0;
}

Projectile::Projectile(const Projectile& p)
{
   this->id = p.id;
   this->pos.x = p.pos.x;
   this->pos.y = p.pos.y;
   this->target = p.target;
   this->speed = p.speed;
   this->damage = p.damage;
   this->timeLastUpdated = p.timeLastUpdated;
}

Projectile::Projectile(int x, int y, int targetId, int damage)
{
   this->id = 0; // the server probably sets this by calling setId passing in the length of the current projectile list
   this->pos.x = x;
   this->pos.y = y;
   this->target = targetId;
   this->speed = 400;
   this->damage = damage;
   this->timeLastUpdated = 0;
}

Projectile::~Projectile()
{
}

void Projectile::setId(unsigned int id)
{
   this->id = id;
}

void Projectile::serialize(char* buffer)
{
   memcpy(buffer, &this->id, 4);
   memcpy(buffer+4, &this->pos.x, 4);
   memcpy(buffer+8, &this->pos.y, 4);
   memcpy(buffer+12, &this->target, 4);
   memcpy(buffer+16, &this->speed, 4);
   memcpy(buffer+20, &this->damage, 4);
}

void Projectile::deserialize(char* buffer)
{
   memcpy(&this->id, buffer, 4);
   memcpy(&this->pos.x, buffer+4, 4);
   memcpy(&this->pos.y, buffer+8, 4);
   memcpy(&this->target, buffer+12, 4);
   memcpy(&this->speed, buffer+16, 4);
   memcpy(buffer+16, &this->speed, 4);
   memcpy(buffer+20, &this->damage, 4);
}

bool Projectile::move(map<unsigned int, Player*>& mapPlayers) {
   // if the current target logs off, this method will run into problems

   unsigned long long curTime = getCurrentMillis();

   Player* targetP = mapPlayers[target];

   if (timeLastUpdated == 0) {
      timeLastUpdated = curTime;
      return false;
   }


   float pixels = speed * (curTime-timeLastUpdated) / 1000.0;
   double angle = atan2(targetP->pos.y-pos.y, targetP->pos.x-pos.x);
   float dist = sqrt(pow(targetP->pos.x-pos.x, 2) + pow(targetP->pos.y-pos.y, 2));

   if (dist <= pixels) {
      pos.x = targetP->pos.x;
      pos.y = targetP->pos.y;
      return true;
   }else {
      pos.x = pos.x + cos(angle)*pixels;
      pos.y = pos.y + sin(angle)*pixels;
      return false;
   }
}
