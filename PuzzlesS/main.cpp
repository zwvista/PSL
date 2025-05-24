#include "stdafx.h"

extern void solve_puz_Sentinels();
extern void solve_puz_SheepAndWolves();
extern void solve_puz_ShopAndGas();
extern void solve_puz_Skydoku();
extern void solve_puz_Skyscrapers();
extern void solve_puz_SlantedMaze();
extern void solve_puz_SlitherLink();
extern void solve_puz_SlitherLink2();
extern void solve_puz_Snail();
extern void solve_puz_Snake();
extern void solve_puz_sokoban();
extern void solve_puz_Square100();
extern void solve_puz_Stacks();
extern void solve_puz_Steps();
extern void solve_puz_StraightAndTurn();
extern void solve_puz_strimko();
extern void solve_puz_Sukrokuro();
extern void solve_puz_sumscrapers();

int main(int argc, char **argv)
{
    srand(time(0));
    println("e: Sentinels");
    println("h1: Sheep & Wolves");
    println("h2: Shop & Gas");
    println("k1: Skydoku");
    println("k2: Skyscrapers");
    println("l1: Slanted Maze");
    println("l2: SlitherLink");
    println("l3: SlitherLink2");
    println("n1: Snail");
    println("n2: Snake");
    println("o: sokoban");
    println("q: Square 100");
    println("t1: Stacks");
    println("t2: Steps");
    println("t3: Straight and Turn");
    println("t4: strimko");
    println("u1: Sukrokuro");
    println("u2: sumscrapers");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "e") solve_puz_Sentinels();
    else if (str == "h1") solve_puz_SheepAndWolves();
    else if (str == "h2") solve_puz_ShopAndGas();
    else if (str == "k1") solve_puz_Skydoku();
    else if (str == "k2") solve_puz_Skyscrapers();
    else if (str == "l1") solve_puz_SlantedMaze();
    else if (str == "l2") solve_puz_SlitherLink();
    else if (str == "l3") solve_puz_SlitherLink2();
    else if (str == "n1") solve_puz_Snail();
    else if (str == "n2") solve_puz_Snake();
    else if (str == "o") solve_puz_sokoban();
    else if (str == "q") solve_puz_Square100();
    else if (str == "t1") solve_puz_Stacks();
    else if (str == "t2") solve_puz_Steps();
    else if (str == "t3") solve_puz_StraightAndTurn();
    else if (str == "t4") solve_puz_strimko();
    else if (str == "u1") solve_puz_Sukrokuro();
    else if (str == "u2") solve_puz_sumscrapers();

    return 0;
}
