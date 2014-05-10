#include "stdafx.h"

extern void solve_puz_magic_square();
extern void solve_puz_Magnets();
extern void solve_puz_Masyu();
extern void solve_puz_Mathrax();
extern void solve_puz_mazecat();
extern void solve_puz_MineShips();
extern void solve_puz_Minesweeper();
extern void solve_puz_Mirrors();
extern void solve_puz_Mosaic();
extern void solve_puz_mummymaze();

int main(int argc, char **argv)
{
	cout << "a1: magic_square" << endl;
	cout << "a2: Magnets" << endl;
	cout << "a3: Masyu" << endl;
	cout << "a4: Mathrax" << endl;
	cout << "a5: mazecat" << endl;
	cout << "i1: Mine Ships" << endl;
	cout << "i2: Minesweeper" << endl;
	cout << "i3: Mirrors" << endl;
	cout << "o: Mosaic" << endl;
	cout << "u: mummymaze" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_magic_square();
	else if(str == "a2") solve_puz_Magnets();
	else if(str == "a3") solve_puz_Masyu();
	else if(str == "a4") solve_puz_Mathrax();
	else if(str == "a5") solve_puz_mazecat();
	else if(str == "i1") solve_puz_MineShips();
	else if(str == "i2") solve_puz_Minesweeper();
	else if(str == "i3") solve_puz_Mirrors();
	else if(str == "o") solve_puz_Mosaic();
	else if(str == "u") solve_puz_mummymaze();

	return 0;
}