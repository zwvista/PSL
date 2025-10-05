//
//  HiddenPath.hpp
//  PuzzlesB
//
//  Created by 趙偉 on 2017/12/25.
//  Copyright © 2017年 趙偉. All rights reserved.
//

#ifndef HiddenPath_h
#define HiddenPath_h

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/Hidden Path

    Summary
    Jump once on every tile, following the arrows

    Description
    Starting at the tile number 1, reach the last tile by jumping from tile to tile.
    1. When jumping from a tile, you have to follow the direction of the arrow and
       land on a tile in that direction
    2. Although you have to follow the direction of the arrow, you can land on any
       tile in that direction, not just the one next to the current tile.
    3. The goal is to jump on every tile, only once and reach the last tile.
*/

namespace puzzles::HiddenPath{

constexpr auto PUZ_UNKNOWN = 0;

constexpr array<Position, 8> offset = Position::Directions8;

}

extern void solve_puz_HiddenPath();
extern void solve_puz_HiddenPathTest();
extern void gen_puz_HiddenPath();

#endif /* HiddenPath_h */
