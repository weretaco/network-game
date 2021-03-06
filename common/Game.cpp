#include "Game.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

#include "Common.h"

using namespace std;

Game::Game() {
   this->id = 0;
   this->name = "";
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = NULL;
   this->msgProcessor = NULL;
}

Game::Game(string name, string filepath, MessageProcessor* msgProcessor) {
   this->id = 0;
   this->name = name;
   this->blueScore = 0;
   this->redScore = 0;
   this->worldMap = WorldMap::loadMapFromFile(filepath);
   this->msgProcessor = msgProcessor;
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

bool Game::addPlayer(Player* p) {
   if (players.find(p->getId()) == players.end()) {
      players[p->getId()] = p;

      // reset player stats, location, etc.
      p->pos.x = p->target.x = 200;
      p->pos.y = p->target.y = 200;
      p->setTargetPlayer(0);
      p->isChasing = false;
      p->isAttacking = false;
      p->isDead = false;
      p->health = p->maxHealth;
      p->hasBlueFlag = false;
      p->hasRedFlag = false;

      p->currentGame = this;

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

map<unsigned int, Projectile>& Game::getProjectiles() {
   return this->projectiles;
}

bool Game::addProjectile(Projectile p) {
   if (projectiles.find(p.id) == projectiles.end()) {
      projectiles[p.id] = p;
      return true;
   }
   else
      return false;
}

bool Game::removeProjectile(unsigned int id) {
   if (projectiles.erase(id) == 1)
      return true;
   else
      return false;
}

unsigned int Game::getRedScore() {
   return this->redScore;
}

unsigned int Game::getBlueScore() {
   return this->blueScore;
}

WorldMap* Game::getMap() {
   return this->worldMap;
}

void Game::setId(unsigned int id) {
   this->id = id;
}

void Game::setRedScore(unsigned int score) {
   this->redScore = score;
}

void Game::setBlueScore(unsigned int score) {
   this->blueScore = score;
}

void Game::addObjectToMap(ObjectType objectType, int x, int y) {
   NETWORK_MSG serverMsg;

   this->getMap()->addObject(objectType, x, y);

   serverMsg.type = MSG_TYPE_OBJECT;
   this->worldMap->getObjects()->back().serialize(serverMsg.buffer);

   this->msgProcessor->broadcastMessage(serverMsg, this->players);
}

bool Game::startPlayerMovement(unsigned int id, int x, int y) {
   // need to check if players actually contains the id
   Player* p = players[id];

   // we need to make sure the player can move here
   if (0 <= x && x < this->worldMap->width*25 &&
       0 <= y && y < this->worldMap->height*25 &&
       this->worldMap->getElement(x/25, y/25) == TERRAIN_GRASS)
   {
      p->target.x = x;
      p->target.y = y;

      p->isChasing = false;
      p->isAttacking = false;
      p->setTargetPlayer(0);

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
      case TERRAIN_NONE:
      case TERRAIN_OCEAN:
      case TERRAIN_ROCK:
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

// returns the id of the picked-up flag or -1 if none was picked up
int Game::processFlagPickupRequest(Player* p) {
   vector<WorldMap::Object>* vctObjects = this->worldMap->getObjects();
   vector<WorldMap::Object>::iterator it;
   int itemId = -1;

   for (it = vctObjects->begin(); it != vctObjects->end(); it++) {
      if (posDistance(p->pos, it->pos.toFloat()) < 10) {
         switch (it->type) {
            case OBJECT_BLUE_FLAG:
               if (p->team == Player::TEAM_RED) {
                  p->hasBlueFlag = true;
                  itemId = it->id;
               }
               break;
            case OBJECT_RED_FLAG:
               if (p->team == Player::TEAM_BLUE) {
                  p->hasRedFlag = true;
                  itemId = it->id;
               }
               break;
            case OBJECT_NONE:
               break;
         }

         if (itemId > -1) {
            vctObjects->erase(it);
            return itemId;
         }
      }
   }

   return itemId;
}

void Game::dealDamageToPlayer(Player* p, int damage) {
   p->takeDamage(damage);

   if (p->isDead) {
      ObjectType flagType = OBJECT_NONE;
      if (p->hasBlueFlag)
         flagType = OBJECT_BLUE_FLAG;
      else if (p->hasRedFlag)
         flagType = OBJECT_RED_FLAG;

      if (flagType != OBJECT_NONE)
         this->addObjectToMap(flagType, p->pos.x, p->pos.y);
   }

   // send a PLAYER message after dealing damage
   NETWORK_MSG serverMsg;
   serverMsg.type = MSG_TYPE_PLAYER;
   p->serialize(serverMsg.buffer);
   msgProcessor->broadcastMessage(serverMsg, this->players);
}

void Game::assignProjectileId(Projectile* p) {
   p->id = unusedProjectileId;
   updateUnusedProjectileId();
}

void Game::updateUnusedProjectileId() {
   while (projectiles.find(unusedProjectileId) != projectiles.end())
      unusedProjectileId++;
}
