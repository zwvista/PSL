#include "stdafx.h"

extern void solve_puz_Farmer();
extern void solve_puz_FenceItUp();
extern void solve_puz_FenceLits();
extern void solve_puz_FenceSentinels();
extern void solve_puz_Fillomino();
extern void solve_puz_fling();
extern void solve_puz_FourMeNot();
extern void solve_puz_FreePlanks();
extern void solve_puz_fullsearch();
extern void solve_puz_FussyWaiter();
extern void solve_puz_Futoshiki();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a: Farmer" << endl;
    cout << "e1: Fence It Up" << endl;
    cout << "e2: FenceLits" << endl;
    cout << "e3: Fence Sentinels" << endl;
    cout << "i: Fillomino" << endl;
    cout << "l: fling" << endl;
    cout << "o: Four-Me-Not" << endl;
    cout << "r: Free Planks" << endl;
    cout << "u1: fullsearch" << endl;
    cout << "u2: Fussy Waiter" << endl;
    cout << "u3: Futoshiki" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a") solve_puz_Farmer();
    else if (str == "e1") solve_puz_FenceItUp();
    else if (str == "e2") solve_puz_FenceLits();
    else if (str == "e3") solve_puz_FenceSentinels();
    else if (str == "i") solve_puz_Fillomino();
    else if (str == "l") solve_puz_fling();
    else if (str == "o") solve_puz_FourMeNot();
    else if (str == "r") solve_puz_FreePlanks();
    else if (str == "u1") solve_puz_fullsearch();
    else if (str == "u2") solve_puz_FussyWaiter();
    else if (str == "u3") solve_puz_Futoshiki();

    return 0;
}
