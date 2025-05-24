#include "stdafx.h"

extern void solve_puz_Rabbits();
extern void solve_puz_RippleEffect();
extern void solve_puz_RobotCrosswords();
extern void solve_puz_RobotFences();
extern void solve_puz_Rome();
extern void solve_puz_Rooms();
extern void solve_puz_rotate9();
extern void solve_puz_rotationgame();
extern void solve_puz_RunInALoop();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a: Rabbits");
    println("i: Ripple Effect");
    println("o1: Robot Crosswords");
    println("o2: Robot Fences");
    println("o3: Rome");
    println("o4: Rooms");
    println("o5: rotate9");
    println("o6: rotation game");
    println("u: Run in a Loop");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a") solve_puz_Rabbits();
    else if (str == "i") solve_puz_RippleEffect();
    else if (str == "o1") solve_puz_RobotCrosswords();
    else if (str == "o2") solve_puz_RobotFences();
    else if (str == "o3") solve_puz_Rome();
    else if (str == "o4") solve_puz_Rooms();
    else if (str == "o5") solve_puz_rotate9();
    else if (str == "o6") solve_puz_rotationgame();
    else if (str == "u") solve_puz_RunInALoop();

    return 0;
}
