#ifndef _MESAGE_H
#define _MESSAGE__H

#define MSG_TYPE_SOMETHING 1000

typedef struct
{
	short type;
	char buffer[256];
} NETWORK_MSG;

#endif