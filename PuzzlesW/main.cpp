#include "stdafx.h"

extern void solve_puz_walls();
extern void solve_puz_wriggle();

int main(int argc, char **argv)
{
	cout << "a: walls" << endl;
	cout << "r: wriggle" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_walls();
	else if(str == "r") solve_puz_wriggle();

	return 0;
}