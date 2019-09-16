#include "stdafx.h"

extern void solve_puz_DigitalBattleships();
extern void solve_puz_DigitalPath();
extern void solve_puz_DisconnectFour();
extern void solve_puz_Domino();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "i1: Digital Battle Ships" << endl;
    cout << "i2: Digital Path" << endl;
    cout << "i3: Disconnect Four" << endl;
    cout << "o: Domino" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "i1") solve_puz_DigitalBattleships();
    else if (str == "i2") solve_puz_DigitalPath();
    else if (str == "i3") solve_puz_DisconnectFour();
    else if (str == "o") solve_puz_Domino();

    return 0;
}
