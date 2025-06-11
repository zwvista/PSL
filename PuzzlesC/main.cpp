#include "stdafx.h"

extern void solve_puz_Caffelatte();
extern void solve_puz_Calcudoku();
extern void solve_puz_CarpentersSquare();
extern void solve_puz_CarpentersWall();
extern void solve_puz_CastleBailey();
extern void solve_puz_CastlePatrol();
extern void solve_puz_Cheese();
extern void solve_puz_Chocolate();
extern void solve_puz_Clouds();
extern void solve_puz_CoffeeAndSugar();
extern void solve_puz_Consecutives();
extern void solve_puz_CrossroadBlocks();
extern void solve_puz_CrosstownTraffic();
extern void solve_puz_CultureTrip();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a1: Caffelatte");
    println("a2: Calcudoku");
    println("a3: Carpenter's Square");
    println("a4: Carpenter's Wall");
    println("a5: Castle Bailey");
    println("a6: Castle Patrol");
    println("h1: Cheese");
    println("h2: Chocolate");
    println("l: Clouds");
    println("o1: Coffee And Sugar");
    println("o2: Consecutives");
    println("r1: Crossroad Blocks");
    println("r2: Crosstown Traffic");
    println("u: Culture Trip");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a1") solve_puz_Caffelatte();
    else if (str == "a2") solve_puz_Calcudoku();
    else if (str == "a3") solve_puz_CarpentersSquare();
    else if (str == "a4") solve_puz_CarpentersWall();
    else if (str == "a5") solve_puz_CastleBailey();
    else if (str == "a6") solve_puz_CastlePatrol();
    else if (str == "h1") solve_puz_Cheese();
    else if (str == "h2") solve_puz_Chocolate();
    else if (str == "l") solve_puz_Clouds();
    else if (str == "o1") solve_puz_CoffeeAndSugar();
    else if (str == "o2") solve_puz_Consecutives();
    else if (str == "r1") solve_puz_CrossroadBlocks();
    else if (str == "r2") solve_puz_CrosstownTraffic();
    else if (str == "u") solve_puz_CultureTrip();

    return 0;
}
