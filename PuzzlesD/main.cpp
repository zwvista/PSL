#include "stdafx.h"

extern void solve_puz_DesertDunes();
extern void solve_puz_DigitalBattleships();
extern void solve_puz_DigitalPath();
extern void solve_puz_DirectionalPlanks();
extern void solve_puz_DisconnectFour();
extern void solve_puz_Domino();

int main(int argc, char **argv)
{
    srand(time(0));
    println("e: Desert Dunes");
    println("i1: Digital Battle Ships");
    println("i2: Digital Path");
    println("i3: Directional Planks");
    println("i4: Disconnect Four");
    println("o: Domino");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "e") solve_puz_DesertDunes();
    else if (str == "i1") solve_puz_DigitalBattleships();
    else if (str == "i2") solve_puz_DigitalPath();
    else if (str == "i3") solve_puz_DirectionalPlanks();
    else if (str == "i4") solve_puz_DisconnectFour();
    else if (str == "o") solve_puz_Domino();

    return 0;
}
