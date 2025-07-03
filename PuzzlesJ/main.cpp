#include "stdafx.h"

extern void solve_puz_JoinMe();

int main(int argc, char **argv)
{
    srand(time(0));
    println("o: Join Me!");
    string str;
    getline(cin, str);
    if (str.empty()) solve_puz_JoinMe();
    else if (str == "o") solve_puz_JoinMe();

    return 0;
}
