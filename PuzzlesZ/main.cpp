#include "stdafx.h"

extern void solve_puz_zafiro();
extern void solve_puz_ZenLandscaper();
extern void solve_puz_zgj();

int main(int argc, char **argv)
{
    cout << "a: zafiro" << endl;
    cout << "e: Zen Landscaper" << endl;
    cout << "g: zgj" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "a") solve_puz_zafiro();
    else if(str == "e") solve_puz_ZenLandscaper();
    else if(str == "g") solve_puz_zgj();

    return 0;
}