#include "Game.h"

using namespace std;

Game::Game() {
   this->id = 0;
   this->name = "";
}

Game::Game(string name) {
   this->id = 0;
   this->name = name;
}

Game::~Game() {
}

void Game::setId(int id) {
   this->id = id;
}

int Game::getNumPlayers() {
   return players.size();
}

void Game::setNumPlayers(int numPlayers) {
   int numCurPlayers = this->getNumPlayers();
   int numNewPlayers = numPlayers-numCurPlayers;

   for (int i=0; i<numNewPlayers; i++)
      this->players[numCurPlayers+i] = NULL;
}

bool Game::addPlayer(Player* p) {
   if (players.count(p->id) == 0) {
      players[p->id] = p;
      return true;
   }
   else
      return false;
}

bool Game::removePlayer(int id) {
   if (players.erase(id) == 1)
      return true;
   else
      return false;
}
