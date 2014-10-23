#include "stdafx.h"

extern void solve_puz_SheepAndWolves();
extern void solve_puz_ShopAndGas();
extern void solve_puz_Skydoku();
extern void solve_puz_Skyscrapers();
extern void solve_puz_SlantedMaze();
extern void solve_puz_SlitherLink();
extern void solve_puz_snail();
extern void solve_puz_sokoban();
extern void solve_puz_square100();
extern void solve_puz_strimko();
extern void solve_puz_sumscrapers();

int main(int argc, char **argv)
{
	cout << "h1: Sheep & Wolves" << endl;
	cout << "h2: Shop & Gas" << endl;
	cout << "k1: Skydoku" << endl;
	cout << "k2: Skyscrapers" << endl;
	cout << "l1: Slanted Maze" << endl;
	cout << "l2: SlitherLink" << endl;
	cout << "n: snail" << endl;
	cout << "o: sokoban" << endl;
	cout << "q: square 100" << endl;
	cout << "t: strimko" << endl;
	cout << "u: sumscrapers" << endl;
	string str;
	getline(cin, str);
	if(str.empty());
	else if(str == "h1") solve_puz_SheepAndWolves();
	else if(str == "h2") solve_puz_ShopAndGas();
	else if(str == "k1") solve_puz_Skydoku();
	else if(str == "k2") solve_puz_Skyscrapers();
	else if(str == "l1") solve_puz_SlantedMaze();
	else if(str == "l2") solve_puz_SlitherLink();
	else if(str == "n") solve_puz_snail();
	else if(str == "o") solve_puz_sokoban();
	else if(str == "q") solve_puz_square100();
	else if(str == "t") solve_puz_strimko();
	else if(str == "u") solve_puz_sumscrapers();

	return 0;
}