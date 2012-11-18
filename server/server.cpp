#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>

#include <mysql/mysql.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in from;
   char buf[1024];

   SSL_load_error_strings();
   ERR_load_BIO_strings();
   OpenSSL_add_all_algorithms();

   if (argc < 2) {
      fprintf(stderr, "ERROR, no port provided\n");
      exit(0);
   }
   
   sock=socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(atoi(argv[1]));
   if (bind(sock,(struct sockaddr *)&server,length)<0) 
       error("binding");
   fromlen = sizeof(struct sockaddr_in);
   while (1) {
       n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);
       if (n < 0) error("recvfrom");
       write(1,"Received a datagram: ",21);
       write(1,buf,n);
       n = sendto(sock,"Got your message\n",17,
                  0,(struct sockaddr *)&from,fromlen);
       if (n  < 0) error("sendto");
   }
   return 0;
}

int dbtest()
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
