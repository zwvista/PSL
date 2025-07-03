#include "stdafx.h"

extern void solve_puz_fullsearch();
extern void solve_puz_pathfind();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("f: fullsearch");
        println("p: pathfind");
        getline(cin, str);
        if (str.empty()) solve_puz_fullsearch();
        else if (str == "f") solve_puz_fullsearch();
        else if (str == "p") solve_puz_pathfind();
    }
    return 0;
}
