#include "stdafx.h"

extern void solve_puz_Venice();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("e: Venice");
        getline(cin, str);
        if (str.empty()) solve_puz_Venice();
        else if (str == "e") solve_puz_Venice();
    }
    return 0;
}
