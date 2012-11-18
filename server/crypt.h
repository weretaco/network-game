#include <string>

using namespace std;

class Crypt
{
public:
	Crypt();
	~Crypt();

	string encrypt(string);
	string decrypt(string);
};
