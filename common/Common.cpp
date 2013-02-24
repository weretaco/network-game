#include "Common.h"

#include <iostream>
using namespace std;

#if defined WINDOWS
   #include <Windows.h>
#elif defined LINUX
   #include <time.h>
#endif

void set_nonblock(int sock)
{
   #if defined WINDOWS
      unsigned long mode = 1;
      ioctlsocket(sock, FIONBIO, &mode);
   #elif defined LINUX
      int flags;
      flags = fcntl(sock, F_GETFL,0);
      assert(flags != -1);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
   #endif
}

unsigned long long getCurrentMillis()
{
   unsigned long long numMilliseconds;

   #if defined WINDOWS
      numMilliseconds = GetTickCount();
   #elif defined LINUX
      timespec curTime;
      clock_gettime(CLOCK_REALTIME, &curTime);

      numMilliseconds = curTime.tv_sec*(unsigned long long)1000+curTime.tv_nsec/(unsigned long long)1000000;
   #endif

   return numMilliseconds;
}
