#include "stdafx.h"

extern void solve_puz_Sentinels();
extern void solve_puz_SheepAndWolves();
extern void solve_puz_ShopAndGas();
extern void solve_puz_Skydoku();
extern void solve_puz_Skyscrapers();
extern void solve_puz_SlantedMaze();
extern void solve_puz_SlitherCorner();
extern void solve_puz_SlitherLink();
extern void solve_puz_Snail();
extern void solve_puz_Snake();
extern void solve_puz_SnakeIslands();
extern void solve_puz_SnakeMaze();
extern void solve_puz_Snakeomino();
extern void solve_puz_sokoban();
extern void solve_puz_Square100();
extern void solve_puz_SquareMergePuzzle();
extern void solve_puz_Stacks();
extern void solve_puz_Steps();
extern void solve_puz_StraightAndBendLands();
extern void solve_puz_StraightAndTurn();
extern void solve_puz_strimko();
extern void solve_puz_Sukrokuro();
extern void solve_puz_sumscrapers();
extern void solve_puz_SuspendedGravity();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("e: Sentinels");
        println("h1: Sheep & Wolves");
        println("h2: Shop & Gas");
        println("k1: Skydoku");
        println("k2: Skyscrapers");
        println("l1: Slanted Maze");
        println("l2: SlitherCorner");
        println("l3: SlitherLink");
        println("n1: Snail");
        println("n2: Snake");
        println("n3: Snake Islands");
        println("n4: Snake Maze");
        println("n5: Snake-omino");
        println("o: sokoban");
        println("q1: Square 100");
        println("q2: Square Merge Puzzle");
        println("t1: Stacks");
        println("t2: Steps");
        println("t3: Straight and Bend Lands");
        println("t4: Straight and Turn");
        println("t5: strimko");
        println("u1: Sukrokuro");
        println("u2: sumscrapers");
        println("u3: SuspendedGravity");
        getline(cin, str);
        if (str.empty()) solve_puz_SlitherLink();
        else if (str == "e") solve_puz_Snail();
        else if (str == "h1") solve_puz_SheepAndWolves();
        else if (str == "h2") solve_puz_ShopAndGas();
        else if (str == "k1") solve_puz_Skydoku();
        else if (str == "k2") solve_puz_Skyscrapers();
        else if (str == "l1") solve_puz_SlantedMaze();
        else if (str == "l2") solve_puz_SlitherCorner();
        else if (str == "l3") solve_puz_SlitherLink();
        else if (str == "n1") solve_puz_Snail();
        else if (str == "n2") solve_puz_Snake();
        else if (str == "n3") solve_puz_SnakeIslands();
        else if (str == "n4") solve_puz_SnakeMaze();
        else if (str == "n5") solve_puz_Snakeomino();
        else if (str == "o") solve_puz_sokoban();
        else if (str == "q1") solve_puz_Square100();
        else if (str == "q2") solve_puz_SquareMergePuzzle();
        else if (str == "t1") solve_puz_Stacks();
        else if (str == "t2") solve_puz_Steps();
        else if (str == "t3") solve_puz_StraightAndBendLands();
        else if (str == "t4") solve_puz_StraightAndTurn();
        else if (str == "t5") solve_puz_strimko();
        else if (str == "u1") solve_puz_Sukrokuro();
        else if (str == "u2") solve_puz_sumscrapers();
        else if (str == "u3") solve_puz_SuspendedGravity();
    }
    return 0;
}
