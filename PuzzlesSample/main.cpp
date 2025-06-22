#include "stdafx.h"

extern void solve_puz_fullsearch();
extern void solve_puz_pathfind();

int main(int argc, char **argv)
{
    srand(time(0));
    println("f: fullsearch");
    println("p: pathfind");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "f") solve_puz_fullsearch();
    else if (str == "p") solve_puz_pathfind();

    return 0;
}
