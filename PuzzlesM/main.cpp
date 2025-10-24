#include "stdafx.h"

extern void solve_puz_magic_square();
extern void solve_puz_Magic5gonRing();
extern void solve_puz_Magnets();
extern void solve_puz_MaketheDifference();
extern void solve_puz_Masyu();
extern void solve_puz_MatchstickEquation();
extern void solve_puz_MatchstickSquares();
extern void solve_puz_MatchstickTriangles();
extern void solve_puz_Mathrax();
extern void solve_puz_mazecat();
extern void solve_puz_MineShips();
extern void solve_puz_MineSlither();
extern void solve_puz_Minesweeper();
extern void solve_puz_MiniLits();
extern void solve_puz_Mirrors();
extern void solve_puz_MirrorsExtended();
extern void solve_puz_MixedTatamis();
extern void solve_puz_MondrianLoop();
extern void solve_puz_MoreOrLess();
extern void solve_puz_Mosaik();
extern void solve_puz_MoveTheBox();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("ag1: magic_square");
        println("ag2: Magic 5-gon ring");
        println("ag3: Magnets");
        println("ak: Make the Difference");
        println("as: Masyu");
        println("at1: Matchstick Equation");
        println("at2: Matchstick Squares");
        println("at3: Matchstick Triangles");
        println("at4: Mathrax");
        println("az: mazecat");
        println("i1: Mine Ships");
        println("i2: MineSlither");
        println("i3: Minesweeper");
        println("i4: Mini-Lits");
        println("i5: Mirrors");
        println("i6: Mirrors, extended");
        println("i7: Mixed Tatamis");
        println("o1: Mondrian Loop");
        println("o2: More Or Less");
        println("o3: Mosaik");
        println("o4: Move the Box");
        getline(cin, str);
        if (str.empty()) solve_puz_Magnets();
        else if (str == "ag1") solve_puz_magic_square();
        else if (str == "ag2") solve_puz_Magic5gonRing();
        else if (str == "ag3") solve_puz_Magnets();
        else if (str == "ak") solve_puz_MaketheDifference();
        else if (str == "as") solve_puz_Masyu();
        else if (str == "at1") solve_puz_MatchstickEquation();
        else if (str == "at2") solve_puz_MatchstickSquares();
        else if (str == "at3") solve_puz_MatchstickTriangles();
        else if (str == "at4") solve_puz_Mathrax();
        else if (str == "az") solve_puz_mazecat();
        else if (str == "i1") solve_puz_MineShips();
        else if (str == "i2") solve_puz_MineSlither();
        else if (str == "i3") solve_puz_Minesweeper();
        else if (str == "i4") solve_puz_MiniLits();
        else if (str == "i5") solve_puz_Mirrors();
        else if (str == "i6") solve_puz_MirrorsExtended();
        else if (str == "i7") solve_puz_MixedTatamis();
        else if (str == "o1") solve_puz_MondrianLoop();
        else if (str == "o2") solve_puz_MoreOrLess();
        else if (str == "o3") solve_puz_Mosaik();
        else if (str == "o4") solve_puz_MoveTheBox();
    }
    return 0;
}
