#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <fcntl.h>
#include <assert.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../common/Compiler.h"
#include "../common/Message.h"
#include "../common/Common.h"

#include "Player.h"
#include "DataAccess.h"

using namespace std;

bool processMessage(const NETWORK_MSG &clientMsg, const struct sockaddr_in &from, vector<Player> &vctPlayers, NETWORK_MSG &serverMsg);

// this should probably go somewhere in the common folder
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

Player *findPlayerByName(vector<Player> &vec, string name)
{
   vector<Player>::iterator it;

   for (it = vec.begin(); it != vec.end(); it++)
   {
      if ( it->name.compare(name) == 0 )
         return &(*it);
   }

   return NULL;
}

Player *findPlayerByAddr(vector<Player> &vec, const sockaddr_in &addr)
{
   vector<Player>::iterator it;

   for (it = vec.begin(); it != vec.end(); it++)
   {
      if ( it->addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
           it->addr.sin_port == addr.sin_port )
         return &(*it);
   }

   return NULL;
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   struct sockaddr_in server;
   struct sockaddr_in from; // info of client sending the message
   NETWORK_MSG clientMsg, serverMsg;
   vector<Player> vctPlayers;

   SSL_load_error_strings();
   ERR_load_BIO_strings();
   OpenSSL_add_all_algorithms();

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

         broadcastResponse = processMessage(clientMsg, from, vctPlayers, serverMsg);

         cout << "msg: " << serverMsg.buffer << endl;

         if (broadcastResponse)
         {
            cout << "Should be broadcasting the message" << endl;

            vector<Player>::iterator it;

            for (it = vctPlayers.begin(); it != vctPlayers.end(); it++)
            {
               if ( sendMessage(&serverMsg, sock, &(it->addr)) < 0 )
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

bool processMessage(const NETWORK_MSG &clientMsg, const struct sockaddr_in &from, vector<Player> &vctPlayers, NETWORK_MSG &serverMsg)
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
         cout << "Player logging in: " << username << endl;

         Player* p = da.getPlayer(username);

         if (p == NULL || p->password != password)
         {
            strcpy(serverMsg.buffer, "Incorrect username or password");
         }
         else if(findPlayerByName(vctPlayers, username) != NULL)
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
         }
         else
         {
            Player newP(username, "");
            newP.setAddr(from);

            vctPlayers.push_back(newP);
            strcpy(serverMsg.buffer, "Login successful. Enjoy chatting with other players.");
         }

         serverMsg.type = MSG_TYPE_LOGIN;
   
         delete(p);

         break;
      }
      case MSG_TYPE_LOGOUT:
      {
         string name(clientMsg.buffer);
         cout << "Player logging out: " << name << endl;

         Player *p = findPlayerByName(vctPlayers, name);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "That player is not logged in. This is either a bug, or you're trying to hack the server.");
         }
         else if( p->addr.sin_addr.s_addr != from.sin_addr.s_addr ||
                  p->addr.sin_port != from.sin_port )
         {
            strcpy(serverMsg.buffer, "That player is logged in using a differemt connection. This is either a bug, or you're trying to hack the server.");
         }
         else
         {
            vctPlayers.erase((vector<Player>::iterator)p);
            strcpy(serverMsg.buffer, "You have successfully logged out.");
         }

         break;
      }
      case MSG_TYPE_CHAT:
      {
         cout << "Got a chat message" << endl;

         Player *p = findPlayerByAddr(vctPlayers, from);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "No player is logged in using this connection. This is either a bug, or you're trying to hack the server.");
         }
         else
         {
            broadcastResponse = true;

            stringstream ss;
            ss << p->name << ": " << clientMsg.buffer;

            strcpy(serverMsg.buffer, ss.str().c_str());
         }	

         serverMsg.type = MSG_TYPE_CHAT;

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

