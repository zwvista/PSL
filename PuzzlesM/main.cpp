#include "stdafx.h"

extern void solve_puz_magic_square();
extern void solve_puz_magnets();
extern void solve_puz_Masyu();
extern void solve_puz_Mathrax();
extern void solve_puz_mazecat();
extern void solve_puz_Minesweeper();
extern void solve_puz_Mosaic();
extern void solve_puz_mummymaze();

int main(int argc, char **argv)
{
	cout << "a1: magic_square" << endl;
	cout << "a2: magnets" << endl;
	cout << "a3: Masyu" << endl;
	cout << "a4: Mathrax" << endl;
	cout << "a5: mazecat" << endl;
	cout << "i: Minesweeper" << endl;
	cout << "o: Mosaic" << endl;
	cout << "u: mummymaze" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_magic_square();
	else if(str == "a2") solve_puz_magnets();
	else if(str == "a3") solve_puz_Masyu();
	else if(str == "a4") solve_puz_Mathrax();
	else if(str == "a5") solve_puz_mazecat();
	else if(str == "i") solve_puz_Minesweeper();
	else if(str == "o") solve_puz_Mosaic();
	else if(str == "u") solve_puz_mummymaze();

	return 0;
}