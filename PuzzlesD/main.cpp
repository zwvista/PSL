#include "stdafx.h"

extern void solve_puz_DigitalBattleships();
extern void solve_puz_Domino();

int main(int argc, char **argv)
{
	cout << "i: Digital Battle Ships" << endl;
	cout << "o: Domino" << endl;
	string str;
	getline(cin, str);
	if(str == "i") solve_puz_DigitalBattleships();
	else if(str == "o") solve_puz_Domino();

	return 0;
}