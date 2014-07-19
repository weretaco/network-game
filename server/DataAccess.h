#ifndef _DATA_ACCESS_H
#define _DATA_ACCESS_H

#include <string>
#include <list>

#include <mysql/mysql.h>

#include "../common/Player.h"

using namespace std;

class DataAccess {
public:
   DataAccess();
   ~DataAccess();

   Player* getPlayer(string username);
   list<Player*>* getPlayers();
   bool verifyPassword(string encrypted, string password);
   int insertPlayer(string username, string password, Player::PlayerClass playerClass);
   // this method needs to be more rebust. maybe pass in a player object amd
   // the method could use the player id to find the player and update any
   // attributes that changed
   int updatePlayer(string username, string password);

   int* getPlayerRecord(int playerId);
   int** getPlayerGameHistory(int playerId, unsigned int& numGames);
   int saveGameHistory(int playerId, int team, int blueScore, int redScore);

   int insert(string table, string rows, string values);
   int update(string table, string values, string where);
   MYSQL_RES *select(string table, string filter);

private:
   MYSQL *connection, mysql;
};

#endif
