#include "stdafx.h"

extern void solve_puz_parks();
extern void solve_puz_pathfind();
extern void solve_puz_patternpuzzle();
extern void solve_puz_pegsolitary();
extern void solve_puz_Pharaoh();

int main(int argc, char **argv)
{
	cout << "a1: parks" << endl;
	cout << "a2: pathfind" << endl;
	cout << "a3: patternpuzzle" << endl;
	cout << "e: pegsolitary" << endl;
	cout << "h: Pharaoh" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_parks();
	else if(str == "a2") solve_puz_pathfind();
	else if(str == "a3") solve_puz_patternpuzzle();
	else if(str == "e") solve_puz_pegsolitary();
	else if(str == "h") solve_puz_Pharaoh();

	return 0;
}