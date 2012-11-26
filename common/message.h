#ifndef _MESAGE_H
#define _MESSAGE_H

#define MSG_TYPE_LOGIN  1
#define MSG_TYPE_CHAT   2

typedef struct
{
   short type;
   char buffer[256];
} NETWORK_MSG;

int sendMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);

int receiveMessage(NETWORK_MSG *msg, int sock, struct sockaddr_in *dest);

#endif
