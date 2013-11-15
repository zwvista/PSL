#include "stdafx.h"

extern void solve_puz_escapology();

int main(int argc, char **argv)
{
	cout << "s: escapology" << endl;
	string str;
	getline(cin, str);
	if(str == "s") solve_puz_escapology();

	return 0;
}