#include "stdafx.h"

extern void solve_puz_Walls();
extern void solve_puz_Walls2();
extern void solve_puz_WallHints();
extern void solve_puz_WallSentinels();
extern void solve_puz_Warehouse();
extern void solve_puz_WildlifePark();
extern void solve_puz_WishSandwich();
extern void solve_puz_Wriggle();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a1: Walls");
    println("a12: Walls2");
    println("a2: Wall Hints");
    println("a3: Wall Sentinels");
    println("a4: Warehouse");
    println("i1: Wildlife Park");
    println("i2: Wish Sandwich");
    println("r: Wriggle");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Walls();
    else if (str == "a12") solve_puz_Walls2();
    else if (str == "a2") solve_puz_WallHints();
    else if (str == "a3") solve_puz_WallSentinels();
    else if (str == "a4") solve_puz_Warehouse();
    else if (str == "i1") solve_puz_WildlifePark();
    else if (str == "i2") solve_puz_WishSandwich();
    else if (str == "r") solve_puz_Wriggle();

    return 0;
}
