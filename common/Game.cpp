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

      // choose a random team (either 0 or 1)
      p->team = rand() % 2;

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
               if (p->team == 1) {
                  p->hasBlueFlag = true;
                  itemId = it->id;
               }
               break;
            case OBJECT_RED_FLAG:
               if (p->team == 0) {
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

   if (p->isDead)
   {
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

bool Game::handleGameEvents() {
   map<unsigned int, Player*>::iterator it;
   bool gameFinished = false;

   for (it = this->getPlayers().begin(); it != this->getPlayers().end(); it++)
   {
      gameFinished = gameFinished ||
         this->handlePlayerEvents(it->second);
   }

   if (gameFinished) {
      for (it = this->players.begin(); it != this->players.end(); it++)
      {
         it->second->currentGame = NULL;
      }
   }

   return gameFinished;
}

bool Game::handlePlayerEvents(Player* p) {
   NETWORK_MSG serverMsg;
   FLOAT_POSITION oldPos;
   bool gameFinished = false;
   bool broadcastMove = false;

   cout << "moving player" << endl;

   // move player and perform associated tasks
   oldPos = p->pos;
   if (p->move(this->worldMap)) {

      cout << "player moved" << endl;
      if (this->processPlayerMovement(p, oldPos))
         broadcastMove = true;
      cout << "player move processed" << endl;

      ObjectType flagType;
      POSITION pos;
      bool flagTurnedIn = false;
      bool flagReturned = false;
      bool ownFlagAtBase = false;

      switch(this->worldMap->getStructure(p->pos.x/25, p->pos.y/25))
      {
         case STRUCTURE_BLUE_FLAG:
         {
            if (p->team == 0 && p->hasRedFlag)
            {
               // check that your flag is at your base
               pos = this->worldMap->getStructureLocation(STRUCTURE_BLUE_FLAG);
                           
               vector<WorldMap::Object>* vctObjects = this->worldMap->getObjects();
               vector<WorldMap::Object>::iterator itObjects;

               for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
               {
                  if (itObjects->type == OBJECT_BLUE_FLAG)
                  {
                     if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12)
                     {
                        ownFlagAtBase = true;
                        break;
                     }
                  }
               }

               if (ownFlagAtBase)
               {
                  p->hasRedFlag = false;
                  flagType = OBJECT_RED_FLAG;
                  pos = this->worldMap->getStructureLocation(STRUCTURE_RED_FLAG);
                  flagTurnedIn = true;
                  this->blueScore++;
               }
            }

            break;
         }
         case STRUCTURE_RED_FLAG:
         {
            if (p->team == 1 && p->hasBlueFlag)
            {
               // check that your flag is at your base
               pos = this->worldMap->getStructureLocation(STRUCTURE_RED_FLAG);
                        
               vector<WorldMap::Object>* vctObjects = this->worldMap->getObjects();
               vector<WorldMap::Object>::iterator itObjects;

               for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
               {
                  if (itObjects->type == OBJECT_RED_FLAG)
                  {
                     if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12)
                     {
                        ownFlagAtBase = true;
                        break;
                     }
                  }
               }

               if (ownFlagAtBase)
               {
                  p->hasBlueFlag = false;
                  flagType = OBJECT_BLUE_FLAG;
                  pos = this->worldMap->getStructureLocation(STRUCTURE_BLUE_FLAG);
                  flagTurnedIn = true;
                  this->redScore++;
               }
            }

            break;
         }
         default:
         {
            break;
         }
      }

      if (flagTurnedIn)
      {
         unsigned int blueScore = this->blueScore;
         unsigned int redScore = this->redScore;

         pos.x = pos.x*25+12;
         pos.y = pos.y*25+12;
         this->addObjectToMap(flagType, pos.x, pos.y);

         serverMsg.type = MSG_TYPE_SCORE;
         memcpy(serverMsg.buffer, &blueScore, 4);
         memcpy(serverMsg.buffer+4, &redScore, 4);
         msgProcessor->broadcastMessage(serverMsg, this->players);

         // check to see if the game should end
         // move to its own method
         if (this->blueScore == 3 || this->redScore == 3) {
            gameFinished = true;

            unsigned int winningTeam;
            if (this->blueScore == 3)
               winningTeam = 0;
            else if (this->redScore == 3)
               winningTeam = 1;

            serverMsg.type = MSG_TYPE_FINISH_GAME;

            // I should create an instance of the GameSummary object here and just serialize it into this message
            memcpy(serverMsg.buffer, &winningTeam, 4);
            memcpy(serverMsg.buffer+4, &blueScore, 4);
            memcpy(serverMsg.buffer+8, &redScore, 4);
            strcpy(serverMsg.buffer+12, this->getName().c_str());
            msgProcessor->broadcastMessage(serverMsg, this->players);
         }

         // this means a PLAYER message will be sent
         broadcastMove = true;
      }

      // go through all objects and check if the player is close to one and if its their flag
      vector<WorldMap::Object>* vctObjects = this->worldMap->getObjects();
      vector<WorldMap::Object>::iterator itObjects;
      POSITION structPos;

      for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
      {
         POSITION pos = itObjects->pos;

         if (posDistance(p->pos, pos.toFloat()) < 10)
         {
            if (p->team == 0 && 
                itObjects->type == OBJECT_BLUE_FLAG)
            {
               structPos = this->worldMap->getStructureLocation(STRUCTURE_BLUE_FLAG);
               flagReturned = true;
               break;
            }
            else if (p->team == 1 &&
                     itObjects->type == OBJECT_RED_FLAG)
            {
               structPos = this->worldMap->getStructureLocation(STRUCTURE_RED_FLAG);
               flagReturned = true;
               break;
            }
         }
      }

      if (flagReturned)
      {
         itObjects->pos.x = structPos.x*25+12;
         itObjects->pos.y = structPos.y*25+12;

         serverMsg.type = MSG_TYPE_OBJECT;
         itObjects->serialize(serverMsg.buffer);
         msgProcessor->broadcastMessage(serverMsg, this->players);
      }

      if (broadcastMove)
      {
         serverMsg.type = MSG_TYPE_PLAYER;
         p->serialize(serverMsg.buffer);
         msgProcessor->broadcastMessage(serverMsg, this->players);
      }
   }

   cout << "processing player attack" << endl;

   // check if the player's attack animation is complete
   if (p->isAttacking && p->timeAttackStarted+p->attackCooldown <= getCurrentMillis())
   {
      p->isAttacking = false;
      cout << "Attack animation is complete" << endl;

      //send everyone an ATTACK message
      cout << "about to broadcast attack" << endl;

      if (p->attackType == Player::ATTACK_MELEE)
      {
         cout << "Melee attack" << endl;

         Player* target = players[p->getTargetPlayer()];
         this->dealDamageToPlayer(target, p->damage);
      }
      else if (p->attackType == Player::ATTACK_RANGED)
      {
         cout << "Ranged attack" << endl;

         Projectile proj(p->pos.x, p->pos.y, p->getTargetPlayer(), p->damage);
         this->assignProjectileId(&proj);
         this->addProjectile(proj);

         int x = p->pos.x;
         int y = p->pos.y;
         unsigned int targetId = p->getTargetPlayer();

         serverMsg.type = MSG_TYPE_PROJECTILE;
         memcpy(serverMsg.buffer, &proj.id, 4);
         memcpy(serverMsg.buffer+4, &x, 4);
         memcpy(serverMsg.buffer+8, &y, 4);
         memcpy(serverMsg.buffer+12, &targetId, 4);
         msgProcessor->broadcastMessage(serverMsg, players);
      }
      else
         cout << "Invalid attack type: " << p->attackType << endl;
   }

   return gameFinished;
}

void Game::assignProjectileId(Projectile* p) {
   p->id = unusedProjectileId;
   updateUnusedProjectileId();
}

void Game::updateUnusedProjectileId() {
   while (projectiles.find(unusedProjectileId) != projectiles.end())
      unusedProjectileId++;
}
