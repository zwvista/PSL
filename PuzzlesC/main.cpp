#include "stdafx.h"

extern void solve_puz_Calcudoku();
extern void solve_puz_Clouds();

int main(int argc, char **argv)
{
	cout << "a: Calcudoku" << endl;
	cout << "l: Clouds" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_Calcudoku();
	else if(str == "l") solve_puz_Clouds();

	return 0;
}