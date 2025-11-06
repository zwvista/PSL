#include "stdafx.h"

extern void solve_puz_PaintTheNurikabe();
extern void solve_puz_Pairakabe();
extern void solve_puz_ParkingLot();
extern void solve_puz_ParkLakes();
extern void solve_puz_Parks();
extern void solve_puz_Parks2();
extern void solve_puz_Pata();
extern void solve_puz_PathOnTheHills();
extern void solve_puz_PathOnTheMountains();
extern void solve_puz_patternpuzzle();
extern void solve_puz_pegsolitary();
extern void solve_puz_Pharaoh();
extern void solve_puz_Picnic();
extern void solve_puz_Pipemania();
extern void solve_puz_Planets();
extern void solve_puz_Planks();
extern void solve_puz_PleaseComeBack();
extern void solve_puz_PlugItIn();
extern void solve_puz_Pointing();
extern void solve_puz_PondCamping();
extern void solve_puz_PouringWater();
extern void solve_puz_PourWater();
extern void solve_puz_PowerGrid();
extern void solve_puz_ProductSentinels();
extern void solve_puz_PuzzleRetreat();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("ai1: Paint The Nurikabe");
        println("ai2: Pairakabe");
        println("ar1: Parking Lot");
        println("ar2: Park Lakes");
        println("ar3: Parks");
        println("ar4: Parks2");
        println("at1: Pata");
        println("at2: Path on the Hills");
        println("at3: Path on the Mountains");
        println("at4: patternpuzzle");
        println("e: pegsolitary");
        println("h: Pharaoh");
        println("i1: Picnic");
        println("i2: Pipemania");
        println("l1: Planets");
        println("l2: Planks");
        println("l3: Please come back");
        println("l4: Plug it in");
        println("o1: Pointing");
        println("o2: Pond camping");
        println("o3: Pouring Water");
        println("o4: Pour Water");
        println("o5: Power Grid");
        println("r: Product Sentinels");
        println("u: Puzzle Retreat");
        getline(cin, str);
        if (str.empty()) solve_puz_Pointing();
        else if (str == "ai1") solve_puz_PaintTheNurikabe();
        else if (str == "ai2") solve_puz_Pairakabe();
        else if (str == "ar1") solve_puz_ParkingLot();
        else if (str == "ar2") solve_puz_ParkLakes();
        else if (str == "ar3") solve_puz_Parks();
        else if (str == "ar4") solve_puz_Parks2();
        else if (str == "at1") solve_puz_Pata();
        else if (str == "at2") solve_puz_PathOnTheHills();
        else if (str == "at3") solve_puz_PathOnTheMountains();
        else if (str == "at4") solve_puz_patternpuzzle();
        else if (str == "e") solve_puz_pegsolitary();
        else if (str == "h") solve_puz_Pharaoh();
        else if (str == "i1") solve_puz_Picnic();
        else if (str == "i2") solve_puz_Pipemania();
        else if (str == "l1") solve_puz_Planets();
        else if (str == "l2") solve_puz_Planks();
        else if (str == "l3") solve_puz_PleaseComeBack();
        else if (str == "l4") solve_puz_PlugItIn();
        else if (str == "o1") solve_puz_Pointing();
        else if (str == "o2") solve_puz_PondCamping();
        else if (str == "o3") solve_puz_PouringWater();
        else if (str == "o4") solve_puz_PourWater();
        else if (str == "o5") solve_puz_PowerGrid();
        else if (str == "r") solve_puz_ProductSentinels();
        else if (str == "u") solve_puz_PuzzleRetreat();
    }
    return 0;
}
