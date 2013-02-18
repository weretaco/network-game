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
   strcpy(buffer+4, this->name.c_str());
   memcpy(buffer+5+this->name.length(), &this->pos.x, 4);
   memcpy(buffer+9+this->name.length(), &this->pos.y, 4);
}

void Player::deserialize(char* buffer)
{
   memcpy(&this->id, buffer, 4);
   this->name.assign(buffer+4);
   memcpy(&this->pos.x, buffer+5+this->name.size(), 4);
   memcpy(&this->pos.y, buffer+9+this->name.size(), 4);

   cout << "id: " << this->id << endl;
   cout << "name: " << this->name << endl;
   cout << "x: " << this->pos.x << endl;
   cout << "y: " << this->pos.y << endl;
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
   // timeLastMoved
   // pos
   // target
   int speed = 100; // pixels per second

   timespec curTS, diffTS;
   clock_gettime(CLOCK_REALTIME, &curTS);

   // get time elapsed
   diffTS.tv_sec = curTS.tv_sec - timeLastUpdated.tv_sec;
   diffTS.tv_nsec = curTS.tv_nsec - timeLastUpdated.tv_nsec;
   if (diffTS.tv_nsec < 0) {
      diffTS.tv_sec -= 1;
      diffTS.tv_nsec += 1000000000;
   }

   cout << "elapsed secs: " << diffTS.tv_sec << endl;
   cout << "elapsed nsecs: " << diffTS.tv_nsec << endl;

   // here we move 100 pixels per second
   float pixels = 100 * (diffTS.tv_sec+diffTS.tv_nsec/1000000000.0);
   cout << "We need to move " << pixels << "pixels" << endl;

   double angle = atan2(target.y-pos.y, target.x-pos.x);

   // we just need to check that we don't overjump the target
   pos.x += cos(angle)*pixels;
   pos.y += sin(angle)*pixels;

   timeLastUpdated.tv_sec = curTS.tv_sec;
   timeLastUpdated.tv_nsec = curTS.tv_nsec;
}
