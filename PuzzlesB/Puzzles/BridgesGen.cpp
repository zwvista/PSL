#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

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

namespace puzzles{ namespace Bridges{

#define PUZ_SPACE             ' '
#define PUZ_NUMBER            'N'
#define PUZ_HORZ_1            '-'
#define PUZ_VERT_1            '|'
#define PUZ_HORZ_2            '='
#define PUZ_VERT_2            'H'
#define PUZ_BOUNDARY          'B'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_generator
{
    int m_sidelen;
    map<Position, vector<int>> m_pos2bridges;
    vector<Position> m_islands;
    string m_start;

    puz_generator(int n);
    void gen_bridge();
};

puz_generator::puz_generator(int n)
: m_sidelen(n + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(int r = 1; r < m_sidelen - 1; ++r){
        m_start.push_back(PUZ_BOUNDARY);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    int i = rand() % (n * n);
    m_islands.emplace_back(i / n + 1, i % n + 1);
}

}}

void gen_puz_Bridges()
{
    using namespace puzzles::Bridges;
    puz_generator g(5);
}
