#ifndef _MESSAGE_CONTAINER_H
#define _MESSAGE_CONTAINER_H

#include <string>

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
#elif defined LINUX
   #include <netinet/in.h>
#endif

using namespace std;

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
#define MSG_TYPE_ATTACK            13
#define MSG_TYPE_PROJECTILE        14
#define MSG_TYPE_REMOVE_PROJECTILE 15
#define MSG_TYPE_CREATE_GAME       16
#define MSG_TYPE_JOIN_GAME         17
#define MSG_TYPE_LEAVE_GAME        18
#define MSG_TYPE_GAME_INFO         19
#define MSG_TYPE_JOIN_GAME_SUCCESS 20
#define MSG_TYPE_JOIN_GAME_FAILURE 21
#define MSG_TYPE_JOIN_GAME_ACK     22
#define MSG_TYPE_PLAYER_JOIN_GAME  23
#define MSG_TYPE_FINISH_GAME       24

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

   static string getMsgTypeString(int msgType) {
      switch(msgType) {
         case MSG_TYPE_ACK: return "MSG_TYPE_ACK";
         case MSG_TYPE_REGISTER: return "MSG_TYPE_REGISTER";
         case MSG_TYPE_LOGIN: return "MSG_TYPE_LOGIN";
         case MSG_TYPE_LOGOUT: return "MSG_TYPE_LOGOUT";
         case MSG_TYPE_CHAT: return "MSG_TYPE_CHAT";
         case MSG_TYPE_PLAYER: return "MSG_TYPE_PLAYER";
         case MSG_TYPE_PLAYER_MOVE: return "MSG_TYPE_PLAYER_MOVE";
         case MSG_TYPE_OBJECT: return "MSG_TYPE_OBJECT";
         case MSG_TYPE_REMOVE_OBJECT: return "MSG_TYPE_REMOVE_OBJECT";
         case MSG_TYPE_PICKUP_FLAG: return "MSG_TYPE_PICKUP_FLAG";
         case MSG_TYPE_DROP_FLAG: return "MSG_TYPE_DROP_FLAG";
         case MSG_TYPE_SCORE: return "MSG_TYPE_SCORE";
         case MSG_TYPE_ATTACK: return "MSG_TYPE_ATTACK";
         case MSG_TYPE_PROJECTILE: return "MSG_TYPE_PROJECTILE";
         case MSG_TYPE_REMOVE_PROJECTILE: return "MSG_TYPE_REMOVE_PROJECTILE";
         case MSG_TYPE_CREATE_GAME: return "MSG_TYPE_CREATE_GAME";
         case MSG_TYPE_JOIN_GAME: return "MSG_TYPE_JOIN_GAME";
         case MSG_TYPE_LEAVE_GAME: return "MSG_TYPE_LEAVE_GAME";
         case MSG_TYPE_GAME_INFO: return "MSG_TYPE_GAME_INFO";
         case MSG_TYPE_JOIN_GAME_SUCCESS: return "MSG_TYPE_JOIN_GAME_SUCCESS";
         case MSG_TYPE_JOIN_GAME_FAILURE: return "MSG_TYPE_JOIN_GAME_FAILURE";
         case MSG_TYPE_JOIN_GAME_ACK: return "MSG_TYPE_JOIN_GAME_ACK";
         case MSG_TYPE_PLAYER_JOIN_GAME: return "MSG_TYPE_PLAYER_JOIN_GAME";
         case MSG_TYPE_FINISH_GAME: return "MSG_TYPE_FINISH_GAME";
         default: return "Unknown";
      }
   }
};

#endif
