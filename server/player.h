#ifndef _PLAYER_H
#define _PLAYER_H

#include <netinet/in.h>
#include <string>

using namespace std;

class player {
private:
   sockaddr_in addr;

public:
   string name;

   player(string name, sockaddr_in addr);
   ~player();

   bool operator == (const player &p);
};

#endif
