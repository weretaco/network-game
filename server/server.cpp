#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>

#include <vector>
#include <map>

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
#include "../common/Player.h"

#include "DataAccess.h"

using namespace std;

bool processMessage(const NETWORK_MSG &clientMsg, const struct sockaddr_in &from, map<unsigned int, Player>& mapPlayers, unsigned int& unusedId, NETWORK_MSG &serverMsg);

void updateUnusedId(unsigned int& id, map<unsigned int, Player>& mapPlayers);

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

void broadcastPlayerPositions(map<unsigned int, Player> &m, int sock)
{
   map<unsigned int, Player>::iterator it, it2;
   NETWORK_MSG serverMsg;

   serverMsg.type = MSG_TYPE_PLAYER;   

   for (it = m.begin(); it != m.end(); it++)
   {
      it->second.serialize(serverMsg.buffer);

      for (it2 = m.begin(); it2 != m.end(); it2++)
      {
         if ( sendMessage(&serverMsg, sock, &(it2->second.addr)) < 0 )
            error("sendMessage");
      }
   }
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   struct sockaddr_in server;
   struct sockaddr_in from; // info of client sending the message
   NETWORK_MSG clientMsg, serverMsg;
   map<unsigned int, Player> mapPlayers;
   unsigned int unusedId = 1;

   //SSL_load_error_strings();
   //ERR_load_BIO_strings();
   //OpenSSL_add_all_algorithms();

   if (argc < 2) {
      cerr << "ERROR, no port provided" << endl;
      exit(1);
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
   while (true) {

      usleep(5000);

      n = receiveMessage(&clientMsg, sock, &from);

      if (n >= 0) {
         cout << "Got a message" << endl;

         broadcastResponse = processMessage(clientMsg, from, mapPlayers, unusedId, serverMsg);

         // probably replace this with a function that prints based on the
         // message type
         cout << "msg: " << serverMsg.buffer << endl;
         cout << "broadcastResponse: " << broadcastResponse << endl;
         if (broadcastResponse)
         {
            cout << "Should be broadcasting the message" << endl;

            map<unsigned int, Player>::iterator it;

            for (it = mapPlayers.begin(); it != mapPlayers.end(); it++)
            {
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

         broadcastPlayerPositions(mapPlayers, sock);
      }
   }

   return 0;
}

bool processMessage(const NETWORK_MSG& clientMsg, const struct sockaddr_in& from, map<unsigned int, Player>& mapPlayers, unsigned int& unusedId, NETWORK_MSG& serverMsg)
{
   DataAccess da;

   cout << "ip address: " << inet_ntoa(from.sin_addr) << endl;
   cout << "port: " << from.sin_port << endl;
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
            p->setAddr(from);
            updateUnusedId(unusedId, mapPlayers);
            p->id = unusedId;
            mapPlayers[unusedId] = *p;

            // sendd back the new player info to the user
            p->serialize(serverMsg.buffer);
         }

         serverMsg.type = MSG_TYPE_LOGIN;
   
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
            if (p->id < unusedId)
               unusedId = p->id;
            mapPlayers.erase(p->id);
            strcpy(serverMsg.buffer, "You have successfully logged out.");
            cout << "Player logged out successfuly" << endl;
         }

         // should really be serverMsg.type = MSG_TYPE_LOGOUT;
         serverMsg.type = MSG_TYPE_LOGIN;

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
         istringstream iss;
         iss.str(clientMsg.buffer);

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
            memcpy(&mapPlayers[id].pos.x, clientMsg.buffer+4, 4);
            memcpy(&mapPlayers[id].pos.y, clientMsg.buffer+8, 4);

            serverMsg.type = MSG_TYPE_PLAYER_MOVE;
            memcpy(serverMsg.buffer, clientMsg.buffer, 12);

            broadcastResponse = true;
         }
         else  // nned to send back a message indicating failure
            cout << "Player id (" << id << ") doesn't match sender" << endl;

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

void updateUnusedId(unsigned int& id, map<unsigned int, Player>& mapPlayers)
{
   while (mapPlayers.find(id) != mapPlayers.end())
      id++;
}
