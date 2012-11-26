#include "player.h"

#include <iostream>
#include <arpa/inet.h>

using namespace std;

player::player(string name, sockaddr_in addr)
{
   this->name = name;
   this->addr = addr;

   cout << "Created new played: " << this->name << endl;
}

player::~player()
{
}

bool player::operator == (const player &p)
{
   bool eq = addr.sin_addr.s_addr == p.addr.sin_addr.s_addr;

   return eq;
}
