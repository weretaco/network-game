#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cmath>

#include <vector>
#include <map>

#include <csignal>

#include <sys/time.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <crypt.h>

/*
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
*/

#include "../common/Compiler.h"
#include "../common/Common.h"
#include "../common/MessageProcessor.h"
#include "../common/WorldMap.h"
#include "../common/Player.h"
#include "../common/Projectile.h"
#include "../common/Game.h"
#include "../common/GameSummary.h"

#include "DataAccess.h"

using namespace std;

bool done;

// from used to be const. Removed that so I could take a reference
// and use it to send messages
void processMessage(const NETWORK_MSG &clientMsg, struct sockaddr_in &from, MessageProcessor &msgProcessor, map<unsigned int, Player*>& mapPlayers, map<string, Game*>& mapGames, WorldMap* gameMap, unsigned int& unusedPlayerId, NETWORK_MSG &serverMsg, int &scoreBlue, int &scoreRed);

void broadcastMessage(MessageProcessor &msgProcessor, NETWORK_MSG &serverMsg, map<unsigned int, Player*>& players);
void updateUnusedPlayerId(unsigned int& id, map<unsigned int, Player*>& mapPlayers);
Player *findPlayerByName(map<unsigned int, Player*> &m, string name);
Player *findPlayerByAddr(map<unsigned int, Player*> &m, const sockaddr_in &addr);
void damagePlayer(Player *p, int damage);

void addObjectToMap(WorldMap::ObjectType objectType, int x, int y, WorldMap* gameMap, map<unsigned int, Player*>& mapPlayers, MessageProcessor &msgProcessor);

void quit(int sig) {
   done = true;
}

int main(int argc, char *argv[])
{
   int sock, length;
   struct sockaddr_in server;
   struct sockaddr_in from; // info of client sending the message
   NETWORK_MSG clientMsg, serverMsg;
   MessageProcessor msgProcessor;
   map<unsigned int, Player*> mapPlayers;
   map<unsigned int, Projectile> mapProjectiles;
   map<string, Game*> mapGames;
   unsigned int unusedPlayerId = 1, unusedProjectileId = 1;
   int scoreBlue, scoreRed;
   ofstream outputLog;

   done = false;

   scoreBlue = 0;
   scoreRed = 0;

   signal(SIGINT, quit);

   //SSL_load_error_strings();
   //ERR_load_BIO_strings();
   //OpenSSL_add_all_algorithms();

   if (argc != 2)
   {
      cerr << "ERROR, expected server [domain] [port]" << endl;
      exit(1);
   }

   outputLog.open("server.log", ios::app);
   outputLog << "Started server on " << getCurrentDateTimeString() << endl;

   WorldMap* gameMap = WorldMap::loadMapFromFile("../data/map.txt");

   // add some items to the map. They will be sent out
   // to players when they login
   for (int y=0; y<gameMap->height; y++)
   {
      for (int x=0; x<gameMap->width; x++)
      {
         switch (gameMap->getStructure(x, y))
         {
            case WorldMap::STRUCTURE_BLUE_FLAG:
               gameMap->addObject(WorldMap::OBJECT_BLUE_FLAG, x*25+12, y*25+12);
               break;
            case WorldMap::STRUCTURE_RED_FLAG:
               gameMap->addObject(WorldMap::OBJECT_RED_FLAG, x*25+12, y*25+12);
               break;
         }
      }
   }

   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0)
      error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_port=htons(atoi(argv[1]));
   server.sin_addr.s_addr=INADDR_ANY;
   if ( bind(sock, (struct sockaddr *)&server, length) < 0 ) 
      error("binding");

   set_nonblock(sock);

   msgProcessor = MessageProcessor(sock, &outputLog);

   timespec ts;
   int timeLastUpdated = 0, curTime = 0, timeLastBroadcast = 0;
   while (!done)
   {
      usleep(5000);

      clock_gettime(CLOCK_REALTIME, &ts);
      // make the number smaller so millis can fit in an int
      ts.tv_sec -= 1368000000;
      curTime = ts.tv_sec*1000 + ts.tv_nsec/1000000;

      if (timeLastUpdated == 0 || (curTime-timeLastUpdated) >= 50)
      {
         timeLastUpdated = curTime;

         msgProcessor.cleanAckedMessages();
         msgProcessor.resendUnackedMessages();

         map<unsigned int, Player*>::iterator it;

         cout << "Updating player targets and respawning dead players" << endl;

         // set targets for all chasing players (or make them attack if they're close enough)
         // this should be moved into the games loop
         for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
         {
            Player* p = it->second;

            // check if it's time to revive dead players
            if (p->isDead)
            {
               cout << "Player is dead" << endl;

               if (getCurrentMillis() - p->timeDied >= 10000)
               {
                  p->isDead = false;

                  POSITION spawnPos;
 
                  switch (p->team)
                  {
                  case 0:// blue team
                     spawnPos = p->currentGame->getMap()->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                     break;
                  case 1:// red team
                     spawnPos = p->currentGame->getMap()->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
                     break;
                  default:
                     // should never go here
                     cout << "Error: Invalid team" << endl;
                     break;
                  }

                  // spawn the player to the right of their flag location
                  spawnPos.x = (spawnPos.x+1) * 25 + 12;
                  spawnPos.y = spawnPos.y * 25 + 12;

                  p->pos = spawnPos.toFloat();
                  p->target = spawnPos;
                  p->health = p->maxHealth;

                  serverMsg.type = MSG_TYPE_PLAYER;
                  p->serialize(serverMsg.buffer);

                  broadcastMessage(msgProcessor, serverMsg, p->currentGame->getPlayers());
               }

               continue;
            }

            if (p->currentGame != NULL) {
               map<unsigned int, Player*> playersInGame = p->currentGame->getPlayers();
               if (p->updateTarget(playersInGame))
               {
                  serverMsg.type = MSG_TYPE_PLAYER;
                  p->serialize(serverMsg.buffer);

                  broadcastMessage(msgProcessor, serverMsg, playersInGame);
               }
            }
         }

         cout << "Processing players in a game" << endl;

         // process players currently in a game
         FLOAT_POSITION oldPos;
         map<string, Game*>::iterator itGames;
         Game* game = NULL;
         WorldMap* gameMap = NULL;
         bool gameFinished;

         for (itGames = mapGames.begin(); itGames != mapGames.end();) { 
            game = itGames->second;
            gameMap = game->getMap();
            map<unsigned int, Player*>& playersInGame = game->getPlayers();
            gameFinished = false;

            for (it = game->getPlayers().begin(); it != game->getPlayers().end(); it++)
            {
               Player* p = it->second;

               cout << "moving player" << endl;
               bool broadcastMove = false;

               // xompute playersInGame here

               // move player and perform associated tasks
               oldPos = p->pos;
               if (p->move(gameMap)) {

                  cout << "player moved" << endl;
                  if (game->processPlayerMovement(p, oldPos))
                      broadcastMove = true;
                  cout << "player move processed" << endl;

                  WorldMap::ObjectType flagType;
                  POSITION pos;
                  bool flagTurnedIn = false;
                  bool flagReturned = false;
                  bool ownFlagAtBase = false;

                  // need to figure out how to move this to a different file
                  // while still sending back flag type and position
                  switch(gameMap->getStructure(p->pos.x/25, p->pos.y/25))
                  {
                     case WorldMap::STRUCTURE_BLUE_FLAG:
                     {
                        if (p->team == 0 && p->hasRedFlag)
                        {
                           // check that your flag is at your base
                           pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                           
                           vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
                           vector<WorldMap::Object>::iterator itObjects;

                           for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
                           {
                              if (itObjects->type == WorldMap::OBJECT_BLUE_FLAG)
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
                              flagType = WorldMap::OBJECT_RED_FLAG;
                              pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
                              flagTurnedIn = true;
                              scoreBlue++;
                           }
                        }

                        break;
                     }
                     case WorldMap::STRUCTURE_RED_FLAG:
                     {
                        if (p->team == 1 && p->hasBlueFlag)
                        {
                           // check that your flag is at your base
                           pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
                        
                           vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
                           vector<WorldMap::Object>::iterator itObjects;

                           for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
                           {
                              if (itObjects->type == WorldMap::OBJECT_RED_FLAG)
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
                              flagType = WorldMap::OBJECT_BLUE_FLAG;
                              pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                              flagTurnedIn = true;
                              scoreRed++;
                           }
                        }

                        break;
                     }
                  }

                  if (flagTurnedIn)
                  {
                     // send an OBJECT message to add the flag back to its spawn point
                     pos.x = pos.x*25+12;
                     pos.y = pos.y*25+12;
                     gameMap->addObject(flagType, pos.x, pos.y);

                     serverMsg.type = MSG_TYPE_OBJECT;
                     gameMap->getObjects()->back().serialize(serverMsg.buffer);
                     broadcastMessage(msgProcessor, serverMsg, playersInGame);

                     serverMsg.type = MSG_TYPE_SCORE;
                     memcpy(serverMsg.buffer, &scoreBlue, 4);
                     memcpy(serverMsg.buffer+4, &scoreRed, 4);
                     broadcastMessage(msgProcessor, serverMsg, playersInGame);

                     // check to see if the game should end
                     // move to its own method
                     if (scoreBlue == 3 || scoreRed == 3) {
                        gameFinished = true;

                        unsigned int winningTeam;
                        if (scoreBlue == 3)
                           winningTeam = 0;
                        else if (scoreRed == 3)
                           winningTeam = 1;

                        serverMsg.type = MSG_TYPE_FINISH_GAME;

                        // I should create an instance of the GameSummary object here and just serialize it into this message
                        memcpy(serverMsg.buffer, &winningTeam, 4);
                        memcpy(serverMsg.buffer+4, &scoreBlue, 4);
                        memcpy(serverMsg.buffer+8, &scoreRed, 4);
                        strcpy(serverMsg.buffer+12, game->getName().c_str());
                        broadcastMessage(msgProcessor, serverMsg, playersInGame);
                     }

                     // this means a PLAYER message will be sent
                     broadcastMove = true;
                  }

                  // go through all objects and check if the player is close to one and if its their flag
                  vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
                  vector<WorldMap::Object>::iterator itObjects;
                  POSITION structPos;

                  for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++)
                  {
                     POSITION pos = itObjects->pos;

                     if (posDistance(p->pos, pos.toFloat()) < 10)
                     {
                        if (p->team == 0 && 
                            itObjects->type == WorldMap::OBJECT_BLUE_FLAG)
                        {
                           structPos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                           flagReturned = true;
                           break;
                        }
                        else if (p->team == 1 &&
                                 itObjects->type == WorldMap::OBJECT_RED_FLAG)
                        {
                           structPos = gameMap->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
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
                     broadcastMessage(msgProcessor, serverMsg, playersInGame);
                  }

                  if (broadcastMove)
                  {
                     serverMsg.type = MSG_TYPE_PLAYER;
                     p->serialize(serverMsg.buffer);
                     broadcastMessage(msgProcessor, serverMsg, playersInGame);
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

                  serverMsg.type = MSG_TYPE_ATTACK; 
                  memcpy(serverMsg.buffer, &p->id, 4);
                  memcpy(serverMsg.buffer+4, &p->targetPlayer, 4);
                  broadcastMessage(msgProcessor, serverMsg, playersInGame);

                  if (p->attackType == Player::ATTACK_MELEE)
                  {
                     cout << "Melee attack" << endl;

                     Player* target = playersInGame[p->targetPlayer];
                     damagePlayer(target, p->damage);

                     if (target->isDead)
                     {
                        WorldMap::ObjectType flagType = WorldMap::OBJECT_NONE;
                        if (target->hasBlueFlag)
                           flagType = WorldMap::OBJECT_BLUE_FLAG;
                        else if (target->hasRedFlag)
                           flagType = WorldMap::OBJECT_RED_FLAG;

                        if (flagType != WorldMap::OBJECT_NONE) {
                           addObjectToMap(flagType, target->pos.x, target->pos.y, gameMap, playersInGame, msgProcessor);
                        }
                     }

                     serverMsg.type = MSG_TYPE_PLAYER;
                     target->serialize(serverMsg.buffer);
                  }
                  else if (p->attackType == Player::ATTACK_RANGED)
                  {
                     cout << "Ranged attack" << endl;

                     Projectile proj(p->pos.x, p->pos.y, p->targetPlayer, p->damage);
                     game->assignProjectileId(&proj);
                     game->addProjectile(proj);

                     int x = p->pos.x;
                     int y = p->pos.y;

                     serverMsg.type = MSG_TYPE_PROJECTILE;
                     memcpy(serverMsg.buffer, &proj.id, 4);
                     memcpy(serverMsg.buffer+4, &x, 4);
                     memcpy(serverMsg.buffer+8, &y, 4);
                     memcpy(serverMsg.buffer+12, &p->targetPlayer, 4);
                  }
                  else
                     cout << "Invalid attack type: " << p->attackType << endl;

                  broadcastMessage(msgProcessor, serverMsg, playersInGame);
               }
            }

            if (gameFinished) {
                // send a GAME_INFO message with 0 players to force clients to delete the game
               int numPlayers = 0;
               serverMsg.type = MSG_TYPE_GAME_INFO;
               memcpy(serverMsg.buffer, &numPlayers, 4);
               broadcastMessage(msgProcessor, serverMsg, mapPlayers);

               // erase game from server
               mapGames.erase(itGames++);
               delete game;
            }else
               itGames++;
         }

         cout << "Processing projectiles"  << endl;

         // move all projectiles
         // see if this can be moved inside the game class
         // this method can be moved when I add a MessageProcessor to the Game class
         map<unsigned int, Projectile>::iterator itProj;
         for (itGames = mapGames.begin(); itGames != mapGames.end(); itGames++) { 
            game = itGames->second;
            for (itProj = game->getProjectiles().begin(); itProj != game->getProjectiles().end(); itProj++)
            {
               cout << "About to call projectile move" << endl;
               if (itProj->second.move(game->getPlayers()))
               {
                  // send a REMOVE_PROJECTILE message
                  cout << "send a REMOVE_PROJECTILE message" << endl;
                  serverMsg.type = MSG_TYPE_REMOVE_PROJECTILE;
                  memcpy(serverMsg.buffer, &itProj->second.id, 4);
                  game->removeProjectile(itProj->second.id);
                  broadcastMessage(msgProcessor, serverMsg, game->getPlayers());

                  Player* target = game->getPlayers()[itProj->second.target];
                  damagePlayer(target, itProj->second.damage);

                  if (target->isDead)
                  {
                     WorldMap::ObjectType flagType = WorldMap::OBJECT_NONE;
                     if (target->hasBlueFlag)
                        flagType = WorldMap::OBJECT_BLUE_FLAG;
                     else if (target->hasRedFlag)
                        flagType = WorldMap::OBJECT_RED_FLAG;

                     if (flagType != WorldMap::OBJECT_NONE)
                        addObjectToMap(flagType, target->pos.x, target->pos.y, game->getMap(), game->getPlayers(), msgProcessor);
                  }

                  // send a PLAYER message after dealing damage
                  serverMsg.type = MSG_TYPE_PLAYER;
                  target->serialize(serverMsg.buffer);
                  broadcastMessage(msgProcessor, serverMsg, game->getPlayers());
               }
            }
         }
      }

      if (msgProcessor.receiveMessage(&clientMsg, &from) >= 0)
      {
         processMessage(clientMsg, from, msgProcessor, mapPlayers, mapGames, gameMap, unusedPlayerId, serverMsg, scoreBlue, scoreRed);

         cout << "Finished processing the message" << endl;
      }
   }

   outputLog << "Stopped server on " << getCurrentDateTimeString() << endl;
   outputLog.close();

   // delete all games
   map<string, Game*>::iterator itGames;
   for (itGames = mapGames.begin(); itGames != mapGames.end(); itGames++)
   {
      delete itGames->second;
   }

   map<unsigned int, Player*>::iterator itPlayers;
   for (itPlayers = mapPlayers.begin(); itPlayers != mapPlayers.end(); itPlayers++)
   {
      delete itPlayers->second;
   }

   return 0;
}

void processMessage(const NETWORK_MSG &clientMsg, struct sockaddr_in &from, MessageProcessor &msgProcessor, map<unsigned int, Player*>& mapPlayers, map<string, Game*>& mapGames, WorldMap* gameMap, unsigned int& unusedPlayerId, NETWORK_MSG &serverMsg, int &scoreBlue, int &scoreRed)
{
   DataAccess da;

   cout << "Inside processMessage" << endl;

   cout << "Received message" << endl;
   cout << "MSG: type: " << clientMsg.type << endl;
   cout << "MSG contents: " << clientMsg.buffer << endl;

   // Check that if an invalid message is sent, the client will correctly
   // receive and display the response. Maybe make a special error msg type
   switch(clientMsg.type)
   {
      case MSG_TYPE_REGISTER:
      {
         string username(clientMsg.buffer);
         string password(strchr(clientMsg.buffer, '\0')+1);
         Player::PlayerClass playerClass;

         memcpy(&playerClass, clientMsg.buffer+username.length()+password.length()+2, 4);

         cout << "username: " << username << endl;
         cout << "password: " << password << endl;

         bool validClass = false;

         switch(playerClass) {
            case Player::CLASS_WARRIOR:
            case Player::CLASS_RANGER:
               validClass = true;
               break;
            default:
               validClass = false;
               break;
         }

         serverMsg.type = MSG_TYPE_REGISTER;

         if (validClass) {
            int error = da.insertPlayer(username, password, playerClass);

            if (error)
               strcpy(serverMsg.buffer, "Registration failed. Please try again.");
            else
               strcpy(serverMsg.buffer, "Registration successful.");
         }else
            strcpy(serverMsg.buffer, "You didn't select a class");

         msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_LOGIN:
      {
         cout << "Got login message" << endl;

         string username(clientMsg.buffer);
         string password(strchr(clientMsg.buffer, '\0')+1);

         Player* p = da.getPlayer(username);

         if (p == NULL || !da.verifyPassword(password, p->password))
         {
            strcpy(serverMsg.buffer, "Incorrect username or password");
            if (p != NULL)
               delete(p);
         }
         else if(findPlayerByName(mapPlayers, username) != NULL)
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
            delete(p);
         }
         else
         {
            updateUnusedPlayerId(unusedPlayerId, mapPlayers);
            p->id = unusedPlayerId;
            cout << "new player id: " << p->id << endl;
            p->setAddr(from);
            p->currentGame = NULL;

            // choose a random team (either 0 or 1)
            p->team = rand() % 2;

            serverMsg.type = MSG_TYPE_PLAYER;
            // tell the new player about all the existing players
            cout << "Sending other players to new player" << endl;

            map<unsigned int, Player*>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second->serialize(serverMsg.buffer);

               cout << "sending info about " << it->second->name  << endl;
               cout << "sending id " << it->second->id  << endl;
               msgProcessor.sendMessage(&serverMsg, &from);
            }

            // tell the new player about all map objects
            // (currently just the flags)
            serverMsg.type = MSG_TYPE_OBJECT;
            vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
            vector<WorldMap::Object>::iterator itObjects;
            cout << "sending items" << endl;
            for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
               itObjects->serialize(serverMsg.buffer);
               cout << "sending item id " << itObjects->id  << endl;
               msgProcessor.sendMessage(&serverMsg, &from);
            }

            // send info about existing games to new player
            map<string, Game*>::iterator itGames;
            Game* g;
            int numPlayers;
            serverMsg.type = MSG_TYPE_GAME_INFO;

            for (itGames = mapGames.begin(); itGames != mapGames.end(); itGames++)
            {
               g = itGames->second;
               numPlayers = g->getNumPlayers();
               memcpy(serverMsg.buffer, &numPlayers, 4);
               strcpy(serverMsg.buffer+4, g->getName().c_str());
               msgProcessor.sendMessage(&serverMsg, &from);
            }

            // send the current score
            serverMsg.type = MSG_TYPE_SCORE;
            memcpy(serverMsg.buffer, &scoreBlue, 4);
            memcpy(serverMsg.buffer+4, &scoreRed, 4);
            msgProcessor.sendMessage(&serverMsg, &from);

            serverMsg.type = MSG_TYPE_PLAYER;
            p->serialize(serverMsg.buffer);
            broadcastMessage(msgProcessor, serverMsg, mapPlayers);

            mapPlayers[unusedPlayerId] = p;
         }

         serverMsg.type = MSG_TYPE_LOGIN;
         msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_LOGOUT:
      {
         string name(clientMsg.buffer);
         cout << "Player logging out: " << name << endl;

         Player *p = findPlayerByName(mapPlayers, name);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer+4, "That player is not logged in. This is either a bug, or you're trying to hack the server.");
            cout << "Player not logged in" << endl;
         }
         else if ( p->addr.sin_addr.s_addr != from.sin_addr.s_addr ||
                   p->addr.sin_port != from.sin_port )
         {
            strcpy(serverMsg.buffer+4, "That player is logged in using a differemt connection. This is either a bug, or you're trying to hack the server.");
            cout << "Player logged in using a different connection" << endl;
         }
         else
         {
            if (!p->isDead) {
               WorldMap::ObjectType flagType = WorldMap::OBJECT_NONE;
               if (p->hasBlueFlag)
                  flagType = WorldMap::OBJECT_BLUE_FLAG;
               else if (p->hasRedFlag)
                  flagType = WorldMap::OBJECT_RED_FLAG;

               if (flagType != WorldMap::OBJECT_NONE) {
                  addObjectToMap(flagType, p->pos.x, p->pos.y, gameMap, mapPlayers, msgProcessor);
               }
            }

            // broadcast to all players before deleting p from the map
            serverMsg.type = MSG_TYPE_LOGOUT;
            memcpy(serverMsg.buffer, &p->id, 4);

            broadcastMessage(msgProcessor, serverMsg, mapPlayers);

            if (p->id < unusedPlayerId)
               unusedPlayerId = p->id;

            mapPlayers.erase(p->id);
            delete p;

            strcpy(serverMsg.buffer+4, "You have successfully logged out.");
         }

         serverMsg.type = MSG_TYPE_LOGOUT;
         msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_CHAT:
      {
         cout << "Got a chat message" << endl;

         serverMsg.type = MSG_TYPE_CHAT;

         Player *p = findPlayerByAddr(mapPlayers, from);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "No player is logged in using this connection. This is either a bug, or you're trying to hack the server.");
            msgProcessor.sendMessage(&serverMsg, &from);
         }
         else
         {
            ostringstream oss;
            oss << p->name << ": " << clientMsg.buffer;

            strcpy(serverMsg.buffer, oss.str().c_str());
            broadcastMessage(msgProcessor, serverMsg, mapPlayers);
         }	

         break;
      }
      case MSG_TYPE_PLAYER_MOVE:
      {
         cout << "PLAYER_MOVE" << endl;

         int id, x, y;

         memcpy(&id, clientMsg.buffer, 4);
         memcpy(&x, clientMsg.buffer+4, 4);
         memcpy(&y, clientMsg.buffer+8, 4);

         cout << "x: " << x << endl;
         cout << "y: " << y << endl;
         cout << "id: " << id << endl;
         
         Player* p = mapPlayers[id];
         bool validMessage = false;

         if ( p->addr.sin_addr.s_addr == from.sin_addr.s_addr &&
              p->addr.sin_port == from.sin_port )
         {
            if (p->currentGame->startPlayerMovement(id, x, y)) {
               serverMsg.type = MSG_TYPE_PLAYER_MOVE;
               
               memcpy(serverMsg.buffer, &id, 4);
               memcpy(serverMsg.buffer+4, &p->target.x, 4);
               memcpy(serverMsg.buffer+8, &p->target.y, 4);

               broadcastMessage(msgProcessor, serverMsg, mapPlayers);

               validMessage = true;
            }
            else
               cout << "Bad terrain detected" << endl;
         }
         else
            cout << "Player id (" << id << ") doesn't match sender" << endl;

         if (!validMessage)
            msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_PICKUP_FLAG:
      { 
         // may want to check the id matches the sender, just like for PLAYER_NOVE
         cout << "PICKUP_FLAG" << endl;

         int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         Player* p = mapPlayers[id];
         int objectId = p->currentGame->processFlagPickupRequest(p);

         if (objectId >= 0) {
            map<unsigned int, Player*> players = p->currentGame->getPlayers();

            serverMsg.type = MSG_TYPE_REMOVE_OBJECT;
            memcpy(serverMsg.buffer, &objectId, 4);
            broadcastMessage(msgProcessor, serverMsg, players);

            serverMsg.type = MSG_TYPE_PLAYER;
            p->serialize(serverMsg.buffer);
            broadcastMessage(msgProcessor, serverMsg, players);
         }

         break;
      }
      case MSG_TYPE_DROP_FLAG:
      {
         // may want to check the id matches the sender, just like for PLAYER_NOVE
         cout << "DROP_FLAG" << endl;

         int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         Player* p = mapPlayers[id];

         WorldMap::ObjectType flagType = WorldMap::OBJECT_NONE;
         if (p->hasBlueFlag)
            flagType = WorldMap::OBJECT_BLUE_FLAG;
         else if (p->hasRedFlag)
            flagType = WorldMap::OBJECT_RED_FLAG;

         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         addObjectToMap(flagType, p->pos.x, p->pos.y, p->currentGame->getMap(), players, msgProcessor);

         p->hasBlueFlag = false;
         p->hasRedFlag = false;

         serverMsg.type = MSG_TYPE_PLAYER;
         p->serialize(serverMsg.buffer);
         broadcastMessage(msgProcessor, serverMsg, players);

         break;
      }
      case MSG_TYPE_START_ATTACK:
      {
         cout << "Received a START_ATTACK message" << endl;

         int id, targetId;

         memcpy(&id, clientMsg.buffer, 4);
         memcpy(&targetId, clientMsg.buffer+4, 4);

         // need to make sure the target is in the sender's game

         Player* p = mapPlayers[id];
         p->targetPlayer = targetId;
         p->isChasing = true;

         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         serverMsg.type = MSG_TYPE_START_ATTACK;
         memcpy(serverMsg.buffer, &id, 4);
         memcpy(serverMsg.buffer+4, &targetId, 4);
         broadcastMessage(msgProcessor, serverMsg, players);

         break;
      }
      case MSG_TYPE_CREATE_GAME:
      {
         cout << "Received a CREATE_GAME message" << endl;

         string gameName(clientMsg.buffer);
         cout << "Game name: " << gameName << endl;

         // check if this game already exists
         if (mapGames.find(gameName) != mapGames.end()) {
            cout << "Error: Game already exists" << endl;
            serverMsg.type = MSG_TYPE_JOIN_GAME_FAILURE;
         }else {
            Game* g = new Game(gameName, "../data/map.txt");
            mapGames[gameName] = g;

            // add flag objects to the map
            WorldMap* m = g->getMap();
            for (int y=0; y<m->height; y++) {
               for (int x=0; x<m->width; x++) {
                  switch (m->getStructure(x, y)) {
                     case WorldMap::STRUCTURE_BLUE_FLAG:
                        m->addObject(WorldMap::OBJECT_BLUE_FLAG, x*25+12, y*25+12);
                        break;
                     case WorldMap::STRUCTURE_RED_FLAG:
                        m->addObject(WorldMap::OBJECT_RED_FLAG, x*25+12, y*25+12);
                        break;
                  }
               }
            }

            serverMsg.type = MSG_TYPE_JOIN_GAME_SUCCESS;
            strcpy(serverMsg.buffer, gameName.c_str());
         }

         msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_JOIN_GAME:
      {
         cout << "Received a JOIN_GAME message" << endl;

         string gameName(clientMsg.buffer);
         cout << "Game name: " << gameName << endl;

         // check if this game already exists
         if (mapGames.find(gameName) == mapGames.end()) {
            cout << "Error: Game does not exist" << endl;
            serverMsg.type = MSG_TYPE_JOIN_GAME_FAILURE;
         }else {
            Game* g = mapGames[gameName];
            map<unsigned int, Player*>& players = g->getPlayers();
            Player* p = findPlayerByAddr(mapPlayers, from);

            if (players.find(p->id) != players.end()) {
               cout << "Player " << p->name << " trying to join a game he's already in" << endl;
               serverMsg.type = MSG_TYPE_JOIN_GAME_FAILURE;
            }else {
               serverMsg.type = MSG_TYPE_JOIN_GAME_SUCCESS;
               strcpy(serverMsg.buffer, gameName.c_str());
            }
         }

         msgProcessor.sendMessage(&serverMsg, &from);

         break;
      }
      case MSG_TYPE_LEAVE_GAME:
      {
         cout << "Received a LEAVE_GAME message" << endl;

         Player* p = findPlayerByAddr(mapPlayers, from);
         Game* g = p->currentGame;

         if (g == NULL) {
            cout << "Player " << p->name << " is trying to leave a game, but is not currently in a game." << endl;

            /// should send a response back, maybe a new message type is needed
            // not sure what to do here
         }else {
            cout << "Game name: " << g->getName() << endl;
            p->currentGame = NULL;

            serverMsg.type = MSG_TYPE_LEAVE_GAME;
            memcpy(serverMsg.buffer, &p->id, 4);
            strcpy(serverMsg.buffer+4, g->getName().c_str());
            broadcastMessage(msgProcessor, serverMsg, g->getPlayers());

            g->removePlayer(p->id);

            int numPlayers = g->getNumPlayers();

            serverMsg.type = MSG_TYPE_GAME_INFO;
            memcpy(serverMsg.buffer, &numPlayers, 4);
            strcpy(serverMsg.buffer+4, g->getName().c_str());
            broadcastMessage(msgProcessor, serverMsg, mapPlayers);

            // if there are no more players in the game, remove it
            if (numPlayers == 0) {
               mapGames.erase(g->getName());
               delete g;
            }
         }

         break;
      }
      case MSG_TYPE_JOIN_GAME_ACK:
      {
         cout << "Received a JOIN_GAME_ACK message" << endl;

         string gameName(clientMsg.buffer);
         cout << "Game name: " << gameName << endl;

         // check if this game already exists
         if (mapGames.find(gameName) == mapGames.end()) {
            serverMsg.type = MSG_TYPE_JOIN_GAME_FAILURE;

            msgProcessor.sendMessage(&serverMsg, &from);
         }

         Game* g = mapGames[gameName];

         Player* p = findPlayerByAddr(mapPlayers, from);
         p->team = rand() % 2; // choose a random team (either 0 or 1)
         p->currentGame = g;

         // tell the new player about all map objects
         // (currently just the flags)

         serverMsg.type = MSG_TYPE_OBJECT;
         vector<WorldMap::Object>* vctObjects = g->getMap()->getObjects();
         vector<WorldMap::Object>::iterator itObjects;
         cout << "sending items" << endl;
         for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
            itObjects->serialize(serverMsg.buffer);
            cout << "sending item id " << itObjects->id  << endl;
            msgProcessor.sendMessage(&serverMsg, &from);
         }


         // send the current score
         serverMsg.type = MSG_TYPE_SCORE;

         int game_blueScore = g->getBlueScore();
         int game_redScore = g->getRedScore();
         memcpy(serverMsg.buffer, &game_blueScore, 4);
         memcpy(serverMsg.buffer+4, &game_redScore, 4);

         msgProcessor.sendMessage(&serverMsg, &from);

         // send info to other players
         serverMsg.type = MSG_TYPE_PLAYER_JOIN_GAME;
         p->serialize(serverMsg.buffer);
         cout << "Should be broadcasting the message" << endl;
         broadcastMessage(msgProcessor, serverMsg, g->getPlayers());

         g->addPlayer(p);


         // tell the new player about all the players in the game (including himself)
         cout << "Sending other players to new player" << endl;
         serverMsg.type = MSG_TYPE_PLAYER_JOIN_GAME;

         
         map<unsigned int, Player*>& allPlayers = g->getPlayers();
         map<unsigned int, Player*>::iterator it;
         for (it = allPlayers.begin(); it != allPlayers.end(); it++)
         {
            it->second->serialize(serverMsg.buffer);

            cout << "sending info about " << it->second->name  << endl;
            cout << "sending id " << it->second->id  << endl;
            msgProcessor.sendMessage(&serverMsg, &from);
         }

         int numPlayers = g->getNumPlayers();

         serverMsg.type = MSG_TYPE_GAME_INFO;
         memcpy(serverMsg.buffer, &numPlayers, 4);
         strcpy(serverMsg.buffer+4, gameName.c_str());
         broadcastMessage(msgProcessor, serverMsg, mapPlayers);

         break;
      }
      default:
      {
         // probably want to log the error rather than sending a chat message,
         // especially since chat isn't currently visible on all screens

         serverMsg.type = MSG_TYPE_CHAT;
         strcpy(serverMsg.buffer, "Server error occured. Report this please.");

         break;
      }
   }
}

void broadcastMessage(MessageProcessor &msgProcessor, NETWORK_MSG &serverMsg, map<unsigned int, Player*>& players) {
   map<unsigned int, Player*>::iterator it;
   for (it = players.begin(); it != players.end(); it++) {
      msgProcessor.sendMessage(&serverMsg, &(it->second->addr));
   }
}

void updateUnusedPlayerId(unsigned int& id, map<unsigned int, Player*>& mapPlayers)
{
   while (mapPlayers.find(id) != mapPlayers.end())
      id++;
}

Player *findPlayerByName(map<unsigned int, Player*> &m, string name)
{
   map<unsigned int, Player*>::iterator it;

   for (it = m.begin(); it != m.end(); it++)
   {
      if ( it->second->name.compare(name) == 0 )
         return it->second;
   }

   return NULL;
}

Player *findPlayerByAddr(map<unsigned int, Player*> &m, const sockaddr_in &addr)
{
   map<unsigned int, Player*>::iterator it;

   for (it = m.begin(); it != m.end(); it++)
   {
      if ( it->second->addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
           it->second->addr.sin_port == addr.sin_port )
         return it->second;
   }

   return NULL;
}

void damagePlayer(Player *p, int damage) {
   p->health -= damage;
   if (p->health < 0)
      p->health = 0;
   if (p->health == 0) {
      cout << "Player died" << endl;
      p->isDead = true;
      p->timeDied = getCurrentMillis();
   }
}

void addObjectToMap(WorldMap::ObjectType objectType, int x, int y, WorldMap* gameMap, map<unsigned int, Player*>& mapPlayers, MessageProcessor &msgProcessor) {
   NETWORK_MSG serverMsg;

   gameMap->addObject(objectType, x, y);

   serverMsg.type = MSG_TYPE_OBJECT;
   gameMap->getObjects()->back().serialize(serverMsg.buffer);

   broadcastMessage(msgProcessor, serverMsg, mapPlayers);
}
