#include "DataAccess.h"

#include <iostream>

#include <mysql/mysql.h>

using namespace std;

DataAccess::DataAccess()
{
}

DataAccess::~DataAccess()
{
}

int DataAccess::dbtest()
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
