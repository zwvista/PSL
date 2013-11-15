#include "stdafx.h"

extern void solve_puz_magic_square();
extern void solve_puz_magnets();
extern void solve_puz_mazecat();
extern void solve_puz_mummymaze();

int main(int argc, char **argv)
{
	cout << "a1: magic_square" << endl;
	cout << "a2: magnets" << endl;
	cout << "a3: mazecat" << endl;
	cout << "u: mummymaze" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_magic_square();
	else if(str == "a2") solve_puz_magnets();
	else if(str == "a3") solve_puz_mazecat();
	else if(str == "u") solve_puz_mummymaze();

	return 0;
}