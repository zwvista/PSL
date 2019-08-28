#include "stdafx.h"

extern void solve_puz_icedin();
extern void solve_puz_IslandConnections();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "c: icedin" << endl;
    cout << "s: Island Connections" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "c") solve_puz_icedin();
    else if (str == "s") solve_puz_IslandConnections();

    return 0;
}
