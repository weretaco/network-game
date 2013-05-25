#include "Player.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <cmath>

using namespace std;

Player::Player()
{
   this->id = 0;
   this->name = "";
   this->password = "";
   this->pos.x = this->target.x = 0;
   this->pos.y = this->target.y = 0;
   this->timeLastUpdated = 0;
   this->team = 0;   // blue team by default
   this->hasBlueFlag = false;
   this->hasRedFlag = false;
}

Player::Player(const Player& p)
{
   this->id = p.id;
   this->name = p.name;
   this->password = p.password;
   this->pos.x = p.pos.x;
   this->pos.y = p.pos.y;
   this->target.x = p.target.x;
   this->target.y = p.target.y;
   this->addr = p.addr;
   this->team = p.team;
   this->hasBlueFlag = p.hasBlueFlag;
   this->hasRedFlag = p.hasRedFlag;
}

Player::Player(string name, string password)
{
   this->id = 0;
   this->name = name;
   this->password = password;
   this->pos.x = this->target.x = 200;
   this->pos.y = this->target.y = 200;
   this->team = 0;   // blue team by default
   this->hasBlueFlag = false;
   this->hasRedFlag = false;
}

Player::~Player()
{
}

void Player::serialize(char* buffer)
{
   memcpy(buffer, &this->id, 4);
   memcpy(buffer+4, &this->pos.x, 4);
   memcpy(buffer+8, &this->pos.y, 4);
   memcpy(buffer+12, &this->target.x, 4);
   memcpy(buffer+16, &this->target.y, 4);
   memcpy(buffer+20, &this->team, 4);
   memcpy(buffer+24, &this->hasBlueFlag, 1);
   memcpy(buffer+25, &this->hasRedFlag, 1);
   strcpy(buffer+26, this->name.c_str());
}

void Player::deserialize(char* buffer)
{
   memcpy(&this->id, buffer, 4);
   memcpy(&this->pos.x, buffer+4, 4);
   memcpy(&this->pos.y, buffer+8, 4);
   memcpy(&this->target.x, buffer+12, 4);
   memcpy(&this->target.y, buffer+16, 4);
   memcpy(&this->team, buffer+20, 4);
   memcpy(&this->hasBlueFlag, buffer+24, 1);
   memcpy(&this->hasRedFlag, buffer+25, 1);
   this->name.assign(buffer+26);
}

void Player::setId(int id)
{
   this->id = id;
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}

bool Player::move(WorldMap *map) {
   int speed = 100; // pixels per second. should probably be in the constructor
   unsigned long long curTime = getCurrentMillis();

   // if we're at our target, don't move
   bool moving = (pos.x != target.x || pos.y != target.y);

   if (moving) {
      float pixels = speed * (curTime-timeLastUpdated) / 1000.0;
      double angle = atan2(target.y-pos.y, target.x-pos.x);
      float dist = sqrt(pow(target.x-pos.x, 2) + pow(target.y-pos.y, 2));

      if (dist <= pixels) {
         pos.x = target.x;
         pos.y = target.y;
      }else {
         pos.x = pos.x + cos(angle)*pixels;
         pos.y = pos.y + sin(angle)*pixels;
      }
   }

   timeLastUpdated = curTime;

   return moving;
}
