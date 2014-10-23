#include "stdafx.h"

extern void solve_puz_Kakurasu();
extern void solve_puz_Kakuro();
extern void solve_puz_Knightoku();
extern void solve_puz_Kropki();

int main(int argc, char **argv)
{
	cout << "a1: Kakurasu" << endl;
	cout << "a2: Kakuro" << endl;
	cout << "n: Knightoku" << endl;
	cout << "r: Kropki" << endl;
	string str;
	getline(cin, str);
	if(str.empty());
	else if(str == "a1") solve_puz_Kakurasu();
	else if(str == "a2") solve_puz_Kakuro();
	else if(str == "n") solve_puz_Knightoku();
	else if(str == "r") solve_puz_Kropki();

	return 0;
}