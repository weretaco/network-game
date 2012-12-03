#include "DataAccess.h"

#include <iostream>
#include <sstream>

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

int DataAccess::insertPlayer(string username, string password)
{
   ostringstream oss;

   oss << "'" << username << "', '" << password << "'";

   return insert("users", "name, password", oss.str());
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

   if ( ( row = mysql_fetch_row(result)) != NULL )
      p = new Player(string(row[1]), string(row[2]));
   else {
      cout << "Returned no results for some reason" << endl;
      p = NULL;
   }

   mysql_free_result(result);

   return p;
}

int DataAccess::printPlayers()
{
   MYSQL_RES *result;
   MYSQL_ROW row;
   ostringstream oss;

   result = select("users", "");

   if (result == NULL) {
      cout << mysql_error(connection) << endl;
      return 1;
   }

   while ( ( row = mysql_fetch_row(result)) != NULL ) {
      cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
   }

   mysql_free_result(result);

   return 0;
}

int DataAccess::insert(string table, string rows, string values)
{
   int query_state;
   ostringstream oss;

   oss << "INSERT into " << table << " (" << rows << ") VALUES (" << values << ")";
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
