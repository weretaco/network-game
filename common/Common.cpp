#include "Common.h"

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
