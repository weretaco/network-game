#ifndef _MESSAGE_CONTAINER_H
#define _MESSAGE_CONTAINER_H

#include <string>

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
#elif defined LINUX
   #include <netinet/in.h>
#elif defined MAC
   #include <netinet/in.h>
#endif

using namespace std;

enum MessageType {
   MSG_TYPE_ACK = 1,
   MSG_TYPE_REGISTER,
   MSG_TYPE_LOGIN,
   MSG_TYPE_LOGOUT,
   MSG_TYPE_CHAT,
   MSG_TYPE_PLAYER,
   MSG_TYPE_PLAYER_MOVE,
   MSG_TYPE_OBJECT,
   MSG_TYPE_REMOVE_OBJECT,
   MSG_TYPE_PICKUP_FLAG,
   MSG_TYPE_DROP_FLAG,
   MSG_TYPE_SCORE,
   MSG_TYPE_ATTACK,
   MSG_TYPE_PROJECTILE,
   MSG_TYPE_REMOVE_PROJECTILE,
   MSG_TYPE_PROFILE,
   MSG_TYPE_CREATE_GAME,
   MSG_TYPE_JOIN_GAME,
   MSG_TYPE_LEAVE_GAME,
   MSG_TYPE_GAME_INFO,
   MSG_TYPE_CREATE_GAME_FAILURE,
   MSG_TYPE_JOIN_GAME_SUCCESS,
   MSG_TYPE_JOIN_GAME_FAILURE,
   MSG_TYPE_JOIN_GAME_ACK,
   MSG_TYPE_PLAYER_JOIN_GAME,
   MSG_TYPE_FINISH_GAME,
   MSG_TYPE_JOIN_TEAM,
   MSG_TYPE_START_GAME
};

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

   static string getMsgTypeString(int msgType);
};

#endif
