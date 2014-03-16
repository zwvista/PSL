#include "stdafx.h"

extern void solve_puz_on_the_edge();
extern void solve_puz_openvalve();
extern void solve_puz_Orchards();

int main(int argc, char **argv)
{
	cout << "n: on_the_edge" << endl;
	cout << "p: openvalve" << endl;
	cout << "r: Orchards" << endl;
	string str;
	getline(cin, str);
	if(str == "n") solve_puz_on_the_edge();
	else if(str == "p") solve_puz_openvalve();
	else if(str == "r") solve_puz_Orchards();

	return 0;
}