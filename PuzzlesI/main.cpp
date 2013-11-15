#include "stdafx.h"

extern void solve_puz_icedin();

int main(int argc, char **argv)
{
	cout << "c: icedin" << endl;
	string str;
	getline(cin, str);
	if(str == "c") solve_puz_icedin();

	return 0;
}