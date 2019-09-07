#include "stdafx.h"

extern void solve_puz_Walls();
extern void solve_puz_WallHints();
extern void solve_puz_WallSentinels();
extern void solve_puz_Warehouse();
extern void solve_puz_WishSandwich();
extern void solve_puz_Wriggle();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: Walls" << endl;
    cout << "a2: Wall Hints" << endl;
    cout << "a3: Wall Sentinels" << endl;
    cout << "a4: Warehouse" << endl;
    cout << "i: Wish Sandwich" << endl;
    cout << "r: Wriggle" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Walls();
    else if (str == "a2") solve_puz_WallHints();
    else if (str == "a3") solve_puz_WallSentinels();
    else if (str == "a4") solve_puz_Warehouse();
    else if (str == "i") solve_puz_WishSandwich();
    else if (str == "r") solve_puz_Wriggle();

    return 0;
}
