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
   ostringstream oss;

   cout << "Player name: " << this->name << endl;

   /*
   oss.write((char*)&(this->id), sizeof(int));
   oss << this->name;
   cout << "first oss str: " << oss.str() << endl;
   oss.write("\0", 1);
   cout << "second oss str: " << oss.str() << endl;
   oss.write((char*)&(this->pos.x), sizeof(int));
   cout << "third oss str: " << oss.str() << endl;
   oss.write((char*)&(this->pos.y), sizeof(int));
   */

   oss << this->id;
   oss << this->name;
   css << this->pos.x;
   css << this->pos.y;

   memcpy(buffer, oss.str().c_str(), oss.str().length);
}

void Player::deserialize(char* buffer)
{
   istringstream iss;
   iss.str(buffer);

   /*
   iss.read((char*)&(this->id), sizeof(int));
   iss >> this->name;
   iss.read((char*)&(this->pos.x), sizeof(int));
   iss.read((char*)&(this->pos.y), sizeof(int));
   */

   iss >> this.id;
   iss >> this->name;
   iss >> this->pos.x;
   iss >> this->pos.y;
}

void Player::setId(int id)
{
   this->id = id;
}

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}
