#include "stdafx.h"

extern void solve_puz_Neighbours();
extern void solve_puz_NumberLink();
extern void solve_puz_numeric_paranoia();
extern void solve_puz_nurikabe();

int main(int argc, char **argv)
{
	cout << "e: Neighbours" << endl;
	cout << "u1: NumberLink" << endl;
	cout << "u2: numeric_paranoia" << endl;
	cout << "u3: nurikabe" << endl;
	string str;
	getline(cin, str);
	if(str == "e") solve_puz_Neighbours();
	else if(str == "u1") solve_puz_NumberLink();
	else if(str == "u2") solve_puz_numeric_paranoia();
	else if(str == "u3") solve_puz_nurikabe();

	return 0;
}