#include "stdafx.h"

extern void solve_puz_landscaper();
extern void solve_puz_lightenup();
extern void solve_puz_lightsout();
extern void solve_puz_lightsout_int();

int main(int argc, char **argv)
{
	cout << "a: landscaper" << endl;
	cout << "i1: lightenup" << endl;
	cout << "i2: lightsout" << endl;
	cout << "i3: lightsout_int" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_landscaper();
	else if(str == "i1") solve_puz_lightenup();
	else if(str == "i2") solve_puz_lightsout();
	else if(str == "i3") solve_puz_lightsout_int();

	return 0;
}