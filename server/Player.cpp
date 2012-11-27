#include "Player.h"

#include <iostream>
#include <arpa/inet.h>

using namespace std;

Player::Player(string name, sockaddr_in addr)
{
   this->name = name;
   this->addr = addr;

   cout << "Created new played: " << this->name << endl;
}

Player::~Player()
{
}

// was meant for the find find function. Currently unused
bool Player::operator == (const Player &p)
{
   bool eq = addr.sin_addr.s_addr == p.addr.sin_addr.s_addr;

   return eq;
}
