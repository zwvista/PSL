#include "stdafx.h"

extern void solve_puz_Landscaper();
extern void solve_puz_Landscapes();
extern void solve_puz_LightBattleships();
extern void solve_puz_LightenUp();
extern void solve_puz_Lighthouses();
extern void solve_puz_lightsout();
extern void solve_puz_lightsout_int();
extern void solve_puz_LineSweeper();
extern void solve_puz_Lits();
extern void solve_puz_Loopy();

int main(int argc, char **argv)
{
    cout << "a1: Landscaper" << endl;
    cout << "a2: Landscapes" << endl;
    cout << "i1: Light Battle Ships" << endl;
    cout << "i2: Lighten Up" << endl;
    cout << "i3: Lighthouses" << endl;
    cout << "i4: lightsout" << endl;
    cout << "i5: lightsout_int" << endl;
    cout << "i6: LineSweeper" << endl;
    cout << "i7: Lits" << endl;
    cout << "o: Loopy" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "a1") solve_puz_Landscaper();
    else if(str == "a2") solve_puz_Landscapes();
    else if(str == "i1") solve_puz_LightBattleships();
    else if(str == "i2") solve_puz_LightenUp();
    else if(str == "i3") solve_puz_Lighthouses();
    else if(str == "i4") solve_puz_lightsout();
    else if(str == "i5") solve_puz_lightsout_int();
    else if(str == "i6") solve_puz_LineSweeper();
    else if(str == "i7") solve_puz_Lits();
    else if(str == "o") solve_puz_Loopy();

    return 0;
}