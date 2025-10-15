#include "stdafx.h"

extern void solve_puz_Neighbours();
extern void solve_puz_NewCarpenterSquare();
extern void solve_puz_Nooks();
extern void solve_puz_NorthPoleFishing();
extern void solve_puz_NoughtsAndCrosses();
extern void solve_puz_NumberChain();
extern void solve_puz_NumberConnect();
extern void solve_puz_NumberCrossing();
extern void solve_puz_NumberCrosswords();
extern void solve_puz_NumberPath();
extern void solve_puz_NumberPath2();
extern void solve_puz_NumberLink();
extern void solve_puz_numeric_paranoia();
extern void solve_puz_Nurikabe();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("e1: Neighbours");
        println("e2: New Carpenter Square");
        println("o1: Nooks");
        println("o2: North Pole Fishing");
        println("o3: Noughts & Crosses");
        println("u1: Number Chain");
        println("u2: Number Connect");
        println("u3: Number Crossing");
        println("u4: Number Crosswords");
        println("u5: Number Path");
        println("u52: Number Path 2");
        println("u6: NumberLink");
        println("u7: numeric_paranoia");
        println("u8: Nurikabe");
        getline(cin, str);
        if (str.empty()) solve_puz_NumberPath();
        else if (str == "e1") solve_puz_Neighbours();
        else if (str == "e2") solve_puz_NewCarpenterSquare();
        else if (str == "o1") solve_puz_Nooks();
        else if (str == "o2") solve_puz_NorthPoleFishing();
        else if (str == "o3") solve_puz_NoughtsAndCrosses();
        else if (str == "u1") solve_puz_NumberChain();
        else if (str == "u2") solve_puz_NumberConnect();
        else if (str == "u3") solve_puz_NumberCrossing();
        else if (str == "u4") solve_puz_NumberCrosswords();
        else if (str == "u5") solve_puz_NumberPath();
        else if (str == "u52") solve_puz_NumberPath2();
        else if (str == "u6") solve_puz_NumberLink();
        else if (str == "u7") solve_puz_numeric_paranoia();
        else if (str == "u8") solve_puz_Nurikabe();
    }
    return 0;
}
