#include "stdafx.h"

extern void solve_puz_ParkingLot();
extern void solve_puz_parks();
extern void solve_puz_Parks2();
extern void solve_puz_pathfind();
extern void solve_puz_patternpuzzle();
extern void solve_puz_pegsolitary();
extern void solve_puz_Pharaoh();

int main(int argc, char **argv)
{
	cout << "a1: Parking Lot" << endl;
	cout << "a2: parks" << endl;
	cout << "a3: Parks2" << endl;
	cout << "a4: pathfind" << endl;
	cout << "a5: patternpuzzle" << endl;
	cout << "e: pegsolitary" << endl;
	cout << "h: Pharaoh" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_ParkingLot();
	else if(str == "a2") solve_puz_parks();
	else if(str == "a3") solve_puz_Parks2();
	else if(str == "a4") solve_puz_pathfind();
	else if(str == "a5") solve_puz_patternpuzzle();
	else if(str == "e") solve_puz_pegsolitary();
	else if(str == "h") solve_puz_Pharaoh();

	return 0;
}