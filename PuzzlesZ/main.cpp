#include "stdafx.h"

extern void solve_puz_zafiro();
extern void solve_puz_ZenGardens();
extern void solve_puz_ZenLandscaper();
extern void solve_puz_zgj();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a: zafiro" << endl;
    cout << "e1: Zen Gardens" << endl;
    cout << "e2: Zen Landscaper" << endl;
    cout << "g: zgj" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "a") solve_puz_zafiro();
    else if (str == "e1") solve_puz_ZenGardens();
    else if (str == "e2") solve_puz_ZenLandscaper();
    else if (str == "g") solve_puz_zgj();

    return 0;
}
