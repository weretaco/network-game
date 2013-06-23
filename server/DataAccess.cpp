#include "DataAccess.h"

#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;

DataAccess::DataAccess()
{
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

int DataAccess::updatePlayer(string username, string password)
{
   ostringstream values, where;

   values << "password='" << password << "'";
   
   where << "name='" << username << "'";

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
      if (row[3] == NULL) {
         p->setClass(Player::CLASS_NONE);
         cout << "Class from db was NULL" << endl;
      }else {
         p->setClass((Player::PlayerClass)atoi(row[3]));
         cout << "Class from db: " << atoi(row[3]) << endl;
      }
      cout << "Player class: " << p->playerClass << endl;
      cout << "Created new player" << endl;
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
   MYSQL_RES *result;
   MYSQL_ROW row;
   ostringstream oss;

   result = select("users", "");

   if (result == NULL) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   list<Player*>* lstPlayers = new list<Player*>();
   while ( ( row = mysql_fetch_row(result)) != NULL ) {
      cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
      lstPlayers->push_back(new Player(row[1], row[2]));
   }

   mysql_free_result(result);

   return lstPlayers;
}

bool DataAccess::verifyPassword(string password, string encrypted)
{
   string test(crypt(password.c_str(), encrypted.c_str()));

   return encrypted.compare(test) == 0;
}

int DataAccess::insert(string table, string columns, string values)
{
   int query_state;
   ostringstream oss;

   oss << "INSERT into " << table << " (" << columns << ") VALUES (" << values << ")";
   cout << "query: " << oss.str() << endl;

   query_state = mysql_query(connection, oss.str().c_str());

   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return 1;
   }

   return 0;
}

int DataAccess::update(string table, string values, string where)
{
   int query_state;
   ostringstream oss;

   oss << "UPDATE " << table << " SET " << values << " WHERE " << where;
   cout << "query: " << oss.str() << endl;

   query_state = mysql_query(connection, oss.str().c_str());

   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return 1;
   }

   return 0;
}

MYSQL_RES *DataAccess::select(string table, string filter)
{
   MYSQL_RES *result;
   int query_state;
   ostringstream oss;

   oss << "SELECT * FROM " << table;
   if (!filter.empty())
      oss << " WHERE " << filter;

   query_state = mysql_query(connection, oss.str().c_str());

   if (query_state != 0) {
      cout << mysql_error(connection) << endl;
      return NULL;
   }

   return mysql_store_result(connection);
}
