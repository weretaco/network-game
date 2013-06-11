#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cmath>

#include <vector>
#include <map>

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
#include "../common/Message.h"
#include "../common/WorldMap.h"
#include "../common/Player.h"
#include "../common/Projectile.h"

#include "DataAccess.h"

using namespace std;

// from used to be const. Removed that so I could take a reference
// and use it to send messages
bool processMessage(const NETWORK_MSG &clientMsg, struct sockaddr_in &from, map<unsigned int, Player>& mapPlayers, WorldMap* gameMap, unsigned int& unusedPlayerId, NETWORK_MSG &serverMsg, int sock, int &scoreBlue, int &scoreRed);

void updateUnusedPlayerId(unsigned int& id, map<unsigned int, Player>& mapPlayers);
void updateUnusedProjectileId(unsigned int& id, map<unsigned int, Projectile>& mapProjectiles);

// this should probably go somewhere in the common folder
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

Player *findPlayerByName(map<unsigned int, Player> &m, string name)
{
   map<unsigned int, Player>::iterator it;

   for (it = m.begin(); it != m.end(); it++)
   {
      if ( it->second.name.compare(name) == 0 )
         return &(it->second);
   }

   return NULL;
}

Player *findPlayerByAddr(map<unsigned int, Player> &m, const sockaddr_in &addr)
{
   map<unsigned int, Player>::iterator it;

   for (it = m.begin(); it != m.end(); it++)
   {
      if ( it->second.addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
           it->second.addr.sin_port == addr.sin_port )
         return &(it->second);
   }

   return NULL;
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   struct sockaddr_in server;
   struct sockaddr_in from; // info of client sending the message
   NETWORK_MSG clientMsg, serverMsg;
   map<unsigned int, Player> mapPlayers;
   map<unsigned int, Projectile> mapProjectiles;
   unsigned int unusedPlayerId = 1, unusedProjectileId = 1;
   int scoreBlue, scoreRed;

   scoreBlue = 0;
   scoreRed = 0;

   //SSL_load_error_strings();
   //ERR_load_BIO_strings();
   //OpenSSL_add_all_algorithms();

   if (argc < 2) {
      cerr << "ERROR, no port provided" << endl;
      exit(1);
   }

   WorldMap* gameMap = WorldMap::loadMapFromFile("../data/map.txt");

   // add some items to the map. They will be sent out
   // to players when they login
   for (int y=0; y<gameMap->height; y++) {
      for (int x=0; x<gameMap->width; x++) {
         switch (gameMap->getStructure(x, y)) {
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
   if (sock < 0) error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_port=htons(atoi(argv[1]));
   server.sin_addr.s_addr=INADDR_ANY;
   if ( bind(sock, (struct sockaddr *)&server, length) < 0 ) 
      error("binding");

   set_nonblock(sock);

   bool broadcastResponse;
   timespec ts;
   int timeLastUpdated = 0, curTime = 0, timeLastBroadcast = 0;
   while (true) {

      usleep(5000);

      clock_gettime(CLOCK_REALTIME, &ts);
      // make the number smaller so millis can fit in an int
      ts.tv_sec -= 1368000000;
      curTime = ts.tv_sec*1000 + ts.tv_nsec/1000000;

      if (timeLastUpdated == 0 || (curTime-timeLastUpdated) >= 50) {
         timeLastUpdated = curTime;

         map<unsigned int, Player>::iterator it;

         // set targets for all chasing players (or make them attack if they're close enough)
         for (it = mapPlayers.begin(); it != mapPlayers.end(); it++) {
            //Player* p = &it->second;
            it->second.updateTarget(mapPlayers);

            /*
            if (p->isChasing) {
               p->target.x = mapPlayers[p->targetPlayer].pos.x;
               p->target.y = mapPlayers[p->targetPlayer].pos.y;

               if (posDistance(p->pos, p->target.toFloat()) <= p->range) {
                  p->target.x = p->pos.x;
                  p->target.y = p->pos.y;

                  p->isChasing = false;
                  p->isAttacking = true;
                  p->timeAttackStarted = getCurrentMillis();
               }
            }
            */
         }

         // move all players
         // maybe put this in a separate method
         FLOAT_POSITION oldPos;
         bool broadcastMove = false;
         for (it = mapPlayers.begin(); it != mapPlayers.end(); it++) {
            cout << "Starting new for loop iteration" << endl;
            oldPos = it->second.pos;
            if (it->second.move(gameMap)) {

               // check if the move needs to be canceled
               switch(gameMap->getElement(it->second.pos.x/25, it->second.pos.y/25)) {
                  case WorldMap::TERRAIN_NONE:
                  case WorldMap::TERRAIN_OCEAN:
                  case WorldMap::TERRAIN_ROCK:
                  {
                     it->second.pos = oldPos;
                     it->second.target.x = it->second.pos.x;
                     it->second.target.y = it->second.pos.y;
                     broadcastMove = true;
                     break;
                  }
                  default:
                     // if there are no obstacles, do nothing
                     break;
               }

               WorldMap::ObjectType flagType;
               POSITION pos;
               bool flagTurnedIn = false;
               bool flagReturned = false;
               bool ownFlagAtBase = false;
        
               switch(gameMap->getStructure(it->second.pos.x/25, it->second.pos.y/25)) {
                  case WorldMap::STRUCTURE_BLUE_FLAG:
                  {
                     if (it->second.team == 0 && it->second.hasRedFlag)
                     {
                        // check that your flag is at your base
                        pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                        
                        vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
                        vector<WorldMap::Object>::iterator itObjects;

                        for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
                           if (itObjects->type == WorldMap::OBJECT_BLUE_FLAG) {
                              if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12) {
                                 ownFlagAtBase = true;
                                 break;
                              }
                           }
                        }

                        if (ownFlagAtBase) {
                           it->second.hasRedFlag = false;
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
                     if (it->second.team == 1 && it->second.hasBlueFlag)
                     {
                        // check that your flag is at your base
                        pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
                        
                        vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
                        vector<WorldMap::Object>::iterator itObjects;

                        for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
                           if (itObjects->type == WorldMap::OBJECT_RED_FLAG) {
                              if (itObjects->pos.x == pos.x*25+12 && itObjects->pos.y == pos.y*25+12) {
                                 ownFlagAtBase = true;
                                 break;
                              }
                           }
                        }

                        if (ownFlagAtBase) {
                           it->second.hasBlueFlag = false;
                           flagType = WorldMap::OBJECT_BLUE_FLAG;
                           pos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                           flagTurnedIn = true;
                           scoreRed++;
                        }
                     }

                     break;
                  }
               }

               if (flagTurnedIn) {
                  // send an OBJECT message to add the flag back to its spawn point
                  pos.x = pos.x*25+12;
                  pos.y = pos.y*25+12;
                  gameMap->addObject(flagType, pos.x, pos.y);

                  serverMsg.type = MSG_TYPE_OBJECT;
                  gameMap->getObjects()->back().serialize(serverMsg.buffer);

                  map<unsigned int, Player>::iterator it2;
                  for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
                  {
                     if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                        error("sendMessage");
                  }

                  serverMsg.type = MSG_TYPE_SCORE;
                  memcpy(serverMsg.buffer, &scoreBlue, 4);
                  memcpy(serverMsg.buffer+4, &scoreRed, 4);

                  for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
                  {
                     if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                        error("sendMessage");
                  }

                  // this means a PLAYER message will be sent
                  broadcastMove = true;
               }

               // go through all objects and check if the player is close to one and if its their flag
               vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
               vector<WorldMap::Object>::iterator itObjects;
               POSITION structPos;

               for (itObjects = vctObjects->begin(); itObjects != vctObjects->end(); itObjects++) {
                  POSITION pos = itObjects->pos;

                  if (posDistance(it->second.pos, pos.toFloat()) < 10) {
                     if (it->second.team == 0 && 
                        itObjects->type == WorldMap::OBJECT_BLUE_FLAG) {
                        structPos = gameMap->getStructureLocation(WorldMap::STRUCTURE_BLUE_FLAG);
                        flagReturned = true;
                        break;
                     } else if (it->second.team == 1 &&
                        itObjects->type == WorldMap::OBJECT_RED_FLAG) {
                        structPos = gameMap->getStructureLocation(WorldMap::STRUCTURE_RED_FLAG);
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

                  map<unsigned int, Player>::iterator it2;
                  for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
                  {
                     if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                        error("sendMessage");
                  }
               }

               if (broadcastMove) {
                  serverMsg.type = MSG_TYPE_PLAYER;
                  it->second.serialize(serverMsg.buffer);

                  cout << "about to broadcast move" << endl;
                  map<unsigned int, Player>::iterator it2;
                  for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
                  {
                     if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                        error("sendMessage");
                  }
               }
            }

            // check if the player's attack animation is complete
            if (it->second.isAttacking && it->second.timeAttackStarted+it->second.attackCooldown <= getCurrentMillis()) {
               it->second.isAttacking = false;
               cout << "Attack animation is complete" << endl;

               //send everyone an ATTACK message
               cout << "about to broadcast attack" << endl;

               serverMsg.type = MSG_TYPE_ATTACK; 
               memcpy(serverMsg.buffer, &it->second.id, 4);
               memcpy(serverMsg.buffer+4, &it->second.targetPlayer, 4);

               map<unsigned int, Player>::iterator it2;
               for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
               {
                  if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                     error("sendMessage");
               }

               if (it->second.attackType == Player::ATTACK_MELEE) {
                  cout << "Melee attack" << endl;

                  Player* target = &mapPlayers[it->second.targetPlayer];

                  target->health -= it->second.damage;
                  if (target->health < 0)
                     target->health = 0;

                  serverMsg.type = MSG_TYPE_PLAYER;
                  target->serialize(serverMsg.buffer);
               }else if (it->second.attackType == Player::ATTACK_RANGED) {
                  cout << "Ranged attack" << endl;

                  Projectile proj(it->second.pos.x, it->second.pos.y, it->second.targetPlayer, it->second.damage);
                  proj.id = unusedProjectileId;
                  updateUnusedProjectileId(unusedProjectileId, mapProjectiles);
                  mapProjectiles[proj.id] = proj;

                  int x = it->second.pos.x;
                  int y = it->second.pos.y;

                  serverMsg.type = MSG_TYPE_PROJECTILE;
                  memcpy(serverMsg.buffer, &proj.id, 4);
                  memcpy(serverMsg.buffer+4, &x, 4);
                  memcpy(serverMsg.buffer+8, &y, 4);
                  memcpy(serverMsg.buffer+12, &it->second.targetPlayer, 4);
               }else {
                  cout << "Invalid attack type: " << it->second.attackType << endl;
               }

               // broadcast either a PLAYER or PROJECTILE message
               cout << "Broadcasting player or projectile message" << endl;
               for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
               {
                  if (sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                     error("sendMessage");
               }
               cout << "Done broadcasting" << endl;
            }
         }

         cout << "Done with the for loop" << endl;

         // move all projectiles
         cout << "Moving projectiles" << endl;
         map<unsigned int, Projectile>::iterator itProj;
         for (itProj = mapProjectiles.begin(); itProj != mapProjectiles.end(); itProj++) {
            if (itProj->second.move(mapPlayers)) {
               // send a REMOVE_PROJECTILE message
               cout << "send a REMOVE_PROJECTILE message" << endl;
               serverMsg.type = MSG_TYPE_REMOVE_PROJECTILE;
               memcpy(serverMsg.buffer, &itProj->second.id, 4);
               mapProjectiles.erase(itProj->second.id);

               map<unsigned int, Player>::iterator it2;
               cout << "Broadcasting REMOVE_PROJECTILE" << endl;
               for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
               {
                  if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                     error("sendMessage");
               }

               cout << "send a PLAYER message after dealing damage" << endl;
               // send a PLAYER message after dealing damage
               Player* target = &mapPlayers[itProj->second.target];

               target->health -= itProj->second.damage;
               if (target->health < 0)
                  target->health = 0;

               serverMsg.type = MSG_TYPE_PLAYER;
               target->serialize(serverMsg.buffer);

               cout << "Sending a PLAYER message" << endl;
               for (it2 = mapPlayers.begin(); it2 != mapPlayers.end(); it2++)
               {
                  if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
                     error("sendMessage");
               }
            }
         }
      }
      cout << "Done moving projectiles" << endl;

      n = receiveMessage(&clientMsg, sock, &from);

      if (n >= 0) {
         broadcastResponse = processMessage(clientMsg, from, mapPlayers, gameMap, unusedPlayerId, serverMsg, sock, scoreBlue, scoreRed);

         if (broadcastResponse)
         {
            cout << "Should be broadcasting the message" << endl;

            map<unsigned int, Player>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               cout << "Sent message back to " << it->second.name << endl;
               if ( sendMessage(&serverMsg, sock, &(it->second.addr)) < 0 )
                  error("sendMessage");
            }
         }
         else
         {
            cout << "Should be sending back the message" << endl;

            if ( sendMessage(&serverMsg, sock, &from) < 0 )
               error("sendMessage");
         }
      }
   }

   return 0;
}

bool processMessage(const NETWORK_MSG& clientMsg, struct sockaddr_in& from, map<unsigned int, Player>& mapPlayers, WorldMap* gameMap, unsigned int& unusedPlayerId, NETWORK_MSG& serverMsg, int sock, int &scoreBlue, int &scoreRed)
{
   DataAccess da;

   cout << "Received message" << endl;
   cout << "MSG: type: " << clientMsg.type << endl;
   cout << "MSG contents: " << clientMsg.buffer << endl;

   // maybe we should make a message class and have this be a member
   bool broadcastResponse = false;

   // Check that if an invalid message is sent, the client will correctly
   // receive and display the response. Maybe make a special error msg type
   switch(clientMsg.type)
   {
      case MSG_TYPE_REGISTER:
      {
         string username(clientMsg.buffer);
         string password(strchr(clientMsg.buffer, '\0')+1);

         cout << "username: " << username << endl;
         cout << "password: " << password << endl;

         int error = da.insertPlayer(username, password);

         if (!error)
            strcpy(serverMsg.buffer, "Registration successful.");
         else
            strcpy(serverMsg.buffer, "Registration failed. Please try again.");

         serverMsg.type = MSG_TYPE_REGISTER;

         break;
      }
      case MSG_TYPE_LOGIN:
      {
         cout << "Got login message" << endl;

         serverMsg.type = MSG_TYPE_LOGIN;
         
         string username(clientMsg.buffer);
         string password(strchr(clientMsg.buffer, '\0')+1);

         Player* p = da.getPlayer(username);

         if (p == NULL || !da.verifyPassword(password, p->password))
         {
            strcpy(serverMsg.buffer, "Incorrect username or password");
         }
         else if(findPlayerByName(mapPlayers, username) != NULL)
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
         }
         else
         {
            serverMsg.type = MSG_TYPE_PLAYER;

            updateUnusedPlayerId(unusedPlayerId, mapPlayers);
            p->id = unusedPlayerId;
            cout << "new player id: " << p->id << endl;
            p->setAddr(from);

            // choose a random team (either 0 or 1)
            p->team = rand() % 2;

            // choose a random class
            int intClass = rand() % 2;
            switch (intClass) {
               case 0:
                  p->setClass(Player::CLASS_WARRIOR);
                  break;
               case 1:
                  p->setClass(Player::CLASS_RANGER);
                  break;
            }

            // tell the new player about all the existing players
            cout << "Sending other players to new player" << endl;

            map<unsigned int, Player>::iterator it;
            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               it->second.serialize(serverMsg.buffer);

               cout << "sending info about " << it->second.name  << endl;
               cout << "sending id " << it->second.id  << endl;
               if ( sendMessage(&serverMsg, sock, &from) < 0 )
                  error("sendMessage");
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
               if ( sendMessage(&serverMsg, sock, &from) < 0 )
                  error("sendMessage");
            }

            // send the current score
            serverMsg.type = MSG_TYPE_SCORE;
            memcpy(serverMsg.buffer, &scoreBlue, 4);
            memcpy(serverMsg.buffer+4, &scoreRed, 4);
            if ( sendMessage(&serverMsg, sock, &from) < 0 )
               error("sendMessage");

            serverMsg.type = MSG_TYPE_PLAYER;
            p->serialize(serverMsg.buffer);
            cout << "Should be broadcasting the message" << endl;

            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
               cout << "Sent message back to " << it->second.name << endl;
               if ( sendMessage(&serverMsg, sock, &(it->second.addr)) < 0 )
                  error("sendMessage");
            }

            serverMsg.type = MSG_TYPE_LOGIN;
            mapPlayers[unusedPlayerId] = *p;
         }

         delete(p);

         break;
      }
      case MSG_TYPE_LOGOUT:
      {
         string name(clientMsg.buffer);
         cout << "Player logging out: " << name << endl;

         Player *p = findPlayerByName(mapPlayers, name);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "That player is not logged in. This is either a bug, or you're trying to hack the server.");
            cout << "Player not logged in" << endl;
         }
         else if ( p->addr.sin_addr.s_addr != from.sin_addr.s_addr ||
                   p->addr.sin_port != from.sin_port )
         {
            strcpy(serverMsg.buffer, "That player is logged in using a differemt connection. This is either a bug, or you're trying to hack the server.");
            cout << "Player logged in using a different connection" << endl;
         }
         else
         {
            if (p->id < unusedPlayerId)
               unusedPlayerId = p->id;
            mapPlayers.erase(p->id);
            strcpy(serverMsg.buffer, "You have successfully logged out.");
         }

         serverMsg.type = MSG_TYPE_LOGOUT;

         break;
      }
      case MSG_TYPE_CHAT:
      {
         cout << "Got a chat message" << endl;

         Player *p = findPlayerByAddr(mapPlayers, from);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "No player is logged in using this connection. This is either a bug, or you're trying to hack the server.");
         }
         else
         {
            broadcastResponse = true;

            ostringstream oss;
            oss << p->name << ": " << clientMsg.buffer;

            strcpy(serverMsg.buffer, oss.str().c_str());
         }	

         serverMsg.type = MSG_TYPE_CHAT;

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
         
         if ( mapPlayers[id].addr.sin_addr.s_addr == from.sin_addr.s_addr &&
              mapPlayers[id].addr.sin_port == from.sin_port )
         {
            // we need to make sure the player can move here
            if (0 <= x && x < 300 && 0 <= y && y < 300 &&
               gameMap->getElement(x/25, y/25) == WorldMap::TERRAIN_GRASS)
            {
               cout << "valid terrain" << endl;

               mapPlayers[id].target.x = x;
               mapPlayers[id].target.y = y;

               serverMsg.type = MSG_TYPE_PLAYER_MOVE;
               
               memcpy(serverMsg.buffer, &id, 4);
               memcpy(serverMsg.buffer+4, &mapPlayers[id].target.x, 4);
               memcpy(serverMsg.buffer+8, &mapPlayers[id].target.y, 4);

               broadcastResponse = true;
            }
            else
               cout << "Bad terrain detected" << endl;
         }
         else  // nned to send back a message indicating failure
            cout << "Player id (" << id << ") doesn't match sender" << endl;

         break;
      }
      case MSG_TYPE_PICKUP_FLAG:
      { 
         // may want to check the id matches the sender, just like for PLAYER_NOVE
         cout << "PICKUP_FLAG" << endl;

         int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         vector<WorldMap::Object>* vctObjects = gameMap->getObjects();
         vector<WorldMap::Object>::iterator itObjects;

         for (itObjects = vctObjects->begin(); itObjects != vctObjects->end();) {
            POSITION pos = itObjects->pos;
            bool gotFlag = false;

            if (posDistance(mapPlayers[id].pos, pos.toFloat()) < 10) {
               switch (itObjects->type) {
                  case WorldMap::OBJECT_BLUE_FLAG:
                     if (mapPlayers[id].team == 1) {
                        gotFlag = true;
                        mapPlayers[id].hasBlueFlag = true;
                        broadcastResponse = true;
                     }
                     break;
                  case WorldMap::OBJECT_RED_FLAG:
                     if (mapPlayers[id].team == 0) {
                        gotFlag = true;
                        mapPlayers[id].hasRedFlag = true;
                        broadcastResponse = true;
                     }
                     break;
               }

               if (gotFlag) {
                  serverMsg.type = MSG_TYPE_REMOVE_OBJECT;
                  memcpy(serverMsg.buffer, &itObjects->id, 4);

                  map<unsigned int, Player>::iterator it;
                  for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
                  {
                     if ( sendMessage(&serverMsg, sock, &(it->second.addr)) < 0 )
                        error("sendMessage");
                  }

                  // remove the object from the server-side map
                  cout << "size before: " << gameMap->getObjects()->size() << endl;
                  itObjects = vctObjects->erase(itObjects);
                  cout << "size after: " << gameMap->getObjects()->size() << endl;
               }
            }

            if (!gotFlag)
               itObjects++;
         }

         serverMsg.type = MSG_TYPE_PLAYER;
         mapPlayers[id].serialize(serverMsg.buffer);

         break;
      }
      case MSG_TYPE_DROP_FLAG:
      {
         // may want to check the id matches the sender, just like for PLAYER_NOVE
         cout << "DROP_FLAG" << endl;

         int id;

         memcpy(&id, clientMsg.buffer, 4);
         cout << "id: " << id << endl;

         WorldMap::ObjectType flagType = WorldMap::OBJECT_NONE;
         if (mapPlayers[id].hasBlueFlag)
            flagType = WorldMap::OBJECT_BLUE_FLAG;
         else if (mapPlayers[id].hasRedFlag)
            flagType = WorldMap::OBJECT_RED_FLAG;

         gameMap->addObject(flagType, mapPlayers[id].pos.x, mapPlayers[id].pos.y);

         // need to send the OBJECT message too
         serverMsg.type = MSG_TYPE_OBJECT;
         gameMap->getObjects()->back().serialize(serverMsg.buffer);

         map<unsigned int, Player>::iterator it;
         for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
         {
            if ( sendMessage(&serverMsg, sock, &(it->second.addr)) < 0 )
               error("sendMessage");
         }

         mapPlayers[id].hasBlueFlag = false;
         mapPlayers[id].hasRedFlag = false;

         serverMsg.type = MSG_TYPE_PLAYER;
         mapPlayers[id].serialize(serverMsg.buffer);

         broadcastResponse = true;

         break;
      }
      case MSG_TYPE_START_ATTACK:
      {
         cout << "Received a START_ATTACK message" << endl;

         int id, targetId;

         memcpy(&id, clientMsg.buffer, 4);
         memcpy(&targetId, clientMsg.buffer+4, 4);

         Player* source = &mapPlayers[id];
         source->targetPlayer = targetId;
         source->isChasing = true;

         // this is irrelevant since the client doesn't even listen for START_ATTACK messages
         // actually, the client should not ignore this and should instead perform the same movement
         // algorithm on its end (following the target player until in range) that the server does.
         // Once the attacker is in range, the client should stop movement and wait for messages
         // from the server
         serverMsg.type = MSG_TYPE_START_ATTACK;
         memcpy(serverMsg.buffer, &id, 4);
         memcpy(serverMsg.buffer+4, &targetId, 4);
         broadcastResponse = true;

         break;
      }
      case MSG_TYPE_ATTACK:
      {
         cout << "Received am ATTACK message" << endl;
         cout << "ERROR: Clients should not send ATTACK messages" << endl;

         break;
      }
      default:
      {
         strcpy(serverMsg.buffer, "Server error occured. Report this please.");

         serverMsg.type = MSG_TYPE_CHAT;

         break;
      }
   }

   return broadcastResponse;
}

void updateUnusedPlayerId(unsigned int& id, map<unsigned int, Player>& mapPlayers)
{
   while (mapPlayers.find(id) != mapPlayers.end())
      id++;
}

void updateUnusedProjectileId(unsigned int& id, map<unsigned int, Projectile>& mapProjectiles)
{
   while (mapProjectiles.find(id) != mapProjectiles.end())
      id++;
}
