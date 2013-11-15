#include "stdafx.h"

extern void solve_puz_blockedview();
extern void solve_puz_bloxorz();
extern void solve_puz_branches();

int main(int argc, char **argv)
{
	cout << "l1: blocked view" << endl;
	cout << "l2: bloxorz" << endl;
	cout << "r: branches" << endl;
	string str;
	getline(cin, str);
	if(str == "l1") solve_puz_blockedview();
	else if(str == "l2") solve_puz_bloxorz();
	else if(str == "r") solve_puz_branches();

	return 0;
}