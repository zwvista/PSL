#include "stdafx.h"

extern void solve_puz_Domino();

int main(int argc, char **argv)
{
	cout << "o: Domino" << endl;
	string str;
	getline(cin, str);
	if(str == "o") solve_puz_Domino();

	return 0;
}