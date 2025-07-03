#include "stdafx.h"

extern void solve_puz_escapology();

int main(int argc, char **argv)
{
    srand(time(0));
    println("s: escapology");
    string str;
    getline(cin, str);
    if (str.empty()) solve_puz_escapology();
    else if (str == "s") solve_puz_escapology();

    return 0;
}
