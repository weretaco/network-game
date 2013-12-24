#ifndef _COMMON_H
#define _COMMON_H

#include <string>

using namespace std;

struct FLOAT_POSITION;

struct POSITION {
   int x;
   int y;
   FLOAT_POSITION toFloat();
};

struct FLOAT_POSITION {
   float x;
   float y;
   POSITION toInt();
};

void error(const char *msg);
void set_nonblock(int sock);
unsigned long long getCurrentMillis();
string getCurrentDateTimeString();

POSITION screenToMap(POSITION pos);
POSITION mapToScreen(POSITION pos);
float posDistance(FLOAT_POSITION pos1, FLOAT_POSITION pos2);

#endif
