#include "stdafx.h"

extern void solve_puz_Landscaper();
extern void solve_puz_LightenUp();
extern void solve_puz_lightsout();
extern void solve_puz_lightsout_int();
extern void solve_puz_Loopy();

int main(int argc, char **argv)
{
	cout << "a: Landscaper" << endl;
	cout << "i1: Lighten Up" << endl;
	cout << "i2: lightsout" << endl;
	cout << "i3: lightsout_int" << endl;
	cout << "o: Loopy" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_Landscaper();
	else if(str == "i1") solve_puz_LightenUp();
	else if(str == "i2") solve_puz_lightsout();
	else if(str == "i3") solve_puz_lightsout_int();
	else if(str == "o") solve_puz_Loopy();

	return 0;
}