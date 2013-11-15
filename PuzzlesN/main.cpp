#include "stdafx.h"

extern void solve_puz_numeric_paranoia();

int main(int argc, char **argv)
{
	cout << "u: numeric_paranoia" << endl;
	string str;
	getline(cin, str);
	if(str == "u") solve_puz_numeric_paranoia();

	return 0;
}