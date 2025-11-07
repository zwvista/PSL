#include "stdafx.h"

extern void solve_puz_Galaxies();
extern void solve_puz_Gardener();
extern void solve_puz_GardenTunnels();
extern void solve_puz_Gems();
extern void solve_puz_Guesstris();

int main(int argc, char **argv)
{
    srand(time(0));
    for(string str; str.empty();) {
        println("a1: Galaxies");
        println("a2: Gardener");
        println("a3: Garden Tunnels");
        println("e: Gems");
        println("u: Guesstris");
        getline(cin, str);
        if (str.empty()) solve_puz_Guesstris();
        else if (str == "a1") solve_puz_Galaxies();
        else if (str == "a2") solve_puz_Gardener();
        else if (str == "a3") solve_puz_GardenTunnels();
        else if (str == "e") solve_puz_Gems();
        else if (str == "u") solve_puz_Guesstris();
    }
    return 0;
}
