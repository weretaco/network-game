#include "crypt.h"

Crypt::Crypt()
{
}

Crypt::~Crypt()
{
}

string Crypt::encrypt(string s)
{
	return s + " encrypted";
}

string Crypt::decrypt(string s)
{
	return s.substr(0, s.length()-10);
}
