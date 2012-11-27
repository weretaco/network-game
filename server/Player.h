#ifndef _PLAYER_H
#define _PLAYER_H

#include <netinet/in.h>
#include <string>

using namespace std;

class Player {
public:
   Player(string name, string password);
   Player(string name, sockaddr_in addr); // this will be deleted
   ~Player();

   bool operator == (const Player &p);

   void setAddr(sockaddr_in addr);

   string name;
   string password;
   sockaddr_in addr;
};

#endif
