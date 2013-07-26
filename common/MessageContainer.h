#ifndef _MESSAGE_CONTAINER_H
#define _MESSAGE_CONTAINER_H

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <netinet/in.h>
#endif

#define MSG_TYPE_ACK               1
#define MSG_TYPE_REGISTER          2
#define MSG_TYPE_LOGIN             3
#define MSG_TYPE_LOGOUT            4
#define MSG_TYPE_CHAT              5
#define MSG_TYPE_PLAYER            6  // server sends this to update player positions
#define MSG_TYPE_PLAYER_MOVE       7  // client sends this when a player wants to move
#define MSG_TYPE_OBJECT            8
#define MSG_TYPE_REMOVE_OBJECT     9
#define MSG_TYPE_PICKUP_FLAG       10
#define MSG_TYPE_DROP_FLAG         11
#define MSG_TYPE_SCORE             12
#define MSG_TYPE_START_ATTACK      13
#define MSG_TYPE_ATTACK            14
#define MSG_TYPE_PROJECTILE        15
#define MSG_TYPE_REMOVE_PROJECTILE 16

typedef struct
{
   unsigned int id;
   unsigned short type;
   char buffer[256];
} NETWORK_MSG;

class MessageContainer {
private:
   NETWORK_MSG msg;
   struct sockaddr_in clientAddr;
   bool isAcked;
   unsigned long long timeAcked;

public:
   MessageContainer();
   MessageContainer(const MessageContainer& mc);
   MessageContainer(NETWORK_MSG msg, struct sockaddr_in clientAddr);
   ~MessageContainer();

   bool getAcked();
   unsigned long long getTimeAcked();
   NETWORK_MSG* getMessage();

   void setAcked(bool acked);
   void setTimeAcked(unsigned long long time);
};

#endif
