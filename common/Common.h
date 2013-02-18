#ifndef _COMMON_H
#define _COMMON_H

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
   #include <WS2tcpip.h>
#elif defined LINUX
   #include <fcntl.h>
   #include <assert.h>
#endif

void set_nonblock(int sock);

typedef struct
{
   int x;
   int y;
} POSITION;

typedef struct
{
   float x;
   float y;
} FLOAT_POSITION;

#endif
