#include "stdafx.h"

extern void solve_puz_Yalooniq();

int main(int argc, char **argv)
{
    cout << "a: Yalooniq" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "a") solve_puz_Yalooniq();

    return 0;
}