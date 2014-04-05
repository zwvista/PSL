#include "stdafx.h"

extern void solve_puz_hexrotation();
extern void solve_puz_hiddenpath();
extern void solve_puz_hiddenstars();
extern void solve_puz_Hidoku();
extern void solve_puz_Hitori();
extern void solve_puz_holidayisland();
extern void solve_puz_hopover();
extern void solve_puz_hrd();

int main(int argc, char **argv)
{
	cout << "e: hexrotation" << endl;
	cout << "i1: hiddenpath" << endl;
	cout << "i2: hiddenstars" << endl;
	cout << "i3: Hidoku" << endl;
	cout << "i4: Hitori" << endl;
	cout << "o1: holiday island" << endl;
	cout << "o2: hopover" << endl;
	cout << "r: hrd" << endl;
	string str;
	getline(cin, str);
	if(str == "e") solve_puz_hexrotation();
	else if(str == "i1") solve_puz_hiddenpath();
	else if(str == "i2") solve_puz_hiddenstars();
	else if(str == "i3") solve_puz_Hidoku();
	else if(str == "i4") solve_puz_Hitori();
	else if(str == "o1") solve_puz_holidayisland();
	else if(str == "o2") solve_puz_hopover();
	else if(str == "r") solve_puz_hrd();

	return 0;
}