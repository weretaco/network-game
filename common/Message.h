#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MSG_TYPE_REGISTER     1
#define MSG_TYPE_LOGIN        2
#define MSG_TYPE_LOGOUT       3
#define MSG_TYPE_CHAT         4

typedef struct
{
   short type;
   char buffer[256];
} NETWORK_MSG;

int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);

int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);

#endif
