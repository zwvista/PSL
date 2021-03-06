#include "stdafx.h"

extern void solve_puz_Neighbours();
extern void solve_puz_NorthPoleFishing();
extern void solve_puz_NoughtsAndCrosses();
extern void solve_puz_NumberCrossing();
extern void solve_puz_NumberCrosswords();
extern void solve_puz_NumberPath();
extern void solve_puz_NumberPath2();
extern void solve_puz_NumberLink();
extern void solve_puz_numeric_paranoia();
extern void solve_puz_Nurikabe();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "e: Neighbours" << endl;
    cout << "o1: North Pole Fishing" << endl;
    cout << "o2: Noughts & Crosses" << endl;
    cout << "u1: Number Crossing" << endl;
    cout << "u2: Number Crosswords" << endl;
    cout << "u3: Number Path" << endl;
    cout << "u32: Number Path 2" << endl;
    cout << "u4: NumberLink" << endl;
    cout << "u5: numeric_paranoia" << endl;
    cout << "u6: Nurikabe" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "e") solve_puz_Neighbours();
    else if (str == "o1") solve_puz_NorthPoleFishing();
    else if (str == "o2") solve_puz_NoughtsAndCrosses();
    else if (str == "u1") solve_puz_NumberCrossing();
    else if (str == "u2") solve_puz_NumberCrosswords();
    else if (str == "u3") solve_puz_NumberPath();
    else if (str == "u32") solve_puz_NumberPath2();
    else if (str == "u4") solve_puz_NumberLink();
    else if (str == "u5") solve_puz_numeric_paranoia();
    else if (str == "u6") solve_puz_Nurikabe();

    return 0;
}
