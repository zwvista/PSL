#include "stdafx.h"

extern void solve_puz_Abc();
extern void solve_puz_ABCPath();
extern void solve_puz_AbstractPainting();
extern void solve_puz_ADifferentFarmer();
extern void solve_puz_Archipelago();
extern void solve_puz_Arrows();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "b1: Abc" << endl;
    cout << "b2: ABC Path" << endl;
    cout << "b3: Abstract Painting" << endl;
    cout << "d: A Different Farmer" << endl;
    cout << "r1: Archipelago" << endl;
    cout << "r2: Arrows" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "b1") solve_puz_Abc();
    else if (str == "b2") solve_puz_ABCPath();
    else if (str == "b3") solve_puz_AbstractPainting();
    else if (str == "d") solve_puz_ADifferentFarmer();
    else if (str == "r1") solve_puz_Archipelago();
    else if (str == "r2") solve_puz_Arrows();

    return 0;
}
