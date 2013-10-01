#include "Game.h"

using namespace std;

Game::Game() {
   this->id = 0;
   this->name = "";
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = NULL;
}

Game::Game(string name, string filepath) {
   this->id = 0;
   this->name = name;
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = WorldMap::loadMapFromFile(filepath);
}

Game::~Game() {
   delete this->worldMap;
}

string Game::getName() {
   return this->name;
}

int Game::getNumPlayers() {
   return this->players.size();
}

map<unsigned int, Player*>& Game::getPlayers() {
   return this->players;
}

int Game::getRedScore() {
   return this->redScore;
}

int Game::getBlueScore() {
   return this->blueScore;
}

WorldMap* Game::getMap() {
   return this->worldMap;
}

void Game::setId(unsigned int id) {
   this->id = id;
}

bool Game::addPlayer(Player* p) {
   if (players.find(p->id) == players.end()) {
      players[p->id] = p;
      return true;
   }
   else
      return false;
}

bool Game::removePlayer(unsigned int id) {
   if (players.erase(id) == 1)
      return true;
   else
      return false;
}

void Game::setRedScore(int score) {
   this->redScore = score;
}

void Game::setBlueScore(int score) {
   this->blueScore = score;
}
