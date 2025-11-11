#include "stdafx.h"

extern void solve_puz_Yalooniq();
extern void solve_puz_YouTurnMeOn();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("a: Yalooniq");
        println("o: You Turn me on");
        getline(cin, str);
        if (str.empty()) solve_puz_YouTurnMeOn();
        else if (str == "a") solve_puz_Yalooniq();
        else if (str == "o") solve_puz_YouTurnMeOn();
    }
    return 0;
}
