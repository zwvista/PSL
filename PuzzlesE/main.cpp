#include "stdafx.h"

extern void solve_puz_escapology();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("s: escapology");
        getline(cin, str);
        if (str.empty()) solve_puz_escapology();
        else if (str == "s") solve_puz_escapology();
    }
    return 0;
}
