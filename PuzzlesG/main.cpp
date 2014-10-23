#include "stdafx.h"

extern void solve_puz_Galaxies();
extern void solve_puz_Gardener();

int main(int argc, char **argv)
{
	cout << "a1: Galaxies" << endl;
	cout << "a2: Gardener" << endl;
	string str;
	getline(cin, str);
	if(str.empty());
	else if(str == "a1") solve_puz_Galaxies();
	else if(str == "a2") solve_puz_Gardener();

	return 0;
}