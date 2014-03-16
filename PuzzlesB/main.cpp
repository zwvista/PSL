#include "stdafx.h"

extern void solve_puz_BattleShips();
extern void solve_puz_BlockedProduct();
extern void solve_puz_BlockedView();
extern void solve_puz_bloxorz();
extern void solve_puz_BootyIsland();
extern void solve_puz_BotanicalPark();
extern void solve_puz_BoxItAround();
extern void solve_puz_BoxItUp();
extern void solve_puz_Branches();
extern void solve_puz_Bridges();

int main(int argc, char **argv)
{
	cout << "a: Battle Ships" << endl;
	cout << "l1: Blocked Product" << endl;
	cout << "l2: Blocked View" << endl;
	cout << "l3: bloxorz" << endl;
	cout << "o1: Booty Island" << endl;
	cout << "o2: Botanical Park" << endl;
	cout << "o3: Box It Around" << endl;
	cout << "o4: Box It Up" << endl;
	cout << "r1: Branches" << endl;
	cout << "r2: Bridges" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_BattleShips();
	else if(str == "l1") solve_puz_BlockedProduct();
	else if(str == "l2") solve_puz_BlockedView();
	else if(str == "l3") solve_puz_bloxorz();
	else if(str == "o1") solve_puz_BootyIsland();
	else if(str == "o2") solve_puz_BotanicalPark();
	else if(str == "o3") solve_puz_BoxItAround();
	else if(str == "o4") solve_puz_BoxItUp();
	else if(str == "r1") solve_puz_Branches();
	else if(str == "r2") solve_puz_Bridges();

	return 0;
}