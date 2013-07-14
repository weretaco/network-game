#ifndef _MESSAGE_H
#define _MESSAGE_H

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

int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);

int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *source);

#endif
