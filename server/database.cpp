#include "database.h"

#include <string>
#include <mysql/mysql.h> 

using namespace std;

Database::Database(string host, string username, string password, string database)
{
	mysql_init(&(this->mysql));

        this->conn = mysql_real_connect(&(this->mysql),
		host.c_str(), username.c_str(), password.c_str(), database.c_str(),
		0, 0, 0);

        if (this->conn == NULL) {
		// throw an error
        }
}

Database::~Database()
{
        mysql_close(conn);
}

void Database::createUser(string username, string password)
{
        MYSQL_RES *result;
        MYSQL_ROW row;
        int query_state;

        query_state = mysql_query(conn, (string()+"INSERT INTO users VALUES ("+username+","+password+")").c_str());

        if (query_state !=0) {
		// throw an error
        }

        mysql_free_result(result);
}

string Database::getPassword(string username)
{
	return "defaultPassword";
}
