#include "stdafx.h"

extern void solve_puz_Abc();
extern void solve_puz_ABCPath();
extern void solve_puz_AbstractPainting();
extern void solve_puz_ADifferentFarmer();
extern void solve_puz_Archipelago();
extern void solve_puz_Arrows();
extern void solve_puz_AssemblyInstructions();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("b1: Abc");
        println("b2: ABC Path");
        println("b3: Abstract Painting");
        println("d: A Different Farmer");
        println("r1: Archipelago");
        println("r2: Arrows");
        println("s: Assembly Instructions");
        getline(cin, str);
        if (str.empty()) solve_puz_Abc();
        else if (str == "b1") solve_puz_Abc();
        else if (str == "b2") solve_puz_ABCPath();
        else if (str == "b3") solve_puz_AbstractPainting();
        else if (str == "d") solve_puz_ADifferentFarmer();
        else if (str == "r1") solve_puz_Archipelago();
        else if (str == "r2") solve_puz_Arrows();
        else if (str == "s") solve_puz_AssemblyInstructions();
    }
    return 0;
}
