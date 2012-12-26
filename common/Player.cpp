#include "Player.h"

#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

Player::Player(string name, string password)
{
   this->name = name;
   this->password = password;
   this->pos.x = 200;
   this->pos.y = 200;

   cout << "Created new player: " << this->name << endl;
}

Player::Player(string name, sockaddr_in addr)
{
   this->name = name;
   this->password = "";
   this->pos.x = 200;
   this->pos.y = 200;
   this->addr = addr;

   cout << "Created new played: " << this->name << endl;
}

Player::~Player()
{
}

void Player::serialize(char* buffer)
{
   ostringstream oss;

   oss << this->name;

   memcpy(buffer, oss.str().c_str(), this->name.length()+1);
}

void Player::deserialize(char* buffer)
{
   istringstream iss;

   iss >> this->name;
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}
