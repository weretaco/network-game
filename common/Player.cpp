#include "Player.h"

#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

Player::Player()
{
   this->id = 0;
   this->name = "";
   this->password = "";
   this->pos.x = 0;
   this->pos.y = 0;
}

Player::Player(const Player& p)
{
   this->id = p.id;
   this->name = p.name;
   this->password = p.password;
   this->pos.x = p.pos.x;
   this->pos.y = p.pos.y;
   this->addr = p.addr;
}

Player::Player(string name, string password)
{
   this->id = 0;
   this->name = name;
   this->password = password;
   this->pos.x = 200;
   this->pos.y = 200;
}

Player::Player(string name, sockaddr_in addr)
{
   this->id = 0;
   this->name = name;
   this->password = "";
   this->pos.x = 200;
   this->pos.y = 200;
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
