#include "stdafx.h"

#include "PSLhelper.h"
#include "Puzzles/BentBridges.h"
#include "Puzzles/Bridges.h"

extern void solve_puz_Banquet();
extern void solve_puz_BalancedTapas();
extern void solve_puz_BattleShips();
extern void solve_puz_bloxorz();
extern void solve_puz_BootyIsland();
extern void solve_puz_BotanicalPark();
extern void solve_puz_BoxItAgain();
extern void solve_puz_BoxItAround();
extern void solve_puz_BoxItUp();
extern void solve_puz_Branches();
extern void solve_puz_Branches2();
extern void solve_puz_BusySeas();
extern void solve_puz_BWTapa();

int main(int argc, char **argv)
{
    srand(time(0));
    string str;
    println("a1: Banquet");
    println("a2: Balanced Tapas");
    println("a3: Battle Ships");
    println("e: Bent Bridges");
    println("l: bloxorz");
    println("o1: Booty Island");
    println("o2: Botanical Park");
    println("o3: Box It Again");
    println("o4: Box It Around");
    println("o5: Box It Up");
    println("r1: Branches");
    println("r12: Branches2");
    println("r2: Bridges");
    println("r2g: Bridges Gen");
    println("u: Busy Seas");
    println("w: B&W Tapa");
    getline(cin, str);
    if (str.empty()) solve_puz_Banquet();
    else if (str == "a1") solve_puz_Banquet();
    else if (str == "a2") solve_puz_BalancedTapas();
    else if (str == "a3") solve_puz_BattleShips();
    else if (str == "e") solve_puz_BentBridges();
    else if (str == "l") solve_puz_bloxorz();
    else if (str == "o1") solve_puz_BootyIsland();
    else if (str == "o2") solve_puz_BotanicalPark();
    else if (str == "o3") solve_puz_BoxItAgain();
    else if (str == "o4") solve_puz_BoxItAround();
    else if (str == "o5") solve_puz_BoxItUp();
    else if (str == "r1") solve_puz_Branches();
    else if (str == "r12") solve_puz_Branches2();
    else if (str == "r2") solve_puz_Bridges();
    else if (str == "r2g") gen_puz_Bridges();
    else if (str == "u") solve_puz_BusySeas();
    else if (str == "w") solve_puz_BWTapa();

    return 0;
}
