#include "stdafx.h"

extern void solve_puz_LakesAndMeadows();
extern void solve_puz_Landscaper();
extern void solve_puz_Landscapes();
extern void solve_puz_LiarLiar();
extern void solve_puz_LightBattleships();
extern void solve_puz_LightenUp();
extern void gen_puz_LightenUp();
extern void solve_puz_Lighthouses();
extern void solve_puz_lightsout();
extern void solve_puz_lightsout_int();
extern void solve_puz_LineSweeper();
extern void solve_puz_Lits();
extern void solve_puz_LoopAndBlocks();
extern void solve_puz_Loopy();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a1: Lakes and Meadows");
    println("a2: Landscaper");
    println("a3: Landscapes");
    println("i1: Liar Liar");
    println("i2: Light Battle Ships");
    println("i3: Lighten Up");
    println("i3g: Lighten Up Gen");
    println("i4: Lighthouses");
    println("i5: lightsout");
    println("i6: lightsout_int");
    println("i7: LineSweeper");
    println("i8: Lits");
    println("o1: Loop and Blocks");
    println("o2: Loopy");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_LakesAndMeadows();
    else if (str == "a2") solve_puz_Landscaper();
    else if (str == "a3") solve_puz_Landscapes();
    else if (str == "i1") solve_puz_LiarLiar();
    else if (str == "i2") solve_puz_LightBattleships();
    else if (str == "i3") solve_puz_LightenUp();
    else if (str == "i3g") gen_puz_LightenUp();
    else if (str == "i4") solve_puz_Lighthouses();
    else if (str == "i5") solve_puz_lightsout();
    else if (str == "i6") solve_puz_lightsout_int();
    else if (str == "i7") solve_puz_LineSweeper();
    else if (str == "i8") solve_puz_Lits();
    else if (str == "o1") solve_puz_LoopAndBlocks();
    else if (str == "o2") solve_puz_Loopy();

    return 0;
}
