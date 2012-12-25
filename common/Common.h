#ifndef _COMMON_H
#define _COMMON_H

#include <fcntl.h>
#include <assert.h>

void set_nonblock(int sock);

typedef struct
{
   int x;
   int y;
} PLAYER_POS;

#endif
