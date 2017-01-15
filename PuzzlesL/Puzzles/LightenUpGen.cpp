#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 2/Lighten Up

    Summary
    Place lightbulbs to light up all the room squares

    Description
    1. What you see from above is a room and the marked squares are walls.
    2. The goal is to put lightbulbs in the room so that all the blank(non-wall)
       squares are lit, following these rules.
    3. Lightbulbs light all free, unblocked squares horizontally and vertically.
    4. A lightbulb can't light another lightbulb.
    5. Walls block light. Also walls with a number tell you how many lightbulbs
       are adjacent to it, horizontally and vertically.
    6. Walls without a number can have any number of lightbulbs. However,
       lightbulbs don't need to be adjacent to a wall.
*/

namespace puzzles{ namespace LightenUp{

#define PUZ_WALL        'W'
#define PUZ_BULB        'B'
#define PUZ_SPACE       ' '
#define PUZ_UNLIT       '.'
#define PUZ_LIT         '+'

const Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
};

struct puz_generator
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_walls;
    string m_start;
    char cells(const Position& p) const { return m_start.at(p.first * m_sidelen + p.second); }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }

    puz_generator(int n);
    Position gen_elem(int n, char ch);
    void gen_walls(int n);
    void gen_lightbulbs();
    string to_string();
};

puz_generator::puz_generator(int n)
    : m_sidelen(n + 2)
{
    m_start.append(m_sidelen, PUZ_WALL);
    for(int r = 1; r < m_sidelen - 1; ++r){
        m_start.push_back(PUZ_WALL);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_WALL);
    }
    m_start.append(m_sidelen, PUZ_WALL);
}

Position puz_generator::gen_elem(int n, char ch_n)
{
    int j = 0;
    for(int i = 0; i < m_start.length(); i++){
        char& ch = m_start[i];
        if(ch == PUZ_SPACE && j++ == n){
            ch = ch_n;
            return {i / m_sidelen, i % m_sidelen};
        }
    }
    return {-1, -1};
}

void puz_generator::gen_walls(int n)
{
    for(int i = 0; i < n; i++){
        int m = rand() % boost::count(m_start, PUZ_SPACE);
        auto p = gen_elem(m, PUZ_WALL);
        m_walls[p] = 0;
    }
}

void puz_generator::gen_lightbulbs()
{
    for(;;){
        int n = boost::count(m_start, PUZ_SPACE);
        if(n == 0) return;
        int m = rand() % n;
        auto p = gen_elem(m, PUZ_BULB);
        for(auto& os : offset){
            [&]{
                auto p2 = p + os;
                if(cells(p2) == PUZ_WALL)
                    m_walls[p2]++;
                for(;; p2 += os)
                    switch(char& ch2 = cells(p2)){
                    case PUZ_WALL:
                        return;
                    case PUZ_SPACE:
                        ch2 = PUZ_LIT;
                        break;
                    }
            }();
        }
    }
}

string puz_generator::to_string()
{
    stringstream ss;
    for(int r = 1; r < m_sidelen - 1; r++){
        for(int c = 1; c < m_sidelen - 1; c++){
            Position p(r, c);
            char ch = cells(p);
            ss << (ch == PUZ_WALL ? char(m_walls[p] + '0') : PUZ_SPACE);
        }
        ss << '\\' << endl;
    }
    return ss.str();
}

}}

void gen_puz_LightenUp()
{
    using namespace puzzles::LightenUp;
    srand(time(NULL));
    puz_generator g(6);
    g.gen_walls(12);
    g.gen_lightbulbs();
    auto s = g.to_string();
    cout << s;
}
