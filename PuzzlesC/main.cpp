#include "stdafx.h"

extern void solve_puz_Gardener();

int main(int argc, char **argv)
{
	cout << "a: Gardener" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_Gardener();

	return 0;
}