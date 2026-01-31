#include "stdafx.h"

extern void solve_puz_Farmer();
extern void solve_puz_FenceItUp();
extern void solve_puz_FenceLits();
extern void solve_puz_FenceSentinels();
extern void solve_puz_FencingSheep();
extern void solve_puz_Fields();
extern void solve_puz_Fill();
extern void solve_puz_Fillomino();
extern void solve_puz_FingerPointing();
extern void solve_puz_fling();
extern void solve_puz_FloorPlan();
extern void solve_puz_FlowerBeds();
extern void solve_puz_FlowerbedShrubs();
extern void solve_puz_FlowerOMino();
extern void solve_puz_FourMeNot();
extern void solve_puz_FreePlanks();
extern void solve_puz_FunnyNumbers();
extern void solve_puz_FussyWaiter();
extern void solve_puz_Futoshiki();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("a: Farmer");
        println("e1: Fence It Up");
        println("e2: FenceLits");
        println("e3: Fence Sentinels");
        println("e4: Fencing Sheep");
        println("i1: Fields");
        println("i2: Fill");
        println("i3: Fillomino");
        println("i4: Finger Pointing");
        println("l1: fling");
        println("l2: Floor Plan");
        println("l3: Flower Beds");
        println("l4: Flowerbed Shrubs");
        println("l5: Flower-O-Mino");
        println("o: Four-Me-Not");
        println("r: Free Planks");
        println("u1: Funny Numbers");
        println("u2: Fussy Waiter");
        println("u3: Futoshiki");
        getline(cin, str);
        if (str.empty()) solve_puz_FlowerbedShrubs();
        else if (str == "a") solve_puz_Farmer();
        else if (str == "e1") solve_puz_FenceItUp();
        else if (str == "e2") solve_puz_FenceLits();
        else if (str == "e3") solve_puz_FenceSentinels();
        else if (str == "e4") solve_puz_FencingSheep();
        else if (str == "i1") solve_puz_Fields();
        else if (str == "i2") solve_puz_Fill();
        else if (str == "i3") solve_puz_Fillomino();
        else if (str == "i4") solve_puz_FingerPointing();
        else if (str == "l1") solve_puz_fling();
        else if (str == "l2") solve_puz_FloorPlan();
        else if (str == "l3") solve_puz_FlowerBeds();
        else if (str == "l4") solve_puz_FlowerbedShrubs();
        else if (str == "l5") solve_puz_FlowerOMino();
        else if (str == "o") solve_puz_FourMeNot();
        else if (str == "r") solve_puz_FreePlanks();
        else if (str == "u1") solve_puz_FunnyNumbers();
        else if (str == "u2") solve_puz_FussyWaiter();
        else if (str == "u3") solve_puz_Futoshiki();
    }
    return 0;
}
