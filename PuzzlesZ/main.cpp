#include "stdafx.h"

extern void solve_puz_zafiro();
extern void solve_puz_ZenGardens();
extern void solve_puz_ZenLandscaper();
extern void solve_puz_ZenSolitaire();
extern void solve_puz_zgj();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
    println("a: zafiro");
    println("e1: Zen Gardens");
    println("e2: Zen Landscaper");
    println("e3: Zen Solitaire");
    println("g: zgj");
    getline(cin, str);
        if (str.empty()) solve_puz_ZenSolitaire();
        else if (str == "a") solve_puz_zafiro();
        else if (str == "e1") solve_puz_ZenGardens();
        else if (str == "e2") solve_puz_ZenLandscaper();
        else if (str == "e3") solve_puz_ZenSolitaire();
        else if (str == "g") solve_puz_zgj();
    }
    return 0;
}
