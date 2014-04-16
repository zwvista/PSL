#include "stdafx.h"

extern void solve_puz_FenceItUp();
extern void solve_puz_FenceLits();
extern void solve_puz_Fillomino();
extern void solve_puz_fling();
extern void solve_puz_FourMeNot();
extern void solve_puz_FourMeNot2();
extern void solve_puz_futoshiki();

int main(int argc, char **argv)
{
	cout << "e1: Fence It Up" << endl;
	cout << "e2: FenceLits" << endl;
	cout << "i: Fillomino" << endl;
	cout << "l: fling" << endl;
	cout << "o1: Four-Me-Not" << endl;
	cout << "o2: Four-Me-Not2" << endl;
	cout << "u: futoshiki" << endl;
	string str;
	getline(cin, str);
	if(str == "e1") solve_puz_FenceItUp();
	else if(str == "e2") solve_puz_FenceLits();
	else if(str == "i") solve_puz_Fillomino();
	else if(str == "l") solve_puz_fling();
	else if(str == "o1") solve_puz_FourMeNot();
	else if(str == "o2") solve_puz_FourMeNot2();
	else if(str == "u") solve_puz_futoshiki();

	return 0;
}