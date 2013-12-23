#include "GameSummary.h"

#include "Common.h"

using namespace std;

GameSummary::GameSummary() {
   this->gameName = "";
   this->winner = 0;
   this->blueScore = 0;
   this->redScore = 0;
}

GameSummary::GameSummary(string name, unsigned int winner, unsigned int blueScore, unsigned int redScore) {
   this->gameName = name;
   this->winner = winner;
   this->blueScore = blueScore;
   this->redScore = redScore;
}

GameSummary::~GameSummary() {
}

string GameSummary::getName() {
   return this->gameName;
}

unsigned int GameSummary::getWinner() {
   return this->winner;
}

unsigned int GameSummary::getBlueScore() {
   return this->blueScore;
}

unsigned int GameSummary::getRedScore() {
   return this->redScore;
}
