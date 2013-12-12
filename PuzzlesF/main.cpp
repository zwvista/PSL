#include "stdafx.h"

extern void solve_puz_fenceitup();
extern void solve_puz_fling();
extern void solve_puz_futoshiki();

int main(int argc, char **argv)
{
	cout << "e: fence it up" << endl;
	cout << "l: fling" << endl;
	cout << "u: futoshiki" << endl;
	string str;
	getline(cin, str);
	if(str == "e") solve_puz_fenceitup();
	else if(str == "l") solve_puz_fling();
	else if(str == "u") solve_puz_futoshiki();

	return 0;
}