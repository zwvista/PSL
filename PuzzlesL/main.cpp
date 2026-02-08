#include "stdafx.h"

extern void solve_puz_LakesAndMeadows();
extern void solve_puz_Landscaper();
extern void solve_puz_Landscapes();
extern void solve_puz_LiarLiar();
extern void solve_puz_LightBattleships();
extern void solve_puz_LightConnect();
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
    for(string str; str.empty();) {
        println("a1: Lakes and Meadows");
        println("a2: Landscaper");
        println("a3: Landscapes");
        println("i1: Liar Liar");
        println("ig1: Light Battle Ships");
        println("ig2: Light Connect Puzzle!");
        println("ig3: Lighten Up");
        println("igg: Lighten Up Gen");
        println("ig4: Lighthouses");
        println("ig5: lightsout");
        println("ig6: lightsout_int");
        println("i2: LineSweeper");
        println("i3: Lits");
        println("o1: Loop and Blocks");
        println("o2: Loopy");
        getline(cin, str);
        if (str.empty()) solve_puz_LightBattleships();
        else if (str == "a1") solve_puz_LakesAndMeadows();
        else if (str == "a2") solve_puz_Landscaper();
        else if (str == "a3") solve_puz_Landscapes();
        else if (str == "i1") solve_puz_LiarLiar();
        else if (str == "ig1") solve_puz_LightBattleships();
        else if (str == "ig2") solve_puz_LightConnect();
        else if (str == "ig3") solve_puz_LightenUp();
        else if (str == "igg") gen_puz_LightenUp();
        else if (str == "ig4") solve_puz_Lighthouses();
        else if (str == "ig5") solve_puz_lightsout();
        else if (str == "ig6") solve_puz_lightsout_int();
        else if (str == "i2") solve_puz_LineSweeper();
        else if (str == "i3") solve_puz_Lits();
        else if (str == "o1") solve_puz_LoopAndBlocks();
        else if (str == "o2") solve_puz_Loopy();
    }
    return 0;
}
