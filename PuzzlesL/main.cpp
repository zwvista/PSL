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
    cout << "a1: Lakes and Meadows" << endl;
    cout << "a2: Landscaper" << endl;
    cout << "a3: Landscapes" << endl;
    cout << "i1: Liar Liar" << endl;
    cout << "i2: Light Battle Ships" << endl;
    cout << "i3: Lighten Up" << endl;
    cout << "i3g: Lighten Up Gen" << endl;
    cout << "i4: Lighthouses" << endl;
    cout << "i5: lightsout" << endl;
    cout << "i6: lightsout_int" << endl;
    cout << "i7: LineSweeper" << endl;
    cout << "i8: Lits" << endl;
    cout << "o1: Loop and Blocks" << endl;
    cout << "o2: Loopy" << endl;
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
