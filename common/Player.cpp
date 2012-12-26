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
   oss.write("\0", 1);
   oss.write((char*)&(this->pos.x), sizeof(int));
   oss.write((char*)&(this->pos.y), sizeof(int));

   memcpy(buffer, oss.str().c_str(), this->name.length()+1+2*sizeof(int));
}

void Player::deserialize(char* buffer)
{
   istringstream iss;
   iss.str(buffer);

   iss >> this->name;
   iss.read((char*)&(this->pos.x), sizeof(int));
   iss.read((char*)&(this->pos.y), sizeof(int));
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}
