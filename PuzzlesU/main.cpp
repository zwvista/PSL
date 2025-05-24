#include "stdafx.h"

extern void solve_puz_UnreliableHints();

int main(int argc, char **argv)
{
    srand(time(0));
    println("n: Unreliable Hints");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "n") solve_puz_UnreliableHints();

    return 0;
}
