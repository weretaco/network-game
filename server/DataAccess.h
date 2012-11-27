#ifndef _DATA_ACCES_H
#define _DATA_ACCESS_H

#include <string>

#include <mysql/mysql.h>

#include "Player.h"

using namespace std;

class DataAccess {
public:
   DataAccess();
   ~DataAccess();

   int insertPlayer(string username, string password);

   Player *getPlayer(string username);
   int printPlayers();

   int insert(string table, string rows, string values);
   MYSQL_RES *select(string table, string filter);

private:
   MYSQL *connection, mysql;
};

#endif
