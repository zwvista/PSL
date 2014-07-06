#include "stdafx.h"

extern void solve_puz_Hedgehog();
extern void solve_puz_hexrotation();
extern void solve_puz_HiddenPath();
extern void solve_puz_HiddenStar();
extern void solve_puz_Hidoku();
extern void solve_puz_Hitori();
extern void solve_puz_HolidayIsland();
extern void solve_puz_hopover();
extern void solve_puz_hrd();

int main(int argc, char **argv)
{
	cout << "e1: Hedgehog" << endl;
	cout << "e2: hexrotation" << endl;
	cout << "i1: HiddenPath" << endl;
	cout << "i2: HiddenStar" << endl;
	cout << "i3: Hidoku" << endl;
	cout << "i4: Hitori" << endl;
	cout << "o1: holiday island" << endl;
	cout << "o2: hopover" << endl;
	cout << "r: hrd" << endl;
	string str;
	getline(cin, str);
	if(str == "e1") solve_puz_Hedgehog();
	else if(str == "e2") solve_puz_hexrotation();
	else if(str == "i1") solve_puz_HiddenPath();
	else if(str == "i2") solve_puz_HiddenStar();
	else if(str == "i3") solve_puz_Hidoku();
	else if(str == "i4") solve_puz_Hitori();
	else if(str == "o1") solve_puz_HolidayIsland();
	else if(str == "o2") solve_puz_hopover();
	else if(str == "r") solve_puz_hrd();

	return 0;
}