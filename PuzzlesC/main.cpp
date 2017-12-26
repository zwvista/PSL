#include "stdafx.h"

extern void solve_puz_Calcudoku();
extern void solve_puz_CarpentersSquare();
extern void solve_puz_CarpentersWall();
extern void solve_puz_CastleBailey();
extern void solve_puz_Clouds();
extern void solve_puz_Consecutives();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: Calcudoku" << endl;
    cout << "a2: Carpenter's Square" << endl;
    cout << "a3: Carpenter's Wall" << endl;
    cout << "a4: Castle Bailey" << endl;
    cout << "l: Clouds" << endl;
    cout << "o: Consecutives" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Calcudoku();
    else if (str == "a2") solve_puz_CarpentersSquare();
    else if (str == "a3") solve_puz_CarpentersWall();
    else if (str == "a4") solve_puz_CastleBailey();
    else if (str == "l") solve_puz_Clouds();
    else if (str == "o") solve_puz_Consecutives();

    return 0;
}
