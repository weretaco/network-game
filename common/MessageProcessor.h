#ifndef _MESSAGE_PROCESSOR_H
#define _MESSAGE_PROCESSOR_H

#include "Message.h"

/*
#define MSG_TYPE_REGISTER          1
#define MSG_TYPE_LOGIN             2
#define MSG_TYPE_LOGOUT            3
#define MSG_TYPE_CHAT              4
#define MSG_TYPE_PLAYER            5  // server sends this to update player positions
#define MSG_TYPE_PLAYER_MOVE       6  // client sends this when a player wants to move
#define MSG_TYPE_OBJECT            7
#define MSG_TYPE_REMOVE_OBJECT     8
#define MSG_TYPE_PICKUP_FLAG       9
#define MSG_TYPE_DROP_FLAG         10
#define MSG_TYPE_SCORE             11
#define MSG_TYPE_START_ATTACK      12
#define MSG_TYPE_ATTACK            13
#define MSG_TYPE_PROJECTILE        14
#define MSG_TYPE_REMOVE_PROJECTILE 15

typedef struct
{
   short type;
   char buffer[256];
} NETWORK_MSG;
*/

class MessageProcessor {
public:
   int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);
   int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);
   void resendUnackedMessages();
   void cleanAckedMessages();

private:
};

#endif
