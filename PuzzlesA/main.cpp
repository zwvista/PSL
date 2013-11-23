#include "stdafx.h"

extern void solve_puz_abc();
extern void solve_puz_arrows();

int main(int argc, char **argv)
{
	cout << "b: abc" << endl;
	cout << "r: arrows" << endl;
	string str;
	getline(cin, str);
	if(str == "b") solve_puz_abc();
	else if(str == "r") solve_puz_arrows();

	return 0;
}