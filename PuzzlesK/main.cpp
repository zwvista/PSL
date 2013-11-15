#include "stdafx.h"

extern void solve_puz_abc();

int main(int argc, char **argv)
{
	cout << "b: abc" << endl;
	string str;
	getline(cin, str);
	if(str == "b") solve_puz_abc();

	return 0;
}