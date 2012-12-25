#include "Player.h"

#include <iostream>
#include <arpa/inet.h>

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

void Player::setAddr(sockaddr_in addr)
{
   this->addr = addr;
}

void Player::clearSensitiveInfo()
{
   this->password = "";
   this->addr.sin_family = 0;
   this->addr.sin_port = 0;
   this->addr.sin_addr.s_addr = 0;
}
