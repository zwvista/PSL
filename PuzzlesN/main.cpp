#include "stdafx.h"

extern void solve_puz_Neighbours();
extern void solve_puz_NorthPoleFishing();
extern void solve_puz_NoughtsAndCrosses();
extern void solve_puz_NumberCrossing();
extern void solve_puz_NumberCrosswords();
extern void solve_puz_NumberPath();
extern void solve_puz_NumberPath2();
extern void solve_puz_NumberLink();
extern void solve_puz_numeric_paranoia();
extern void solve_puz_Nurikabe();

int main(int argc, char **argv)
{
    srand(time(0));
    println("e: Neighbours");
    println("o1: North Pole Fishing");
    println("o2: Noughts & Crosses");
    println("u1: Number Crossing");
    println("u2: Number Crosswords");
    println("u3: Number Path");
    println("u32: Number Path 2");
    println("u4: NumberLink");
    println("u5: numeric_paranoia");
    println("u6: Nurikabe");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "e") solve_puz_Neighbours();
    else if (str == "o1") solve_puz_NorthPoleFishing();
    else if (str == "o2") solve_puz_NoughtsAndCrosses();
    else if (str == "u1") solve_puz_NumberCrossing();
    else if (str == "u2") solve_puz_NumberCrosswords();
    else if (str == "u3") solve_puz_NumberPath();
    else if (str == "u32") solve_puz_NumberPath2();
    else if (str == "u4") solve_puz_NumberLink();
    else if (str == "u5") solve_puz_numeric_paranoia();
    else if (str == "u6") solve_puz_Nurikabe();

    return 0;
}
