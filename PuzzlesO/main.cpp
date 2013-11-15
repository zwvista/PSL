#include "stdafx.h"

extern void solve_puz_on_the_edge();
extern void solve_puz_openvalve();

int main(int argc, char **argv)
{
	cout << "n: on_the_edge" << endl;
	cout << "p: openvalve" << endl;
	string str;
	getline(cin, str);
	if(str == "n") solve_puz_on_the_edge();
	if(str == "p") solve_puz_openvalve();

	return 0;
}