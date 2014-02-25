#include "stdafx.h"

extern void solve_puz_Walls();
extern void solve_puz_Wriggle();

int main(int argc, char **argv)
{
	cout << "a: Walls" << endl;
	cout << "r: Wriggle" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_Walls();
	else if(str == "r") solve_puz_Wriggle();

	return 0;
}