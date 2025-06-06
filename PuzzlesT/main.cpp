#include "stdafx.h"

extern void solve_puz_Tapa();
extern void solve_puz_TapaIslands();
extern void solve_puz_TapAlike();
extern void solve_puz_TapARow();
extern void solve_puz_TapDifferently();
extern void solve_puz_Tatami();
extern void solve_puz_Tatamino();
extern void solve_puz_Tatamino2();
extern void solve_puz_TennerGrid();
extern void solve_puz_Tents();
extern void solve_puz_TetrominoPegs();
extern void solve_puz_TheCityRises();
extern void solve_puz_TheOddBrick();
extern void solve_puz_Thermometers();
extern void solve_puz_TierraDelFuego();
extern void solve_puz_TierraDelFuego2();
extern void solve_puz_TopArrow();
extern void solve_puz_TraceNumbers();
extern void solve_puz_Trebuchet();
extern void solve_puz_TurnTwice();
extern void solve_puz_turnz();
extern void solve_puz_twinballs();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a1: Tapa");
    println("a2: Tapa Islands");
    println("a3: Tap-Alike");
    println("a4: Tap-A-Row");
    println("a5: Tap Differently");
    println("a6: Tatami");
    println("a7: Tatamino");
    println("a8: Tatamino2");
    println("e1: Tenner Grid");
    println("e2: Tents");
    println("e3: Tetromino Pegs");
    println("h1: The City Rises");
    println("h2: The Odd Brick");
    println("h3: Thermometers");
    println("i: Tierra Del Fuego");
    println("i2: Tierra Del Fuego2");
    println("o: Top Arrow");
    println("r1: Trace Numbers");
    println("r2: Trebuchet");
    println("u1: Turn Twice");
    println("u2: turnz");
    println("w: twinballs");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Tapa();
    else if (str == "a2") solve_puz_TapaIslands();
    else if (str == "a3") solve_puz_TapAlike();
    else if (str == "a4") solve_puz_TapARow();
    else if (str == "a5") solve_puz_TapDifferently();
    else if (str == "a6") solve_puz_Tatami();
    else if (str == "a7") solve_puz_Tatamino();
    else if (str == "a8") solve_puz_Tatamino2();
    else if (str == "e1") solve_puz_TennerGrid();
    else if (str == "e2") solve_puz_Tents();
    else if (str == "e3") solve_puz_TetrominoPegs();
    else if (str == "h1") solve_puz_TheCityRises();
    else if (str == "h2") solve_puz_TheOddBrick();
    else if (str == "h3") solve_puz_Thermometers();
    else if (str == "i") solve_puz_TierraDelFuego();
    else if (str == "i2") solve_puz_TierraDelFuego2();
    else if (str == "o") solve_puz_TopArrow();
    else if (str == "r1") solve_puz_TraceNumbers();
    else if (str == "r2") solve_puz_Trebuchet();
    else if (str == "u1") solve_puz_TurnTwice();
    else if (str == "u2") solve_puz_turnz();
    else if (str == "w") solve_puz_twinballs();

    return 0;
}
