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
extern void solve_puz_MoreOrLess();
extern void solve_puz_Mosaik();
extern void solve_puz_MoveTheBox();
extern void solve_puz_mummymaze();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: magic_square" << endl;
    cout << "a2: Magic 5-gon ring" << endl;
    cout << "a3: Magnets" << endl;
    cout << "a4: Masyu" << endl;
    cout << "a5: Matchstick Equation" << endl;
    cout << "a6: Matchstick Squares" << endl;
    cout << "a7: Matchstick Triangles" << endl;
    cout << "a8: Mathrax" << endl;
    cout << "a9: mazecat" << endl;
    cout << "i1: Mine Ships" << endl;
    cout << "i2: Minesweeper" << endl;
    cout << "i3: Mini-Lits" << endl;
    cout << "i4: Mirrors" << endl;
    cout << "o1: More Or Less" << endl;
    cout << "o2: Mosaik" << endl;
    cout << "o3: Move the Box" << endl;
    cout << "u: mummymaze" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "a1") solve_puz_magic_square();
    else if(str == "a2") solve_puz_Magic5gonRing();
    else if(str == "a3") solve_puz_Magnets();
    else if(str == "a4") solve_puz_Masyu();
    else if(str == "a5") solve_puz_MatchstickEquation();
    else if(str == "a6") solve_puz_MatchstickSquares();
    else if(str == "a7") solve_puz_MatchstickTriangles();
    else if(str == "a8") solve_puz_Mathrax();
    else if(str == "a9") solve_puz_mazecat();
    else if(str == "i1") solve_puz_MineShips();
    else if(str == "i2") solve_puz_Minesweeper();
    else if(str == "i3") solve_puz_MiniLits();
    else if(str == "i4") solve_puz_Mirrors();
    else if(str == "o1") solve_puz_MoreOrLess();
    else if(str == "o2") solve_puz_Mosaik();
    else if(str == "o3") solve_puz_MoveTheBox();
    else if(str == "u") solve_puz_mummymaze();

    return 0;
}
