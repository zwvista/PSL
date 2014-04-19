#include "stdafx.h"

extern void solve_puz_RippleEffect();
extern void solve_puz_rooms();
extern void solve_puz_rotate9();
extern void solve_puz_rotationgame();

int main(int argc, char **argv)
{
	cout << "i: Ripple Effect" << endl;
	cout << "o1: rooms" << endl;
	cout << "o2: rotate9" << endl;
	cout << "o3: rotation game" << endl;
	string str;
	getline(cin, str);
	if(str == "i") solve_puz_RippleEffect();
	else if(str == "o1") solve_puz_rooms();
	else if(str == "o2") solve_puz_rotate9();
	else if(str == "o3") solve_puz_rotationgame();

	return 0;
}