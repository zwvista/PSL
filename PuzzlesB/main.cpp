#include "stdafx.h"

extern void solve_puz_BalancedTapas();
extern void solve_puz_BattleShips();
extern void solve_puz_bloxorz();
extern void solve_puz_BootyIsland();
extern void solve_puz_BotanicalPark();
extern void solve_puz_BoxItAround();
extern void solve_puz_BoxItUp();
extern void solve_puz_Branches();
extern void solve_puz_Bridges();
extern void solve_puz_BusySeas();
extern void solve_puz_BWTapa();

int main(int argc, char **argv)
{
	cout << "a1: Balanced Tapas" << endl;
	cout << "a2: Battle Ships" << endl;
	cout << "l: bloxorz" << endl;
	cout << "o1: Booty Island" << endl;
	cout << "o2: Botanical Park" << endl;
	cout << "o3: Box It Around" << endl;
	cout << "o4: Box It Up" << endl;
	cout << "r1: Branches" << endl;
	cout << "r2: Bridges" << endl;
	cout << "u: Busy Seas" << endl;
	cout << "w: B&W Tapa" << endl;
	string str;
	getline(cin, str);
	if(str.empty());
	else if(str == "a1") solve_puz_BalancedTapas();
	else if(str == "a2") solve_puz_BattleShips();
	else if(str == "l") solve_puz_bloxorz();
	else if(str == "o1") solve_puz_BootyIsland();
	else if(str == "o2") solve_puz_BotanicalPark();
	else if(str == "o3") solve_puz_BoxItAround();
	else if(str == "o4") solve_puz_BoxItUp();
	else if(str == "r1") solve_puz_Branches();
	else if(str == "r2") solve_puz_Bridges();
	else if(str == "u") solve_puz_BusySeas();
	else if(str == "w") solve_puz_BWTapa();

	return 0;
}