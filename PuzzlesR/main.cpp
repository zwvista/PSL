#include "stdafx.h"

extern void solve_puz_RippleEffect();
extern void solve_puz_RobotCrosswords();
extern void solve_puz_RobotFences();
extern void solve_puz_Rooms();
extern void solve_puz_rotate9();
extern void solve_puz_rotationgame();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "i: Ripple Effect" << endl;
    cout << "o1: Robot Crosswords" << endl;
    cout << "o2: Robot Fences" << endl;
    cout << "o3: Rooms" << endl;
    cout << "o4: rotate9" << endl;
    cout << "o5: rotation game" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "i") solve_puz_RippleEffect();
    else if(str == "o1") solve_puz_RobotCrosswords();
    else if(str == "o2") solve_puz_RobotFences();
    else if(str == "o3") solve_puz_Rooms();
    else if(str == "o4") solve_puz_rotate9();
    else if(str == "o5") solve_puz_rotationgame();

    return 0;
}