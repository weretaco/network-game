#include <string>

#include <mysql/mysql.h>

using namespace std;

class Database
{
private:
	MYSQL mysql, *conn;

public:
        Database(string host, string username, string password, string database);
        ~Database();

        void createUser(string username, string password);

        string getPassword(string username);
};
