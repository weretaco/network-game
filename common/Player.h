#ifndef _PLAYER_H
#define _PLAYER_H

#include <netinet/in.h>
#include <string>

#include "Common.h"

using namespace std;

class Player {
public:
   Player(string name, string password);
   Player(string name, sockaddr_in addr); // this will be deleted
   ~Player();

   void setAddr(sockaddr_in addr);
   void clearSensitiveInfo();

   string name;
   string password;
   sockaddr_in addr;
   PLAYER_POS pos;
};

#endif
