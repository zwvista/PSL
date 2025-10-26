#include "stdafx.h"

extern void solve_puz_Walls();
extern void solve_puz_Walls2();
extern void solve_puz_WallHints();
extern void solve_puz_WallSentinels();
extern void solve_puz_Warehouse();
extern void solve_puz_WaterConnect();
extern void solve_puz_WaterSort();
extern void solve_puz_WildlifePark();
extern void solve_puz_WishSandwich();
extern void solve_puz_Wriggle();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("a1: Walls");
        println("a12: Walls2");
        println("a2: Wall Hints");
        println("a3: Wall Sentinels");
        println("a4: Warehouse");
        println("a5: Water Connect");
        println("a6: Water Sort");
        println("i1: Wildlife Park");
        println("i2: Wish Sandwich");
        println("r: Wriggle");
        getline(cin, str);
        if (str.empty()) solve_puz_WallHints();
        else if (str == "a1") solve_puz_Walls();
        else if (str == "a12") solve_puz_Walls2();
        else if (str == "a2") solve_puz_WallHints();
        else if (str == "a3") solve_puz_WallSentinels();
        else if (str == "a4") solve_puz_Warehouse();
        else if (str == "a5") solve_puz_WaterConnect();
        else if (str == "a6") solve_puz_WaterSort();
        else if (str == "i1") solve_puz_WildlifePark();
        else if (str == "i2") solve_puz_WishSandwich();
        else if (str == "r") solve_puz_Wriggle();
    }
    return 0;
}
