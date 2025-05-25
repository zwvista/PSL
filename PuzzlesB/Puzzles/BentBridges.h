//
//  BentBridges.hpp
//  PuzzlesB
//
//  Created by 趙偉 on 2020/10/12.
//  Copyright © 2020年 趙偉. All rights reserved.
//

#ifndef BentBridges_h
#define BentBridges_h

/*
    iOS Game: Logic Games 3/Puzzle Set 1/BentBridges

    Summary
    One turn at most

    Description
    1. Connect all the islands together with bridges.
    2. You should be able to go from any island to any other island in the end.
    3. The number on the island tells you how many bridges connect to that island.
    4. A bridge can turn once by 90 degrees between islands.
    5. Bridges cannot cross each other.
*/

namespace puzzles::BentBridges{

#define PUZ_SPACE             ' '
#define PUZ_ISLAND            'N'
#define PUZ_BRIDGE            '.'
#define PUZ_HORZ              '-'
#define PUZ_VERT              '|'

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

}

extern void solve_puz_BentBridges();

#endif /* BentBridges_h */
