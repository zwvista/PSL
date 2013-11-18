#include "stdafx.h"

extern void solve_puz_tatami();
extern void solve_puz_tents();
extern void solve_puz_turnz();
extern void solve_puz_twinballs();

int main(int argc, char **argv)
{
	cout << "a: tatami" << endl;
	cout << "e: tents" << endl;
	cout << "u: turnz" << endl;
	cout << "w: twinballs" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_tatami();
	else if(str == "e") solve_puz_tents();
	else if(str == "u") solve_puz_turnz();
	else if(str == "w") solve_puz_twinballs();

	return 0;
}