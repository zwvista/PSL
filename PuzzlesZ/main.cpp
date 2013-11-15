#include "stdafx.h"

extern void solve_puz_zafiro();
extern void solve_puz_zgj();

int main(int argc, char **argv)
{
	cout << "a: zafiro" << endl;
	cout << "g: zgj" << endl;
	string str;
	getline(cin, str);
	if(str == "a") solve_puz_zafiro();
	if(str == "g") solve_puz_zgj();

	return 0;
}