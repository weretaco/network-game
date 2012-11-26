#include "../common/compiler.h"

#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <netdb.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>

#include <mysql/mysql.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "player.h"
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

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

player *findPlayerByName(vector<player> &vec, string name)
{
   vector<player>::iterator it;

   for (it = vec.begin(); it != vec.end(); it++)
   {
      if ( it->name.compare(name) == 0 )
         return &(*it);
   }

   return NULL;
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in from;
   NETWORK_MSG clientMsg, serverMsg;
   vector<player> vctPlayers;

   srand(time(NULL));
   int num = (rand() % 1000) + 1;

   cout << "num: " << num << endl;

   SSL_load_error_strings();
   ERR_load_BIO_strings();
   OpenSSL_add_all_algorithms();

   if (argc < 2) {
      fprintf(stderr, "ERROR, no port provided\n");
      exit(0);
   }
   
   sock=socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_port=htons(atoi(argv[1]));
   server.sin_addr.s_addr=INADDR_ANY;
   if ( bind(sock, (struct sockaddr *)&server, length) < 0 ) 
      error("binding");
   fromlen = sizeof(struct sockaddr_in);
   while (true) {
      // if n == 0, means the client disconnected. may want to check this
      n = receiveMessage(&clientMsg, sock, &from);
      if (n < 0)
         error("recieveMessage");
      cout << "MSG: type: " << clientMsg.type << " contents: " << clientMsg.buffer << endl;

      switch(clientMsg.type)
      {
      {
      case MSG_TYPE_LOGIN:
         string name(clientMsg.buffer);
         cout << "Player logging in: " << name << endl;

         player *p = findPlayerByName(vctPlayers, name);

         if (p == NULL)
         {
            vctPlayers.push_back(player(name, from));
            strcpy(serverMsg.buffer, "I'm thinking of a number between 1 and 1000. Guess what it is.");
         }
         else
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
         }

         serverMsg.type = MSG_TYPE_LOGIN;

         break;
      }
      case MSG_TYPE_CHAT:
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

      /*
      if (strcmp(clientMsg.buffer, "Hello") == 0)
      {
         player *p = findPlayerByName(vctPlayers, "Boberty");

         if (p == NULL)
         {
            vctPlayers.push_back(player("Boberty", from));
            strcpy(serverMsg.buffer, "I'm thinking of a number between 1 and 1000. Guess what it is.");
         }
         else
         {
            strcpy(serverMsg.buffer, "Player has already logged in.");
         }
      }else {
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
      */

      cout << "msg: " << serverMsg.buffer << endl;

      n = sendMessage(&serverMsg, sock, &from);
      if (n  < 0)
         error("sendMessage");
   }
   return 0;
}

int dbtest()
{
	MYSQL *connection, mysql;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int query_state;

	mysql_init(&mysql);

	connection = mysql_real_connect(&mysql,"localhost","pythonAdmin","pyMaster09*","pythondb",0,0,0);

	if (connection == NULL) {
		cout << mysql_error(&mysql) << endl;
		return 1;
	}else
		cout << "Connection successful" << endl;

	query_state = mysql_query(connection, "SELECT * FROM users");

	if (query_state !=0) {
		cout << mysql_error(connection) << endl;
		return 1;
	}

	result = mysql_store_result(connection);

	while ( ( row = mysql_fetch_row(result)) != NULL ) {
		cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
	}

	mysql_free_result(result);
	mysql_close(connection);

	cout << "Test finished" << endl;

	return 0;
}
