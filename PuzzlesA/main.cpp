#include "stdafx.h"

extern void solve_puz_Abc();
extern void solve_puz_AbstractPainting();
extern void solve_puz_Archipelago();
extern void solve_puz_Arrows();

int main(int argc, char **argv)
{
    cout << "b1: Abc" << endl;
    cout << "b2: Abstract Painting" << endl;
    cout << "r1: Archipelago" << endl;
    cout << "r2: Arrows" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "b") solve_puz_Abc();
    else if(str == "b2") solve_puz_AbstractPainting();
    else if(str == "r1") solve_puz_Archipelago();
    else if(str == "r2") solve_puz_Arrows();

    return 0;
}