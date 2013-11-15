#include "stdafx.h"

extern void solve_puz_rippleeffect();
extern void solve_puz_rotate9();
extern void solve_puz_rotationgame();

int main(int argc, char **argv)
{
	cout << "i: ripple effect" << endl;
	cout << "o1: rotate9" << endl;
	cout << "o2: rotation game" << endl;
	string str;
	getline(cin, str);
	if(str == "i") solve_puz_rippleeffect();
	else if(str == "o1") solve_puz_rotate9();
	else if(str == "o2") solve_puz_rotationgame();

	return 0;
}