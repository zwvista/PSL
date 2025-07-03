#include "stdafx.h"

extern void solve_puz_mummymaze();
extern void solve_puz_Patchmania();

int main(int argc, char **argv)
{
    srand(time(0));
    string str;
    println("m: mummymaze");
    println("p: Patchmania");
    getline(cin, str);
    if (str.empty()) solve_puz_mummymaze();
    else if (str == "m") solve_puz_mummymaze();
    else if (str == "p") solve_puz_Patchmania();

    return 0;
}
