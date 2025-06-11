#include "stdafx.h"

extern void solve_puz_icedin();
extern void solve_puz_InbetweenNurikabe();
extern void solve_puz_InbetweenSumscrapers();
extern void solve_puz_InsaneTatamis();
extern void solve_puz_IslandConnections();

int main(int argc, char **argv)
{
    srand(time(0));
    println("c: icedin");
    println("n1: Inbetween Nurikabe");
    println("n2: Inbetween Sumscrapers");
    println("n3: Insane Tatamis");
    println("s: Island Connections");
    string str;
    getline(cin, str);
    if (str.empty());
    else if (str == "c") solve_puz_icedin();
    else if (str == "n1") solve_puz_InbetweenNurikabe();
    else if (str == "n2") solve_puz_InbetweenSumscrapers();
    else if (str == "n3") solve_puz_InsaneTatamis();
    else if (str == "s") solve_puz_IslandConnections();

    return 0;
}
