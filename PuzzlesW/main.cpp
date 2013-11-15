#include "stdafx.h"

extern void solve_puz_wriggle();

int main(int argc, char **argv)
{
	cout << "r: wriggle" << endl;
	string str;
	getline(cin, str);
	if(str == "r") solve_puz_wriggle();

	return 0;
}