#include "stdafx.h"

extern void solve_puz_Yalooniq();

int main(int argc, char **argv)
{
    srand(time(0));
    println("a: Yalooniq");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a") solve_puz_Yalooniq();

    return 0;
}
