#include "stdafx.h"

extern void solve_puz_Yalooniq();
extern void solve_puz_YouTurnMeOn();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a: Yalooniq");
    println("o: You Turn me on");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a") solve_puz_Yalooniq();
    else if (str == "o") solve_puz_YouTurnMeOn();

    return 0;
}
