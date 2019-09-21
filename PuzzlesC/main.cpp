#include "stdafx.h"

extern void solve_puz_Caffelatte();
extern void solve_puz_Calcudoku();
extern void solve_puz_CarpentersSquare();
extern void solve_puz_CarpentersWall();
extern void solve_puz_CastleBailey();
extern void solve_puz_CastlePatrol();
extern void solve_puz_Cheese();
extern void solve_puz_Clouds();
extern void solve_puz_CoffeeAndSugar();
extern void solve_puz_Consecutives();
extern void solve_puz_CultureTrip();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: Caffelatte" << endl;
    cout << "a2: Calcudoku" << endl;
    cout << "a3: Carpenter's Square" << endl;
    cout << "a4: Carpenter's Wall" << endl;
    cout << "a5: Castle Bailey" << endl;
    cout << "a6: Castle Patrol" << endl;
    cout << "h: Cheese" << endl;
    cout << "l: Clouds" << endl;
    cout << "o1: Coffee And Sugar" << endl;
    cout << "o2: Consecutives" << endl;
    cout << "u: Culture Trip" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Caffelatte();
    else if (str == "a2") solve_puz_Calcudoku();
    else if (str == "a3") solve_puz_CarpentersSquare();
    else if (str == "a4") solve_puz_CarpentersWall();
    else if (str == "a5") solve_puz_CastleBailey();
    else if (str == "a6") solve_puz_CastlePatrol();
    else if (str == "h") solve_puz_Cheese();
    else if (str == "l") solve_puz_Clouds();
    else if (str == "o1") solve_puz_CoffeeAndSugar();
    else if (str == "o2") solve_puz_Consecutives();
    else if (str == "u") solve_puz_CultureTrip();

    return 0;
}
