#include "stdafx.h"

extern void solve_puz_15pegs();
extern void solve_puz_3dlogic();
extern void solve_puz_8puzzle();

int main(int argc, char **argv)
{
    cout << "1: 15pegs" << endl;
    cout << "3: 3dlogic" << endl;
    cout << "8: 8puzzle" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "1") solve_puz_15pegs();
    else if(str == "3") solve_puz_3dlogic();
    else if(str == "8") solve_puz_8puzzle();

    return 0;
}