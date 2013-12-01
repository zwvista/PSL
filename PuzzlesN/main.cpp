#include "stdafx.h"

extern void solve_puz_numeric_paranoia();
extern void solve_puz_nurikabe();

int main(int argc, char **argv)
{
	cout << "u1: numeric_paranoia" << endl;
	cout << "u2: nurikabe" << endl;
	string str;
	getline(cin, str);
	if(str == "u1") solve_puz_numeric_paranoia();
	else if(str == "u2") solve_puz_nurikabe();

	return 0;
}