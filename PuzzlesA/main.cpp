#include "stdafx.h"

extern void solve_puz_Abc();
extern void solve_puz_Arrows();

int main(int argc, char **argv)
{
	cout << "b: Abc" << endl;
	cout << "r: Arrows" << endl;
	string str;
	getline(cin, str);
	if(str == "b") solve_puz_Abc();
	else if(str == "r") solve_puz_Arrows();

	return 0;
}