#include "stdafx.h"

extern void solve_puz_Galaxies();
extern void solve_puz_Gardener();
extern void solve_puz_Gems();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: Galaxies" << endl;
    cout << "a2: Gardener" << endl;
    cout << "e: Gems" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Galaxies();
    else if (str == "a2") solve_puz_Gardener();
    else if (str == "e") solve_puz_Gems();

    return 0;
}
