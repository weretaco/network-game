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

#include <string>

using namespace std;

typedef struct
{
   float x;
   float y;
} FLOAT_POSITION;

typedef struct
{
   int x;
   int y;
   FLOAT_POSITION toFloat() {
      FLOAT_POSITION floatPosition;
      floatPosition.x = x;
      floatPosition.y = y;

      return floatPosition;
   }
} POSITION;

void set_nonblock(int sock);
unsigned long long getCurrentMillis();
string getCurrentDateTimeString();
float posDistance(FLOAT_POSITION pos1, FLOAT_POSITION pos2);

#endif
