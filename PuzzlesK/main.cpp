#include "stdafx.h"

extern void solve_puz_kakuro();

int main(int argc, char **argv)
{
	cout << "a: kakuro" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_kakuro();

	return 0;
}