#include "stdafx.h"

extern void solve_puz_Kakurasu();
extern void solve_puz_Kakuro();
extern void solve_puz_Knightoku();
extern void solve_puz_Kropki();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
    println("a1: Kakurasu");
    println("a2: Kakuro");
    println("n: Knightoku");
    println("r: Kropki");
        getline(cin, str);
        if (str.empty()) solve_puz_Kakurasu();
        else if (str == "a1") solve_puz_Kakurasu();
        else if (str == "a2") solve_puz_Kakuro();
        else if (str == "n") solve_puz_Knightoku();
        else if (str == "r") solve_puz_Kropki();
    }
    return 0;
}
