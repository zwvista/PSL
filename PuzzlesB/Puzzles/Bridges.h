//
//  Bridges.hpp
//  PuzzlesB
//
//  Created by 趙偉 on 2017/12/25.
//  Copyright © 2017年 趙偉. All rights reserved.
//

#ifndef Bridges_h
#define Bridges_h

/*
    iOS Game: Logic Games/Puzzle Set 7/Bridges

    Summary
    Enough Sudoku, let's build!

    Description
    1. The board represents a Sea with some islands on it.
    2. You must connect all the islands with Bridges, making sure every
       island is connected to each other with a Bridges path.
    3. The number on each island tells you how many Bridges are touching
       that island.
    4. Bridges can only run horizontally or vertically and can't cross
       each other.
    5. Lastly, you can connect two islands with either one or two Bridges
       (or none, of course)
*/

namespace puzzles::Bridges{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ISLAND = 'N';
constexpr auto PUZ_HORZ_1 = '-';
constexpr auto PUZ_VERT_1 = '|';
constexpr auto PUZ_HORZ_2 = '=';
constexpr auto PUZ_VERT_2 = 'H';
constexpr auto PUZ_BOUNDARY = 'B';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

}

extern void solve_puz_Bridges();
extern void solve_puz_BridgesTest();
extern void gen_puz_Bridges();

#endif /* Bridges_h */
