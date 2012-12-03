#include "Player.h"

#include <iostream>
#include <arpa/inet.h>

using namespace std;

Player::Player(string name, string password)
{
   this->name = name;
   this->password = password;

   cout << "Created new player: " << this->name << endl;
}

Player::Player(string name, sockaddr_in addr)
{
   this->name = name;
   this->password = "";
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
