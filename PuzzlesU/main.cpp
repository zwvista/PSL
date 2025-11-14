#include "stdafx.h"

extern void solve_puz_Underground();
extern void solve_puz_UnreliableHints();
extern void solve_puz_Unseen();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("n1: Underground");
        println("n2: Unreliable Hints");
        println("n3: Unseen");
        getline(cin, str);
        if (str.empty()) solve_puz_Unseen();
        else if (str == "n1") solve_puz_Underground();
        else if (str == "n2") solve_puz_UnreliableHints();
        else if (str == "n3") solve_puz_Unseen();
    }
    return 0;
}
