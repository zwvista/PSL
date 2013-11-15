#include "stdafx.h"

extern void solve_puz_hexrotation();
extern void solve_puz_hiddenpath();
extern void solve_puz_hidoku();
extern void solve_puz_hopover();
extern void solve_puz_hrd();

int main(int argc, char **argv)
{
	cout << "e: hexrotation" << endl;
	cout << "i1: hiddenpath" << endl;
	cout << "i2: hidoku" << endl;
	cout << "o: hopover" << endl;
	cout << "r: hrd" << endl;
	string str;
	getline(cin, str);
	if(str == "e") solve_puz_hexrotation();
	else if(str == "i1") solve_puz_hiddenpath();
	else if(str == "i2") solve_puz_hidoku();
	else if(str == "o") solve_puz_hopover();
	else if(str == "r") solve_puz_hrd();

	return 0;
}