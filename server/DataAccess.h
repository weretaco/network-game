#ifndef _DATA_ACCES_H
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

   int insertPlayer(string username, string password, Player::PlayerClass playerClass);
   int updatePlayer(string username, string password);

   Player* getPlayer(string username);
   list<Player*>* getPlayers();
   bool verifyPassword(string encrypted, string password);

   int insert(string table, string rows, string values);
   int update(string table, string values, string where);
   MYSQL_RES *select(string table, string filter);

private:
   MYSQL *connection, mysql;
};

#endif
