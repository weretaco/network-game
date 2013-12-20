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

bool Game::startPlayerMovement(unsigned int id, int x, int y) {
   // need to check if players actually contains the id
   Player* p = players[id];

   // we need to make sure the player can move here
   if (0 <= x && x < this->worldMap->width*25 &&
       0 <= y && y < this->worldMap->height*25 &&
       this->worldMap->getElement(x/25, y/25) == WorldMap::TERRAIN_GRASS)
   {
      p->target.x = x;
      p->target.y = y;

      p->isChasing = false;
      p->isAttacking = false;

      return true;
   }
   else
      return false;
}

// returns true if the movement should be canceled
bool Game::processPlayerMovement(Player* p, FLOAT_POSITION oldPos) {

   // check if the move needs to be canceled
   switch(this->worldMap->getElement(p->pos.x/25, p->pos.y/25))
   {
      case WorldMap::TERRAIN_NONE:
      case WorldMap::TERRAIN_OCEAN:
      case WorldMap::TERRAIN_ROCK:
      {
         p->pos = oldPos;
         p->target.x = p->pos.x;
         p->target.y = p->pos.y;
         p->isChasing = false;
         return true;
         break;
      }
      default:
         // if there are no obstacles, don't cancel movement
         return false;
         break;
      }
}

void Game::setRedScore(int score) {
   this->redScore = score;
}

void Game::setBlueScore(int score) {
   this->blueScore = score;
}
