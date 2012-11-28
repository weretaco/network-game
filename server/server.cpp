#include "../common/compiler.h"

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
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

#include "Player.h"
#include "DataAccess.h"
#include "../common/message.h"

/*
 Protocol Design

 Client sends a login message
 Server replies with client's position in the world and positions of
 oall other logged in players
 Merver sends player ids along with locations
 This means a newly logged in client will need to know which id is
 assigned to it
 So server needs to send an id message, wait for an ack, and then send
 the location messages
 When a client shuts down, it sends a message to indicate this so the
 server can remove it from the list of connected clients
 Eventually, there'll need to be a way to detect timeouts from clients
 (if they crashed or otherwise failed to send the logout message)
*/

using namespace std;

void processMessage(const NETWORK_MSG &clientMsg, const struct sockaddr_in &from, vector<Player> &vctPlayers, int &num, NETWORK_MSG &serverMsg);

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

void set_nonblock(int sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL,0);
    assert(flags != -1);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   struct sockaddr_in server;
   struct sockaddr_in from; // holds the info on the connected client
   NETWORK_MSG clientMsg, serverMsg;
   vector<Player> vctPlayers;

   srand(time(NULL));
   int num = 500;
   //int num = (rand() % 0) + 1;

   cout << "num: " << num << endl;

   SSL_load_error_strings();
   ERR_load_BIO_strings();
   OpenSSL_add_all_algorithms();

   if (argc < 2) {
      cerr << "ERROR, no port provided" << endl;
      exit(1);
   }

   /*
   DataAccess da;

   da.printPlayers();

   da.insertPlayer("playerName3", "playerPass");
   cout << endl << "Inserted player" << endl << endl;

   Player* p = da.getPlayer("playerName");
   cout << "player name: " << p->name << endl;
   delete(p);

   p = da.getPlayer("playerName3");
   cout << "player name: " << p->name << endl;
   delete(p);

   da.printPlayers();
   cout << endl;
   */
   
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

   while (true) {

      usleep(5000);

      n = receiveMessage(&clientMsg, sock, &from);

      if (n >= 0) {
         cout << "Got a message" << endl;

         processMessage(clientMsg, from, vctPlayers, num, serverMsg);

         cout << "msg: " << serverMsg.buffer << endl;

         n = sendMessage(&serverMsg, sock, &from);
         if (n  < 0)
            error("sendMessage");
      }

   }

   return 0;
}

void processMessage(const NETWORK_MSG &clientMsg, const struct sockaddr_in &from, vector<Player> &vctPlayers, int &num, NETWORK_MSG &serverMsg)
{
   cout << "ip address: " << inet_ntoa(from.sin_addr) << endl;
   cout << "port: " << from.sin_port << endl;
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

         cout << "username: " << username << endl;
         cout << "password: " << password << endl;

         strcpy(serverMsg.buffer, "Registration test");

         serverMsg.type = MSG_TYPE_REGISTER;

         break;
      }
      case MSG_TYPE_LOGIN:
      {
         string username(clientMsg.buffer);
         cout << "Player logging in: " << username << endl;

         Player *p = findPlayerByName(vctPlayers, username);

         if (p == NULL)
         {
            Player newP(username, "");
            newP.setAddr(from);

            vctPlayers.push_back(newP);
            strcpy(serverMsg.buffer, "I'm thinking of a number between 1 and 1000. Guess what it is.");
         }
         else
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
         }

         serverMsg.type = MSG_TYPE_LOGIN;

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
            strcpy(serverMsg.buffer, "You have successfully logged out. You may quit the game.");
         }

         break;
      }
      case MSG_TYPE_CHAT:
      {
         Player *p = findPlayerByAddr(vctPlayers, from);

         if (p == NULL)
         {
            strcpy(serverMsg.buffer, "No player is logged in using this connection. This is either a bug, or you're trying to hack the server.");
         }
         else
         {
            int guess = atoi(clientMsg.buffer);

            cout << "guess: " << guess << endl;

            if (guess < 1 || guess > 1000) {
               strcpy(serverMsg.buffer, "You must guess a number between 1 and 1000");
            }else if(guess > num)
               strcpy(serverMsg.buffer, "The number I'm thinking of is less than that.");
            else if(guess < num)
               strcpy(serverMsg.buffer, "The number I'm thinking of is greater than that.");
            else if(guess == num) {
               strcpy(serverMsg.buffer, "Congratulations! I will now think of a new number.");
               num = (rand() % 1000) + 1;
            }
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
}
