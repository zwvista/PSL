#include "stdafx.h"

extern void solve_puz_UnreliableHints();
extern void solve_puz_Unseen();

int main(int argc, char **argv)
{
    srand(time(0));
    println("n1: Unreliable Hints");
    println("n2: Unseen");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "n") solve_puz_UnreliableHints();
    else if (str == "n") solve_puz_Unseen();

    return 0;
}
