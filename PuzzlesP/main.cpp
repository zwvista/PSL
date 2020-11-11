#include "stdafx.h"

extern void solve_puz_PaintTheNurikabe();
extern void solve_puz_Pairakabe();
extern void solve_puz_ParkingLot();
extern void solve_puz_ParkLakes();
extern void solve_puz_Parks();
extern void solve_puz_Parks2();
extern void solve_puz_Pata();
extern void solve_puz_Patchmania();
extern void solve_puz_pathfind();
extern void solve_puz_patternpuzzle();
extern void solve_puz_pegsolitary();
extern void solve_puz_Pharaoh();
extern void solve_puz_Picnic();
extern void solve_puz_Pipemania();
extern void solve_puz_Planets();
extern void solve_puz_Planks();
extern void solve_puz_PlugItIn();
extern void solve_puz_PouringWater();
extern void solve_puz_PourWater();
extern void solve_puz_PowerGrid();
extern void solve_puz_ProductSentinels();
extern void solve_puz_PuzzleRetreat();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "ai1: Paint The Nurikabe" << endl;
    cout << "ai2: Pairakabe" << endl;
    cout << "a2: Parking Lot" << endl;
    cout << "a3: Park Lakes" << endl;
    cout << "a4: Parks" << endl;
    cout << "a5: Parks2" << endl;
    cout << "a6: Pata" << endl;
    cout << "a7: Patchmania" << endl;
    cout << "a8: pathfind" << endl;
    cout << "a9: patternpuzzle" << endl;
    cout << "e: pegsolitary" << endl;
    cout << "h: Pharaoh" << endl;
    cout << "i1: Picnic" << endl;
    cout << "i2: Pipemania" << endl;
    cout << "l1: Planets" << endl;
    cout << "l2: Planks" << endl;
    cout << "l3: Plug it in" << endl;
    cout << "o1: Pouring Water" << endl;
    cout << "o2: Pour Water" << endl;
    cout << "o3: Power Grid" << endl;
    cout << "r: Product Sentinels" << endl;
    cout << "u: Puzzle Retreat" << endl;
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "ai1") solve_puz_PaintTheNurikabe();
    else if (str == "ai2") solve_puz_Pairakabe();
    else if (str == "a2") solve_puz_ParkingLot();
    else if (str == "a3") solve_puz_ParkLakes();
    else if (str == "a4") solve_puz_Parks();
    else if (str == "a5") solve_puz_Parks2();
    else if (str == "a6") solve_puz_Pata();
    else if (str == "a7") solve_puz_Patchmania();
    else if (str == "a8") solve_puz_pathfind();
    else if (str == "a9") solve_puz_patternpuzzle();
    else if (str == "e") solve_puz_pegsolitary();
    else if (str == "h") solve_puz_Pharaoh();
    else if (str == "i1") solve_puz_Picnic();
    else if (str == "i2") solve_puz_Pipemania();
    else if (str == "l1") solve_puz_Planets();
    else if (str == "l2") solve_puz_Planks();
    else if (str == "l3") solve_puz_PlugItIn();
    else if (str == "o1") solve_puz_PouringWater();
    else if (str == "o2") solve_puz_PourWater();
    else if (str == "o3") solve_puz_PowerGrid();
    else if (str == "r") solve_puz_ProductSentinels();
    else if (str == "u") solve_puz_PuzzleRetreat();

    return 0;
}
