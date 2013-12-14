#include "stdafx.h"

extern void solve_puz_Calcudoku();

int main(int argc, char **argv)
{
	cout << "a: Calcudoku" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_Calcudoku();

	return 0;
}