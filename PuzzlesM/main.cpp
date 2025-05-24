#include "stdafx.h"

extern void solve_puz_magic_square();
extern void solve_puz_Magic5gonRing();
extern void solve_puz_Magnets();
extern void solve_puz_Masyu();
extern void solve_puz_MatchstickEquation();
extern void solve_puz_MatchstickSquares();
extern void solve_puz_MatchstickTriangles();
extern void solve_puz_Mathrax();
extern void solve_puz_mazecat();
extern void solve_puz_MineShips();
extern void solve_puz_Minesweeper();
extern void solve_puz_MiniLits();
extern void solve_puz_Mirrors();
extern void solve_puz_MixedTatamis();
extern void solve_puz_MoreOrLess();
extern void solve_puz_Mosaik();
extern void solve_puz_MoveTheBox();
extern void solve_puz_mummymaze();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a1: magic_square");
    println("a2: Magic 5-gon ring");
    println("a3: Magnets");
    println("a4: Masyu");
    println("a5: Matchstick Equation");
    println("a6: Matchstick Squares");
    println("a7: Matchstick Triangles");
    println("a8: Mathrax");
    println("a9: mazecat");
    println("i1: Mine Ships");
    println("i2: Minesweeper");
    println("i3: Mini-Lits");
    println("i4: Mirrors");
    println("i5: Mixed Tatamis");
    println("o1: More Or Less");
    println("o2: Mosaik");
    println("o3: Move the Box");
    println("u: mummymaze");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_magic_square();
    else if (str == "a2") solve_puz_Magic5gonRing();
    else if (str == "a3") solve_puz_Magnets();
    else if (str == "a4") solve_puz_Masyu();
    else if (str == "a5") solve_puz_MatchstickEquation();
    else if (str == "a6") solve_puz_MatchstickSquares();
    else if (str == "a7") solve_puz_MatchstickTriangles();
    else if (str == "a8") solve_puz_Mathrax();
    else if (str == "a9") solve_puz_mazecat();
    else if (str == "i1") solve_puz_MineShips();
    else if (str == "i2") solve_puz_Minesweeper();
    else if (str == "i3") solve_puz_MiniLits();
    else if (str == "i4") solve_puz_Mirrors();
    else if (str == "i5") solve_puz_MixedTatamis();
    else if (str == "o1") solve_puz_MoreOrLess();
    else if (str == "o2") solve_puz_Mosaik();
    else if (str == "o3") solve_puz_MoveTheBox();
    else if (str == "u") solve_puz_mummymaze();

    return 0;
}
