#include "stdafx.h"

extern void solve_puz_OddsAreEven();
extern void solve_puz_on_the_edge();
extern void solve_puz_openvalve();
extern void solve_puz_Orchards();
extern void solve_puz_OverUnder();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "d: Odds Are Even" << endl;
    cout << "n: on_the_edge" << endl;
    cout << "p: openvalve" << endl;
    cout << "r: Orchards" << endl;
    cout << "v: Over Under" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if (str == "d") solve_puz_OddsAreEven();
    else if (str == "n") solve_puz_on_the_edge();
    else if(str == "p") solve_puz_openvalve();
    else if(str == "r") solve_puz_Orchards();
    else if(str == "v") solve_puz_OverUnder();

    return 0;
}