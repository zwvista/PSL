#include "stdafx.h"

extern void solve_puz_OddsAreEven();
extern void solve_puz_on_the_edge();
extern void solve_puz_OneUpOrDown();
extern void solve_puz_openvalve();
extern void solve_puz_Orchards();
extern void solve_puz_OverUnder();

int main(int argc, char **argv)
{
    srand(time(0));
    println("d: Odds Are Even");
    println("n1: on_the_edge");
    println("n2: One Up or Down");
    println("p: openvalve");
    println("r: Orchards");
    println("v: Over Under");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "d") solve_puz_OddsAreEven();
    else if (str == "n1") solve_puz_on_the_edge();
    else if (str == "n2") solve_puz_OneUpOrDown();
    else if (str == "p") solve_puz_openvalve();
    else if (str == "r") solve_puz_Orchards();
    else if (str == "v") solve_puz_OverUnder();

    return 0;
}
