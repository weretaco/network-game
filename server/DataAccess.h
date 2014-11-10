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
   bool verifyPassword(string encrypted, string password);
   int insertPlayer(string username, string password, Player::PlayerClass playerClass);
   int updatePlayer(Player* player);

   int* getPlayerRecord(int playerId);
   int** getPlayerGameHistory(int playerId, unsigned int& numGames);
   int saveGameHistory(int playerId, int team, int blueScore, int redScore, time_t timeFinished);

   int insert(string table, string rows, string values);
   int update(string table, string values, string where);
   MYSQL_RES *select(string table, string filter);

private:
   MYSQL *connection, mysql;
};

#endif
