#include "stdafx.h"

extern void solve_puz_blockedproduct();
extern void solve_puz_blockedview();
extern void solve_puz_bloxorz();
extern void solve_puz_boxitaround();
extern void solve_puz_boxitup();
extern void solve_puz_branches();

int main(int argc, char **argv)
{
	cout << "l1: blocked product" << endl;
	cout << "l2: blocked view" << endl;
	cout << "l3: bloxorz" << endl;
	cout << "o1: box it around" << endl;
	cout << "o2: box it up" << endl;
	cout << "r: branches" << endl;
	string str;
	getline(cin, str);
	if(str == "l1") solve_puz_blockedproduct();
	else if(str == "l2") solve_puz_blockedview();
	else if(str == "l3") solve_puz_bloxorz();
	else if(str == "o1") solve_puz_boxitaround();
	else if(str == "o2") solve_puz_boxitup();
	else if(str == "r") solve_puz_branches();

	return 0;
}