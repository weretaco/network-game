#include "DataAccess.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <crypt.h>

#include "LuaLoader.h"

using namespace std;

DataAccess::DataAccess()
{
   LuaLoader luaLoader;

   string database, username, password;

   if (luaLoader.runScript("db_settings.lua")) {
       cout << "Loading settings" << endl;

       database = luaLoader.getValue("database");
       username = luaLoader.getValue("username");
       password = luaLoader.getValue("password");

       cout << database << endl;
       cout << username << endl;
       cout << password << endl;
   } else {
       cout << "Failed to load settings from lua script" << endl;
   }

   mysql_init(&mysql);
   connection = mysql_real_connect(&mysql, "localhost", "pythonAdmin", "pyMaster09*", "pythondb", 0, 0, 0);

   if (connection == NULL) {
      cout << mysql_error(&mysql) << endl;
   }else
      cout << "Connection successful" << endl;
}

DataAccess::~DataAccess()
{
   mysql_close(connection);
   mysql_close(&mysql);
}

int DataAccess::insertPlayer(string username, string password, Player::PlayerClass playerClass)
{
   ostringstream oss;

   string salt = "$1$";
   int random;
   char chr;
   for(int i=0; i<8; i++)
   {
      random = rand() % 62;
      if (random < 26)
         chr = (char)('a'+random);
      else if (random < 52)
         chr = (char)('A'+random-26);
      else
         chr = (char)('0'+random-52);
      salt += chr;
   }
   salt += '$';

   string encrypted(crypt(password.c_str(), salt.c_str()));

   oss << "'" << username << "', '" << encrypted << "', " << playerClass;

   return insert("users", "name, password, class", oss.str());
}

int DataAccess::updatePlayer(Player* p)
{
   ostringstream values, where;

   values << "level=" << p->level << ", experience=" << p->experience << ", honor=" << p->honor << ", wins=" << p->wins << ", losses=" << p->losses << "";
   
   where << "id=" << p->getId() << "";

   return update("users", values.str(), where.str());
}

Player *DataAccess::getPlayer(string username)
{
   MYSQL_RES *result;
   MYSQL_ROW row;
   Player *p;
   ostringstream oss;

   oss << "name='" << username << "'";

   result = select("users", oss.str().c_str());

   cout << "Got result" << endl;

   if (result == NULL) {
      cout << "Error occured" << endl;
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   if ( ( row = mysql_fetch_row(result)) != NULL ) {
      cout << "Creating a new player" << endl;
      p = new Player(string(row[1]), string(row[2]));
      p->setId(atoi(row[0]));
      if (row[3] == NULL) {
         p->setClass(Player::CLASS_NONE);
         cout << "Class from db was NULL" << endl;
      }else {
         p->setClass((Player::PlayerClass)atoi(row[3]));
         cout << "Class from db: " << atoi(row[3]) << endl;
      }
      p->level = atoi(row[4]);
      p->experience = atoi(row[5]);
      p->honor = atoi(row[6]);
      p->wins = atoi(row[7]);
      p->losses = atoi(row[8]);
      cout << "Player class: " << p->playerClass << endl;
      cout << "level: " << p->level << endl;
      cout << "experience: " << p->experience << endl;
      cout << "honor: " << p->honor << endl;
      cout << "wins: " << p->wins << endl;
      cout << "losses: " << p->losses << endl;
      cout << "Loaded player from db" << endl;
   }else {
      cout << "Returned no results for some reason" << endl;
      p = NULL;
   }

   mysql_free_result(result);

   return p;
}

// need to make sure this list is freed
// since we need to create a DataAccess class
// when calling these functions,
// we could free this list in the destructor
list<Player*>* DataAccess::getPlayers()
{
   // This method doesn't seem to ever get used. Decide whether it's actually needed
   MYSQL_RES *result;
   MYSQL_ROW row;

   result = select("users", "");

   if (result == NULL) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   list<Player*>* lstPlayers = new list<Player*>();
   while ( ( row = mysql_fetch_row(result)) != NULL ) {
      cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
      lstPlayers->push_back(new Player(row[1], row[2]));

      // need to assign all the other db values to the player
   }

   mysql_free_result(result);

   return lstPlayers;
}

bool DataAccess::verifyPassword(string password, string encrypted)
{
   string test(crypt(password.c_str(), encrypted.c_str()));

   return encrypted.compare(test) == 0;
}

int* DataAccess::getPlayerRecord(int playerId) {
   MYSQL_RES *result;
   MYSQL_ROW row;
   ostringstream oss;
   int* record = new int[5];

   oss << "id=" << playerId;
   result = select("users", oss.str());

   if ( ( row = mysql_fetch_row(result)) != NULL ) {
      cout << "Retrieved player record successfully" << endl;
      record[0] = atoi(row[4]);   // level
      record[1] = atoi(row[5]);   // experience
      record[2] = atoi(row[6]);   // honor
      record[3] = atoi(row[7]);   // wins
      record[4] = atoi(row[8]);   // losses
      cout << "record[0]:" << record[0] << endl;
      cout << "record[1]:" << record[1] << endl;
      cout << "record[2]:" << record[2] << endl;
      cout << "record[3]:" << record[3] << endl;
      cout << "record[4]:" << record[4] << endl;
   }

   if (result == NULL) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   mysql_free_result(result);

   return record;
}

int** DataAccess::getPlayerGameHistory(int playerId, unsigned int& numGames)
{
   // each array is the score for one game
   // the columns are result, team, blue score, and red score
   // for result 0 is defeat and 1 is victory
   // for team, 0 is blue and 1 is red

   MYSQL_RES *result;
   MYSQL_ROW row;
   ostringstream oss;

   int** gameHistory;

   oss << "user_id=" << playerId;
   result = select("gameHistory", oss.str());

   numGames = mysql_num_rows(result);
   gameHistory = (int**)malloc(sizeof(int*)*numGames);
   cout << "Result has " << numGames << " rows" << endl;

   int i=0;
   while ( ( row = mysql_fetch_row(result)) != NULL ) {
      gameHistory[i] = new int[4];

      int userTeam = atoi(row[2]);
      int blueScore = atoi(row[4]);
      int redScore = atoi(row[3]);
      int gameResult = -1;

      if (blueScore == 3) {
         if (userTeam == 0)
            gameResult = 1;
         else
            gameResult = 0;
      }else if (redScore == 3) {
         if (userTeam == 1)
            gameResult = 1;
         else
            gameResult = 0;
      }else {
         cout << "Recorded game has no team with 3 points" << endl;
      }

      gameHistory[i][0] = gameResult;
      gameHistory[i][1] = userTeam;
      gameHistory[i][2] = blueScore;
      gameHistory[i][3] = redScore;

      i++;
   }

   if (result == NULL) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   mysql_free_result(result);

   return gameHistory;
}

int DataAccess::saveGameHistory(int playerId, int team, int blueScore, int redScore)
{
   ostringstream oss;

   cout << "Saving game to db" << endl;
   oss << playerId << ", " << team << ", " << blueScore << ", " << redScore;

   return insert("gameHistory", "user_id, user_team, blue_score, red_score", oss.str());
}

int DataAccess::insert(string table, string columns, string values)
{
   int query_state;
   ostringstream oss;

   if (connection == NULL) {
       cout << "Error: non database connection exists" << endl;
       return -1;
   }

   oss << "INSERT into " << table << " (" << columns << ") VALUES (" << values << ")";
   cout << "query: " << oss.str() << endl;

   query_state = mysql_query(connection, oss.str().c_str());

   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return -1;
   }

   return 0;
}

int DataAccess::update(string table, string values, string where)
{
   int query_state;
   ostringstream oss;

   if (connection == NULL) {
       cout << "Error: no database connection exists" << endl;
       return -1;
   }

   oss << "UPDATE " << table << " SET " << values << " WHERE " << where;
   cout << "query: " << oss.str() << endl;

   query_state = mysql_query(connection, oss.str().c_str());


   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return -1;
   }

   return 0;
}

MYSQL_RES *DataAccess::select(string table, string filter)
{
   int query_state;
   ostringstream oss;

   if (connection == NULL) {
       cout << "Error: non database connection exists" << endl;
       return NULL;
   }

   oss << "SELECT * FROM " << table;
   if (!filter.empty())
      oss << " WHERE " << filter;
   cout << "executing select query: " << oss.str() << endl;

   query_state = mysql_query(connection, oss.str().c_str());

   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   return mysql_store_result(connection);
}
