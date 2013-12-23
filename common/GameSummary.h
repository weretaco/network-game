#ifndef _GAME_SUMMARY_H
#define _GAME_SUMMARY_H

#include "Compiler.h"

#include <string>

using namespace std;

class GameSummary {
private:
   string gameName;
   unsigned int winner;
   unsigned int blueScore;
   unsigned int redScore;

public:
   GameSummary();
   GameSummary(string name, unsigned int winner, unsigned int blueScore, unsigned int redScore);

   ~GameSummary();

   string getName();
   unsigned int getWinner();
   unsigned int getBlueScore();
   unsigned int getRedScore();
};

#endif
