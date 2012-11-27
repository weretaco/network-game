#ifndef _PLAYER_H
#define _PLAYER_H

#include <netinet/in.h>
#include <string>

using namespace std;

class Player {
public:
   Player(string name, sockaddr_in addr);
   ~Player();

   bool operator == (const Player &p);

   string name;
   sockaddr_in addr;
};

#endif
