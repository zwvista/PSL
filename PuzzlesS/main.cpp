#include "stdafx.h"

extern void solve_puz_skyscrapers();
extern void solve_puz_sokoban();
extern void solve_puz_strimko();
extern void solve_puz_sumscrapers();

int main(int argc, char **argv)
{
	cout << "k: skyscrapers" << endl;
	cout << "o: sokoban" << endl;
	cout << "t: strimko" << endl;
	cout << "u: sumscrapers" << endl;
	string str;
	getline(cin, str);
	if(str == "k") solve_puz_skyscrapers();
	else if(str == "o") solve_puz_sokoban();
	else if(str == "t") solve_puz_strimko();
	else if(str == "u") solve_puz_sumscrapers();

	return 0;
}