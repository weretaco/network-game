#ifndef _PLAYER_H
#define _PLAYER_H

#include <netinet/in.h>
#include <string>

using namespace std;

class player {
public:
   string name;
   sockaddr_in addr;

   player(string name, sockaddr_in addr);
   ~player();

   bool operator == (const player &p);
};

#endif
