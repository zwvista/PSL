#include "stdafx.h"

extern void solve_puz_Tapa();
extern void solve_puz_TapaIslands();
extern void solve_puz_TapAlike();
extern void solve_puz_TapARow();
extern void solve_puz_TapDifferently();
extern void solve_puz_Tatami();
extern void solve_puz_Tatamino();
extern void solve_puz_Tatamino2();
extern void solve_puz_TennerGrid();
extern void solve_puz_Tents();
extern void solve_puz_TheOddBrick();
extern void solve_puz_Thermometers();
extern void solve_puz_TierraDelFuego();
extern void solve_puz_turnz();
extern void solve_puz_twinballs();

int main(int argc, char **argv)
{
	cout << "a1: Tapa" << endl;
	cout << "a2: Tapa Islands" << endl;
	cout << "a3: Tap-Alike" << endl;
	cout << "a4: Tap-A-Row" << endl;
	cout << "a5: Tap Differently" << endl;
	cout << "a6: Tatami" << endl;
	cout << "a7: Tatamino" << endl;
	cout << "a8: Tatamino2" << endl;
	cout << "e1: Tenner Grid" << endl;
	cout << "e2: Tents" << endl;
	cout << "e3: The Odd Brick" << endl;
	cout << "e4: Thermometers" << endl;
	cout << "i: Tierra Del Fuego" << endl;
	cout << "u: turnz" << endl;
	cout << "w: twinballs" << endl;
	string str;
	getline(cin, str);
	if(str.empty());
	else if(str == "a1") solve_puz_Tapa();
	else if(str == "a2") solve_puz_TapaIslands();
	else if(str == "a3") solve_puz_TapAlike();
	else if(str == "a4") solve_puz_TapARow();
	else if(str == "a5") solve_puz_TapDifferently();
	else if(str == "a6") solve_puz_Tatami();
	else if(str == "a7") solve_puz_Tatamino();
	else if(str == "a8") solve_puz_Tatamino2();
	else if(str == "e1") solve_puz_TennerGrid();
	else if(str == "e2") solve_puz_Tents();
	else if(str == "e3") solve_puz_TheOddBrick();
	else if(str == "e4") solve_puz_Thermometers();
	else if(str == "i") solve_puz_TierraDelFuego();
	else if(str == "u") solve_puz_turnz();
	else if(str == "w") solve_puz_twinballs();

	return 0;
}