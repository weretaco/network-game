#include "Common.h"

#include "Compiler.h"

#if defined WINDOWS
   #include <winsock2.h>
#endif

#include <sstream>
#include <cmath>
#include <ctime>

using namespace std;

FLOAT_POSITION POSITION::toFloat() {
   FLOAT_POSITION floatPosition;
   floatPosition.x = x;
   floatPosition.y = y;

   return floatPosition;
}

POSITION FLOAT_POSITION::toInt() {
   POSITION position;
   position.x = x;
   position.y = y;

   return position;
}

void set_nonblock(int sock)
{
   #if defined WINDOWS
      unsigned long mode = 1;
      ioctlsocket(sock, FIONBIO, &mode);
   #elif defined LINUX
      int flags = fcntl(sock, F_GETFL,0);
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

string getCurrentDateTimeString() {
   ostringstream timeString;

   time_t millis = time(NULL);
   struct tm *time = localtime(&millis);

   timeString << time->tm_hour << ":" << time->tm_min << ":"<< time->tm_sec << " " << (time->tm_mon+1) << "/" << time->tm_mday << "/" << (time->tm_year+1900);

   return timeString.str();
}

float posDistance(FLOAT_POSITION pos1, FLOAT_POSITION pos2) {
   float xDiff = pos2.x - pos1.x;
   float yDiff = pos2.y - pos1.y;

   return sqrt( pow(xDiff,2) + pow(yDiff,2) );   
}

POSITION screenToMap(POSITION pos)
{
   pos.x = pos.x-300;
   pos.y = pos.y-100;

   if (pos.x < 0 || pos.y < 0)
   {
      pos.x = -1;
      pos.y = -1;
   }

   return pos;
}

POSITION mapToScreen(POSITION pos)
{
   pos.x = pos.x+300;
   pos.y = pos.y+100;

   return pos;
}
