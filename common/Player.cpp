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
}

Player::Player(string name, string password)
{
   this->id = 0;
   this->name = name;
   this->password = password;
   this->pos.x = this->target.x = 200;
   this->pos.y = this->target.y = 200;
}

Player::Player(string name, sockaddr_in addr)
{
   this->id = 0;
   this->name = name;
   this->password = "";
   this->pos.x = this->target.x = 200;
   this->pos.y = this->target.y = 200;
   this->addr = addr;
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
   strcpy(buffer+20, this->name.c_str());
}

void Player::deserialize(char* buffer)
{
   memcpy(&this->id, buffer, 4);
   memcpy(&this->pos.x, buffer+4, 4);
   memcpy(&this->pos.y, buffer+8, 4);
   memcpy(&this->target.x, buffer+12, 4);
   memcpy(&this->target.y, buffer+16, 4);
   this->name.assign(buffer+20);

   cout << "id: " << this->id << endl;
   cout << "pos x: " << this->pos.x << endl;
   cout << "pos y: " << this->pos.y << endl;
   cout << "target x: " << this->target.x << endl;
   cout << "target y: " << this->target.y << endl;
   cout << "name: " << this->name << endl;
}

void Player::setId(int id)
{
   this->id = id;
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}

void Player::move(void) {
   int speed = 100; // pixels per second
   unsigned long long curTime = getCurrentMillis();

   // if we're at our target, don't move
   if (pos.x != target.x || pos.y != target.y) {
      float pixels = speed * (curTime-timeLastUpdated) / 1000.0;

      double angle = atan2(target.y-pos.y, target.x-pos.x);

      float dist = sqrt(pow(target.x-pos.x, 2) + pow(target.y-pos.y, 2));
      if (dist <= pixels) {
         pos.x = target.x;
         pos.y = target.y;
      }else {
         pos.x += cos(angle)*pixels;
         pos.y += sin(angle)*pixels;
      }
   }

   timeLastUpdated = curTime;
}
