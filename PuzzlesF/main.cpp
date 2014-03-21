#include "stdafx.h"

extern void solve_puz_fenceitup();
extern void solve_puz_fling();
extern void solve_puz_FourMeNot();
extern void solve_puz_FourMeNot2();
extern void solve_puz_futoshiki();

int main(int argc, char **argv)
{
	cout << "e: fence it up" << endl;
	cout << "l: fling" << endl;
	cout << "o1: Four-Me-Not" << endl;
	cout << "o2: Four-Me-Not2" << endl;
	cout << "u: futoshiki" << endl;
	string str;
	getline(cin, str);
	if(str == "e") solve_puz_fenceitup();
	else if(str == "l") solve_puz_fling();
	else if(str == "o1") solve_puz_FourMeNot();
	else if(str == "o2") solve_puz_FourMeNot2();
	else if(str == "u") solve_puz_futoshiki();

	return 0;
}