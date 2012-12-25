#include "Common.h"

void set_nonblock(int sock)
{
   #ifdef WIN32
      unsigned long mode = 1;
      ioctlsocket(sock, FIONBIO, &mode);
   #else
      int flags;
      flags = fcntl(sock, F_GETFL,0);
      assert(flags != -1);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
   #endif
}
