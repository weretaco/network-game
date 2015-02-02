#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

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

// from used to be const. Removed that so I could take a reference
// and use it to send messages
void processMessage(const NETWORK_MSG& clientMsg, struct sockaddr_in& from, MessageProcessor& msgProcessor, map<unsigned int, Player*>& mapPlayers, map<string, Game*>& mapGames, DataAccess& da, ofstream& outputLog);

Player *findPlayerByName(map<unsigned int, Player*> &m, string name);
Player *findPlayerByAddr(map<unsigned int, Player*> &m, const sockaddr_in &addr);

bool handleGameEvents(Game* game, MessageProcessor* msgProcessor, DataAccess* da);
bool handlePlayerEvents(Game* game, Player* p, MessageProcessor* msgProcessor, DataAccess* da);

void quit(int sig);

bool done;

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
   DataAccess da;
   ofstream outputLog;

   done = false;

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
   int timeLastUpdated = 0, curTime = 0;
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
                  case Player::TEAM_BLUE:
                     spawnPos = p->currentGame->getMap()->getStructureLocation(STRUCTURE_BLUE_FLAG);
                     break;
                  case Player::TEAM_RED:
                     spawnPos = p->currentGame->getMap()->getStructureLocation(STRUCTURE_RED_FLAG);
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

                  msgProcessor.broadcastMessage(serverMsg, p->currentGame->getPlayers());
               }

               continue;
            }

            if (p->currentGame != NULL) {
               map<unsigned int, Player*> playersInGame = p->currentGame->getPlayers();
               if (p->updateTarget(playersInGame))
               {
                  serverMsg.type = MSG_TYPE_PLAYER;
                  p->serialize(serverMsg.buffer);
                  msgProcessor.broadcastMessage(serverMsg, playersInGame);
               }
            }
         }

         // process players currently in a game
         map<string, Game*>::iterator itGames;
         Game* game = NULL;

         for (itGames = mapGames.begin(); itGames != mapGames.end();) { 
            game = itGames->second;
            bool gameFinished = handleGameEvents(game, &msgProcessor, &da);

            if (gameFinished) {
               // save game record
               int winningTeam = -1;
               if (game->getBlueScore() == 3)
                  winningTeam = 1;
               else if (game->getRedScore() == 3)
                  winningTeam = 2;

               if (winningTeam == -1)
                  cout << "Error: Game ended, but neither team has a score of 3" << endl;
               else {
                  map<unsigned int, Player*>::iterator it;

                  time_t timeFinished = time(NULL);
                  for (it = game->getPlayers().begin(); it != game->getPlayers().end(); it++) {
                      Player* p = it->second;
                      cout << "winning team: " << winningTeam << endl;
                      cout << "player team: " << p->team << endl;
                      cout << "player id: " << p->getId() << endl;
                      
                      if (winningTeam == p->team)
                          p->wins++;
                      else
                          p->losses++;
                      da.updatePlayer(p);
                      da.saveGameHistory(p->getId(), winningTeam, game->getBlueScore(), game->getRedScore(), timeFinished);
                  }
               }

               // send a GAME_INFO message with 0 players to force clients to delete the game
               int numPlayers = 0;
               serverMsg.type = MSG_TYPE_GAME_INFO;
               memcpy(serverMsg.buffer, &numPlayers, 4);
               strcpy(serverMsg.buffer+4, game->getName().c_str());

               // only send this to players in the game
               msgProcessor.broadcastMessage(serverMsg, game->getPlayers());

               delete itGames->second;
               mapGames.erase(itGames++);
            }else
               itGames++;
         }

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
                  msgProcessor.broadcastMessage(serverMsg, game->getPlayers());

                  Player* target = game->getPlayers()[itProj->second.target];
                  game->dealDamageToPlayer(target, itProj->second.damage);
               }
            }
         }
      }

      if (msgProcessor.receiveMessage(&clientMsg, &from) >= 0)
      {
         processMessage(clientMsg, from, msgProcessor, mapPlayers, mapGames, da, outputLog);

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

void processMessage(const NETWORK_MSG &clientMsg, struct sockaddr_in &from, MessageProcessor &msgProcessor, map<unsigned int, Player*>& mapPlayers, map<string, Game*>& mapGames, DataAccess& da, ofstream& outputLog)
{
   NETWORK_MSG serverMsg;

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
            case Player::CLASS_NONE:
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
            cout << "new player id: " << p->getId() << endl;
            p->setAddr(from);

            serverMsg.type = MSG_TYPE_PLAYER;
            // tell the new player about all the existing players
            cout << "Sending other players to new player" << endl;

            map<unsigned int, Player*>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second->serialize(serverMsg.buffer);

               cout << "sending info about " << it->second->name  << endl;
               cout << "sending id " << it->second->getId()  << endl;
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

            serverMsg.type = MSG_TYPE_PLAYER;
            p->serialize(serverMsg.buffer);
            msgProcessor.broadcastMessage(serverMsg, mapPlayers);

            mapPlayers[p->getId()] = p;
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
            // broadcast to all players before deleting p from the map
            unsigned int playerId = p->getId();
            serverMsg.type = MSG_TYPE_LOGOUT;
            memcpy(serverMsg.buffer, &playerId, 4);

            msgProcessor.broadcastMessage(serverMsg, mapPlayers);

            mapPlayers.erase(p->getId());
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
            msgProcessor.broadcastMessage(serverMsg, mapPlayers);
         }	

         break;
      }
      case MSG_TYPE_PLAYER_MOVE:
      {
         cout << "PLAYER_MOVE" << endl;

         unsigned int id;
         int x, y;

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

               msgProcessor.broadcastMessage(serverMsg, mapPlayers);

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

         unsigned int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         Player* p = mapPlayers[id];
         unsigned int objectId = p->currentGame->processFlagPickupRequest(p);

         if (objectId >= 0) {
            map<unsigned int, Player*> players = p->currentGame->getPlayers();

            serverMsg.type = MSG_TYPE_REMOVE_OBJECT;
            memcpy(serverMsg.buffer, &objectId, 4);
            msgProcessor.broadcastMessage(serverMsg, players);

            serverMsg.type = MSG_TYPE_PLAYER;
            p->serialize(serverMsg.buffer);
            msgProcessor.broadcastMessage(serverMsg, players);
         }

         break;
      }
      case MSG_TYPE_DROP_FLAG:
      {
         // may want to check the id matches the sender, just like for PLAYER_NOVE
         cout << "DROP_FLAG" << endl;

         unsigned int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         Player* p = mapPlayers[id];

         ObjectType flagType = OBJECT_NONE;
         if (p->hasBlueFlag)
            flagType = OBJECT_BLUE_FLAG;
         else if (p->hasRedFlag)
            flagType = OBJECT_RED_FLAG;

         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         p->currentGame->addObjectToMap(flagType, p->pos.x, p->pos.y);

         p->hasBlueFlag = false;
         p->hasRedFlag = false;

         serverMsg.type = MSG_TYPE_PLAYER;
         p->serialize(serverMsg.buffer);
         msgProcessor.broadcastMessage(serverMsg, players);

         break;
      }
      case MSG_TYPE_ATTACK:
      {
         cout << "Received a START_ATTACK message" << endl;

         unsigned int id, targetId;

         memcpy(&id, clientMsg.buffer, 4);
         memcpy(&targetId, clientMsg.buffer+4, 4);

         // need to make sure the target is in the sender's game

         Player* p = mapPlayers[id];
         p->setTargetPlayer(targetId);
         p->isChasing = true;

         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         serverMsg.type = MSG_TYPE_ATTACK;
         memcpy(serverMsg.buffer, &id, 4);
         memcpy(serverMsg.buffer+4, &targetId, 4);
         msgProcessor.broadcastMessage(serverMsg, players);

         break;
      }
      case MSG_TYPE_PROFILE:
      {
         cout << "Received a PROFILE message" << endl;

         unsigned int id;

         memcpy(&id, clientMsg.buffer, 4);

         cout << "Player id: " << id << endl;
         unsigned int numGames = 0;
         int** gameHistory = da.getPlayerGameHistory(id, numGames);
         int* playerRecord = da.getPlayerRecord(id);

         // playerRecord[0] is level
         // playerRecord[1] is experience
         int honorPoints = playerRecord[2];
         int wins = playerRecord[3];
         int losses = playerRecord[4];

         serverMsg.type = MSG_TYPE_PROFILE;

         memcpy(serverMsg.buffer, &honorPoints, 4);
         memcpy(serverMsg.buffer+4, &wins, 4);
         memcpy(serverMsg.buffer+8, &losses, 4);
         memcpy(serverMsg.buffer+12, &numGames, 4);
         for (unsigned int i=0; i<numGames; i++) {
            memcpy(serverMsg.buffer+16+i*20, &gameHistory[i][0], 4);
            memcpy(serverMsg.buffer+20+i*20, &gameHistory[i][1], 4);
            memcpy(serverMsg.buffer+24+i*20, &gameHistory[i][2], 4);
            memcpy(serverMsg.buffer+28+i*20, &gameHistory[i][3], 4);
            memcpy(serverMsg.buffer+32+i*20, &gameHistory[i][4], 4);
            delete[] gameHistory[i];
         }

         //delete[] gameHistory;
         free(gameHistory);
         delete[] playerRecord;

         msgProcessor.sendMessage(&serverMsg, &from);

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
            serverMsg.type = MSG_TYPE_CREATE_GAME_FAILURE;
         }else {
            Game* g = new Game(gameName, "../data/map.txt", &msgProcessor);
            mapGames[gameName] = g;

            // add flag objects to the map
            WorldMap* m = g->getMap();
            m->createObjectsFromStructures();

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

            if (players.find(p->getId()) != players.end()) {
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

            if (!p->isDead) {
               ObjectType flagType = OBJECT_NONE;
               if (p->hasBlueFlag)
                  flagType = OBJECT_BLUE_FLAG;
               else if (p->hasRedFlag)
                  flagType = OBJECT_RED_FLAG;

               if (flagType != OBJECT_NONE)
                  g->addObjectToMap(flagType, p->pos.x, p->pos.y);
            }

            p->currentGame = NULL;
            g->removePlayer(p->getId());

            unsigned int playerId = p->getId();
            serverMsg.type = MSG_TYPE_LEAVE_GAME;
            memcpy(serverMsg.buffer, &playerId, 4);
            strcpy(serverMsg.buffer+4, g->getName().c_str());
            msgProcessor.broadcastMessage(serverMsg, g->getPlayers());

            int numPlayers = g->getNumPlayers();

            serverMsg.type = MSG_TYPE_GAME_INFO;
            memcpy(serverMsg.buffer, &numPlayers, 4);
            strcpy(serverMsg.buffer+4, g->getName().c_str());
            msgProcessor.broadcastMessage(serverMsg, mapPlayers);

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

         unsigned int blueScore = g->getBlueScore();
         unsigned int redScore = g->getRedScore();
         memcpy(serverMsg.buffer, &blueScore, 4);
         memcpy(serverMsg.buffer+4, &redScore, 4);

         msgProcessor.sendMessage(&serverMsg, &from);


         map<unsigned int, Player*>& oldPlayers = g->getPlayers();
         g->addPlayer(p);
         p->team = Player::TEAM_NONE;

         // send info to other players
         serverMsg.type = MSG_TYPE_PLAYER_JOIN_GAME;
         p->serialize(serverMsg.buffer);
         cout << "Should be broadcasting the message" << endl;
         msgProcessor.broadcastMessage(serverMsg, oldPlayers);


         // tell the new player about all the players in the game (including himself)
         cout << "Sending other players to new player" << endl;
         serverMsg.type = MSG_TYPE_PLAYER_JOIN_GAME;
         
         map<unsigned int, Player*>& allPlayers = g->getPlayers();
         map<unsigned int, Player*>::iterator it;
         for (it = allPlayers.begin(); it != allPlayers.end(); it++)
         {
            it->second->serialize(serverMsg.buffer);

            cout << "sending info about " << it->second->name  << endl;
            cout << "sending id " << it->second->getId()  << endl;
            msgProcessor.sendMessage(&serverMsg, &from);
         }


         int numPlayers = g->getNumPlayers();

         serverMsg.type = MSG_TYPE_GAME_INFO;
         memcpy(serverMsg.buffer, &numPlayers, 4);
         strcpy(serverMsg.buffer+4, gameName.c_str());
         msgProcessor.broadcastMessage(serverMsg, mapPlayers);

         break;
      }
      case MSG_TYPE_JOIN_TEAM:
      {
         cout << "Received a JOIN_TEAM message" << endl;

         Player* p = findPlayerByAddr(mapPlayers, from);
         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         memcpy(&(p->team), clientMsg.buffer, 4);

         serverMsg.type = MSG_TYPE_PLAYER;
         p->serialize(serverMsg.buffer);
         msgProcessor.broadcastMessage(serverMsg, players);

         break;
      }
      case MSG_TYPE_START_GAME:
      {
         cout << "Received a START_GAME message" << endl;

         Player* p = findPlayerByAddr(mapPlayers, from);
         map<unsigned int, Player*> players = p->currentGame->getPlayers();

         serverMsg.type = MSG_TYPE_START_GAME;
         msgProcessor.broadcastMessage(serverMsg, players);

         // send a GAME_INFO message to all players not in the game so they delete it from their lobby
         map<unsigned int, Player*> playersNotInGame;
         map<unsigned int, Player*>::iterator it;
         
         for (it = mapPlayers.begin(); it != mapPlayers.end(); it++) {
             if (players.count(it->first) == 0)
                 playersNotInGame[it->first] = it->second;
         }

         cout << "Sending game info to " << playersNotInGame.size() << " players not in the currently started game" << endl;

         int numPlayers = 0;
         serverMsg.type = MSG_TYPE_GAME_INFO;
         memcpy(serverMsg.buffer, &numPlayers, 4);
         strcpy(serverMsg.buffer+4, p->currentGame->getName().c_str());

         msgProcessor.broadcastMessage(serverMsg, playersNotInGame);

         break;
      }
      default:
      {
         outputLog << "Received unknown message of type " << clientMsg.type << endl;

         break;
      }
   }
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

bool handleGameEvents(Game* game, MessageProcessor* msgProcessor, DataAccess* da) {
   map<unsigned int, Player*> players = game->getPlayers();
   map<unsigned int, Player*>::iterator it;
   bool gameFinished = false;

   for (it = players.begin(); it != players.end(); it++) {
      gameFinished = gameFinished || handlePlayerEvents(game, it->second, msgProcessor, da);
   }

   if (gameFinished) {
      for (it = players.begin(); it != players.end(); it++) {
         it->second->currentGame = NULL;
      }
   }

   return gameFinished;
}

bool handlePlayerEvents(Game* game, Player* p, MessageProcessor* msgProcessor, DataAccess* da) {
   NETWORK_MSG serverMsg;
   FLOAT_POSITION oldPos;
   bool gameFinished = false;
   bool broadcastMove = false;

   cout << "moving player" << endl;

   // move player and perform associated tasks
   oldPos = p->pos;
   if (p->move(game->getMap())) {

      cout << "player moved" << endl;
      if (game->processPlayerMovement(p, oldPos))
         broadcastMove = true;
      cout << "player move processed" << endl;

      ObjectType flagType;
      POSITION pos;
      bool flagTurnedIn = false;
      bool flagReturned = false;
      bool ownFlagAtBase = false;

      switch(game->getMap()->getStructure(p->pos.x/25, p->pos.y/25)) {
         case STRUCTURE_BLUE_FLAG:
         {
            if (p->team == Player::TEAM_BLUE && p->hasRedFlag) {
               // check that your flag is at your base
               pos = game->getMap()->getStructureLocation(STRUCTURE_BLUE_FLAG);
                           
               vector<WorldMap::Object>* vctObjects = game->getMap()->getObjects();
               vector<WorldMap::Object>::iterator itObjects;

               for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
                  if (itObjects->type == OBJECT_BLUE_FLAG) {
                     if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12) {
                        ownFlagAtBase = true;
                        break;
                     }
                  }
               }

               if (ownFlagAtBase) {
                  p->hasRedFlag = false;
                  flagType = OBJECT_RED_FLAG;
                  pos = game->getMap()->getStructureLocation(STRUCTURE_RED_FLAG);
                  flagTurnedIn = true;
                  game->setBlueScore(game->getBlueScore()+1);
               }
            }

            break;
         }
         case STRUCTURE_RED_FLAG:
         {
            if (p->team == Player::TEAM_RED && p->hasBlueFlag) {
               // check that your flag is at your base
               pos = game->getMap()->getStructureLocation(STRUCTURE_RED_FLAG);
                        
               vector<WorldMap::Object>* vctObjects = game->getMap()->getObjects();
               vector<WorldMap::Object>::iterator itObjects;

               for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
                  if (itObjects->type == OBJECT_RED_FLAG) {
                     if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12) {
                        ownFlagAtBase = true;
                        break;
                     }
                  }
               }

               if (ownFlagAtBase) {
                  p->hasBlueFlag = false;
                  flagType = OBJECT_BLUE_FLAG;
                  pos = game->getMap()->getStructureLocation(STRUCTURE_BLUE_FLAG);
                  flagTurnedIn = true;
                  game->setRedScore(game->getRedScore()+1);
               }
            }

            break;
         }
         default:
         {
            break;
         }
      }

      if (flagTurnedIn) {
         unsigned int blueScore = game->getBlueScore();
         unsigned int redScore = game->getRedScore();

         pos.x = pos.x*25+12;
         pos.y = pos.y*25+12;
         game->addObjectToMap(flagType, pos.x, pos.y);

         serverMsg.type = MSG_TYPE_SCORE;
         memcpy(serverMsg.buffer, &blueScore, 4);
         memcpy(serverMsg.buffer+4, &redScore, 4);
         msgProcessor->broadcastMessage(serverMsg, game->getPlayers());

         // give honor to everyone on the winning team
         map<unsigned int, Player*>::iterator itPlayers;
         for (itPlayers = game->getPlayers().begin(); itPlayers != game->getPlayers().end(); itPlayers++) {
            if (itPlayers->second->team == p->team) {
               itPlayers->second->honor += 50;
               da->updatePlayer(itPlayers->second);
            }
         }

         // check to see if the game should end
         // move to its own method
         if (game->getBlueScore() == 3 || game->getRedScore() == 3) {
            gameFinished = true;

            unsigned int winningTeam;
            if (game->getBlueScore() == 3)
               winningTeam = 0;
            else if (game->getRedScore() == 3)
               winningTeam = 1;

            serverMsg.type = MSG_TYPE_FINISH_GAME;

            // I should create an instance of the GameSummary object here and just serialize it into this message
            memcpy(serverMsg.buffer, &winningTeam, 4);
            memcpy(serverMsg.buffer+4, &blueScore, 4);
            memcpy(serverMsg.buffer+8, &redScore, 4);
            strcpy(serverMsg.buffer+12, game->getName().c_str());
            msgProcessor->broadcastMessage(serverMsg, game->getPlayers());

            // give honor to everyone on the winning team
            for (itPlayers = game->getPlayers().begin(); itPlayers != game->getPlayers().end(); itPlayers++) {
               if (itPlayers->second->team == p->team) {
                  itPlayers->second->honor += 150;
                  da->updatePlayer(itPlayers->second);
               }
            }
         }

         // this means a PLAYER message will be sent
         broadcastMove = true;
      }

      // go through all objects and check if the player is close to one and if its their flag
      vector<WorldMap::Object>* vctObjects = game->getMap()->getObjects();
      vector<WorldMap::Object>::iterator itObjects;
      POSITION structPos;

      for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
         POSITION pos = itObjects->pos;

         if (posDistance(p->pos, pos.toFloat()) < 10) {
            if (p->team == Player::TEAM_BLUE && itObjects->type == OBJECT_BLUE_FLAG) {
               structPos = game->getMap()->getStructureLocation(STRUCTURE_BLUE_FLAG);
               flagReturned = true;
               break;
            } else if (p->team == Player::TEAM_RED && itObjects->type == OBJECT_RED_FLAG) {
               structPos = game->getMap()->getStructureLocation(STRUCTURE_RED_FLAG);
               flagReturned = true;
               break;
            }
         }
      }

      if (flagReturned) {
         itObjects->pos.x = structPos.x*25+12;
         itObjects->pos.y = structPos.y*25+12;

         serverMsg.type = MSG_TYPE_OBJECT;
         itObjects->serialize(serverMsg.buffer);
         msgProcessor->broadcastMessage(serverMsg, game->getPlayers());
      }

      if (broadcastMove) {
         serverMsg.type = MSG_TYPE_PLAYER;
         p->serialize(serverMsg.buffer);
         msgProcessor->broadcastMessage(serverMsg, game->getPlayers());
      }
   }

   cout << "processing player attack" << endl;

   // check if the player's attack animation is complete
   if (p->isAttacking && p->timeAttackStarted+p->attackCooldown <= getCurrentMillis()) {
      p->isAttacking = false;
      cout << "Attack animation is complete" << endl;

      //send everyone an ATTACK message
      cout << "about to broadcast attack" << endl;

      if (p->attackType == Player::ATTACK_MELEE) {
         cout << "Melee attack" << endl;

         Player* target = game->getPlayers()[p->getTargetPlayer()];
         game->dealDamageToPlayer(target, p->damage);
      } else if (p->attackType == Player::ATTACK_RANGED) {
         cout << "Ranged attack" << endl;

         Projectile proj(p->pos.x, p->pos.y, p->getTargetPlayer(), p->damage);
         game->assignProjectileId(&proj);
         game->addProjectile(proj);

         int x = p->pos.x;
         int y = p->pos.y;
         unsigned int targetId = p->getTargetPlayer();

         serverMsg.type = MSG_TYPE_PROJECTILE;
         memcpy(serverMsg.buffer, &proj.id, 4);
         memcpy(serverMsg.buffer+4, &x, 4);
         memcpy(serverMsg.buffer+8, &y, 4);
         memcpy(serverMsg.buffer+12, &targetId, 4);
         msgProcessor->broadcastMessage(serverMsg, game->getPlayers());
      } else
         cout << "Invalid attack type: " << p->attackType << endl;
   }

   return gameFinished;
}

void quit(int sig) {
   done = true;
}
