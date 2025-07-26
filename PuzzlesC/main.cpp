#include "stdafx.h"

extern void solve_puz_Caffelatte();
extern void solve_puz_Calcudoku();
extern void solve_puz_CarpentersSquare();
extern void solve_puz_CarpentersWall();
extern void solve_puz_CastleBailey();
extern void solve_puz_CastlePatrol();
extern void solve_puz_Cheese();
extern void solve_puz_Chocolate();
extern void solve_puz_CleaningPath();
extern void solve_puz_Clouds();
extern void solve_puz_CloudsAndClears();
extern void solve_puz_CoffeeAndSugar();
extern void solve_puz_ConnectPuzzle();
extern void solve_puz_Consecutives();
extern void solve_puz_CrossroadBlocks();
extern void solve_puz_CrossroadsX();
extern void solve_puz_CrosstownTraffic();
extern void solve_puz_CulturedBranches();
extern void solve_puz_CultureTrip();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("a1: Caffelatte");
        println("a2: Calcudoku");
        println("a3: Carpenter's Square");
        println("a4: Carpenter's Wall");
        println("a5: Castle Bailey");
        println("a6: Castle Patrol");
        println("h1: Cheese");
        println("h2: Chocolate");
        println("l1: Cleaning Path");
        println("l2: Clouds");
        println("l3: Clouds and Clears");
        println("o1: Coffee And Sugar");
        println("o2: Connect Puzzle");
        println("o3: Consecutives");
        println("r1: Crossroad Blocks");
        println("r2: Crossroads X");
        println("r3: Crosstown Traffic");
        println("u1: Cultured Branches");
        println("u2: Culture Trip");
        getline(cin, str);
        if (str.empty()) solve_puz_CastlePatrol();
        else if (str == "a1") solve_puz_Caffelatte();
        else if (str == "a2") solve_puz_Calcudoku();
        else if (str == "a3") solve_puz_CarpentersSquare();
        else if (str == "a4") solve_puz_CarpentersWall();
        else if (str == "a5") solve_puz_CastleBailey();
        else if (str == "a6") solve_puz_CastlePatrol();
        else if (str == "h1") solve_puz_Cheese();
        else if (str == "h2") solve_puz_Chocolate();
        else if (str == "l1") solve_puz_CleaningPath();
        else if (str == "l2") solve_puz_Clouds();
        else if (str == "l3") solve_puz_CloudsAndClears();
        else if (str == "o1") solve_puz_CoffeeAndSugar();
        else if (str == "o2") solve_puz_ConnectPuzzle();
        else if (str == "o3") solve_puz_Consecutives();
        else if (str == "r1") solve_puz_CrossroadBlocks();
        else if (str == "r2") solve_puz_CrossroadsX();
        else if (str == "r3") solve_puz_CrosstownTraffic();
        else if (str == "u1") solve_puz_CulturedBranches();
        else if (str == "u2") solve_puz_CultureTrip();
    }
    return 0;
}
