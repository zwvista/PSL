#include "stdafx.h"

extern void solve_puz_kakurasu();
extern void solve_puz_kakuro();
extern void solve_puz_kropki();

int main(int argc, char **argv)
{
	cout << "a1: kakurasu" << endl;
	cout << "a2: kakuro" << endl;
	cout << "r: kropki" << endl;
	string str;
	getline(cin, str);
	if(str == "a1") solve_puz_kakurasu();
	else if(str == "a2") solve_puz_kakuro();
	else if(str == "r") solve_puz_kropki();

	return 0;
}