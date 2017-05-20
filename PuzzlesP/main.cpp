#include "stdafx.h"

extern void solve_puz_Pairakabe();
extern void solve_puz_Pairakabe2();
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
extern void solve_puz_Planks();
extern void solve_puz_PowerGrid();
extern void solve_puz_ProductSentinels();
extern void solve_puz_PuzzleRetreat();

int main(int argc, char **argv)
{
    srand(time(0));
    cout << "a1: Pairakabe" << endl;
    cout << "a12: Pairakabe2" << endl;
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
    cout << "l: Planks" << endl;
    cout << "o: Power Grid" << endl;
    cout << "r: Product Sentinels" << endl;
    cout << "u: Puzzle Retreat" << endl;
    string str;
    getline(cin, str);
    if(str.empty());
    else if(str == "a1") solve_puz_Pairakabe();
    else if(str == "a12") solve_puz_Pairakabe2();
    else if(str == "a2") solve_puz_ParkingLot();
    else if(str == "a3") solve_puz_ParkLakes();
    else if(str == "a4") solve_puz_Parks();
    else if(str == "a5") solve_puz_Parks2();
    else if(str == "a6") solve_puz_Pata();
    else if(str == "a7") solve_puz_Patchmania();
    else if(str == "a8") solve_puz_pathfind();
    else if(str == "a9") solve_puz_patternpuzzle();
    else if(str == "e") solve_puz_pegsolitary();
    else if(str == "h") solve_puz_Pharaoh();
    else if(str == "l") solve_puz_Planks();
    else if(str == "o") solve_puz_PowerGrid();
    else if(str == "r") solve_puz_ProductSentinels();
    else if(str == "u") solve_puz_PuzzleRetreat();

    return 0;
}