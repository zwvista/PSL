#include "stdafx.h"

extern void solve_puz_15pegs();
extern void solve_puz_3dlogic();
extern void solve_puz_8puzzle();

int main(int argc, char **argv)
{
    srand(time(0));
    string str;
    println("1: 15pegs");
    println("3: 3dlogic");
    println("8: 8puzzle");
    getline(cin, str);
    if (str.empty()) solve_puz_15pegs();
    else if (str == "1") solve_puz_15pegs();
    else if (str == "3") solve_puz_3dlogic();
    else if (str == "8") solve_puz_8puzzle();

    return 0;
}
