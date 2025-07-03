#include "stdafx.h"

extern void solve_puz_UnreliableHints();
extern void solve_puz_Unseen();

int main(int argc, char **argv)
{
    srand(time(0));
    string str;
    println("n1: Unreliable Hints");
    println("n2: Unseen");
    getline(cin, str);
    if (str.empty()) solve_puz_UnreliableHints();
    else if (str == "n1") solve_puz_UnreliableHints();
    else if (str == "n2") solve_puz_Unseen();

    return 0;
}
