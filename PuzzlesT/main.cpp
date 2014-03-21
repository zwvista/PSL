#include "stdafx.h"

extern void solve_puz_Tapa();
extern void solve_puz_TapAlike();
extern void solve_puz_TapARow();
extern void solve_puz_TapDifferently();
extern void solve_puz_tatami();
extern void solve_puz_TennerGrid();
extern void solve_puz_tents();
extern void solve_puz_turnz();
extern void solve_puz_twinballs();

int main(int argc, char **argv)
{
	cout << "a1: Tapa" << endl;
	cout << "a2: Tap-Alike" << endl;
	cout << "a3: Tap-A-Row" << endl;
	cout << "a4: Tap Differently" << endl;
	cout << "a5: tatami" << endl;
	cout << "e1: Tenner Grid" << endl;
	cout << "e2: tents" << endl;
	cout << "u: turnz" << endl;
	cout << "w: twinballs" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_Tapa();
	else if(str == "a2") solve_puz_TapAlike();
	else if(str == "a3") solve_puz_TapARow();
	else if(str == "a4") solve_puz_TapDifferently();
	else if(str == "a5") solve_puz_tatami();
	else if(str == "e1") solve_puz_TennerGrid();
	else if(str == "e2") solve_puz_tents();
	else if(str == "u") solve_puz_turnz();
	else if(str == "w") solve_puz_twinballs();

	return 0;
}