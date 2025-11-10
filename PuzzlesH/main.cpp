#include "stdafx.h"

#include "PSLhelper.h"

extern void solve_puz_Hedgehog();
extern void solve_puz_HeliumAndIron();
extern void solve_puz_Hexotris();
extern void solve_puz_hexrotation();
extern void solve_puz_HiddenPath();
extern void gen_puz_HiddenPath();
extern void solve_puz_HiddenClouds();
extern void solve_puz_HiddenStars();
extern void solve_puz_Hidoku();
extern void solve_puz_Hitori();
extern void solve_puz_HolidayIsland();
extern void solve_puz_HolidayIsland2();
extern void solve_puz_HolidayIsland3();
extern void solve_puz_hopover();
extern void solve_puz_hrd();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("e1: Hedgehog");
        println("e2: Helium And Iron");
        println("e3: Hexotris");
        println("e4: hexrotation");
        println("i1: HiddenClouds");
        println("i2: HiddenPath");
        println("i3: HiddenStars");
        println("i4: Hidoku");
        println("i5: Hitori");
        println("o1: Holiday Island");
        println("o12: Holiday Island 2");
        println("o13: Holiday Island 3");
        println("o2: hopover");
        println("r: hrd");
        getline(cin, str);
        if (str.empty()) solve_puz_HeliumAndIron();
        else if (str == "e1") solve_puz_Hedgehog();
        else if (str == "e2") solve_puz_HeliumAndIron();
        else if (str == "e3") solve_puz_Hexotris();
        else if (str == "e4") solve_puz_hexrotation();
        else if (str == "i1") solve_puz_HiddenClouds();
        else if (str == "i2") solve_puz_HiddenPath();
        else if (str == "i2g") gen_puz_HiddenPath();
        else if (str == "i3") solve_puz_HiddenStars();
        else if (str == "i4") solve_puz_Hidoku();
        else if (str == "i5") solve_puz_Hitori();
        else if (str == "o1") solve_puz_HolidayIsland();
        else if (str == "o12") solve_puz_HolidayIsland2();
        else if (str == "o13") solve_puz_HolidayIsland3();
        else if (str == "o2") solve_puz_hopover();
        else if (str == "r") solve_puz_hrd();
    }
    return 0;
}
