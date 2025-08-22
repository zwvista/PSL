#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 2/Lighten Up

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

namespace puzzles::LightenUp{

constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_BULB = 'B';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_UNLIT = '.';
constexpr auto PUZ_LIT = '+';
constexpr auto PUZ_NONHINT = 'w';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_generator
{
    int m_sidelen;
    map<Position, int> m_pos2hint;
    string m_cells;
    char cells(const Position& p) const { return m_cells.at(p.first * m_sidelen + p.second); }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }

    puz_generator(int n);
    Position gen_elem(int n, char ch_old, char ch_new);
    void gen_walls(int n);
    void gen_lightbulbs();
    void gen_nonhint(int n);
    string to_string();
};

puz_generator::puz_generator(int n)
    : m_sidelen(n + 2)
{
    m_cells.append(m_sidelen, PUZ_WALL);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        m_cells.push_back(PUZ_WALL);
        m_cells.append(m_sidelen - 2, PUZ_SPACE);
        m_cells.push_back(PUZ_WALL);
    }
    m_cells.append(m_sidelen, PUZ_WALL);
}

Position puz_generator::gen_elem(int n, char ch_old, char ch_new)
{
    int j = 0;
    for (int r = 1; r < m_sidelen - 1; ++r)
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char& ch = cells(Position(r, c));
            if (ch == ch_old && j++ == n) {
                ch = ch_new;
                return {r, c};
            }
        }
    return {-1, -1};
}

void puz_generator::gen_walls(int n)
{
    for (int i = 0; i < n; i++) {
        int m = rand() % boost::count(m_cells, PUZ_SPACE);
        auto p = gen_elem(m, PUZ_SPACE, PUZ_WALL);
        m_pos2hint[p] = 0;
    }
}

void puz_generator::gen_lightbulbs()
{
    for (;;) {
        int n = boost::count(m_cells, PUZ_SPACE);
        if (n == 0) return;
        int m = rand() % n;
        auto p = gen_elem(m, PUZ_SPACE, PUZ_BULB);
        for (auto& os : offset) {
            [&]{
                auto p2 = p + os;
                if (cells(p2) == PUZ_WALL && m_pos2hint.contains(p2))
                    m_pos2hint[p2]++;
                for (;; p2 += os)
                    switch(char& ch2 = cells(p2)) {
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

void puz_generator::gen_nonhint(int n)
{
    for (int i = 0; i < n; i++) {
        int m = rand() % (m_pos2hint.size() - i);
        gen_elem(m, PUZ_WALL, PUZ_NONHINT);
    }
}

string puz_generator::to_string()
{
    stringstream ss;
    ss << endl;
    for (int r = 1; r < m_sidelen - 1; r++) {
        for (int c = 1; c < m_sidelen - 1; c++) {
            Position p(r, c);
            char ch = cells(p);
            ss << (ch == PUZ_NONHINT ? PUZ_WALL : ch == PUZ_WALL ? char(m_pos2hint[p] + '0') : PUZ_SPACE);
        }
        ss << '\\' << endl;
    }
    return ss.str();
}

}

bool is_valid_LightenUp(const string& s)
{
    extern void solve_puz_LightenUpTest();
    xml_document doc;
    auto levels = doc.append_child("puzzle").append_child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = "test";
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("../Test.xml");
    solve_puz_LightenUpTest();
    ifstream in("../Test.txt");
    string x;
    getline(in, x);
    getline(in, x);
    return x != "Solution 1:";
}

void gen_puz_LightenUp()
{
    using namespace puzzles::LightenUp;

    string s;
    do {
        puz_generator g(10);
        g.gen_walls(20);
        g.gen_lightbulbs();
        g.gen_nonhint(3);
        s = g.to_string();
        print("{}", s);
    } while(!is_valid_LightenUp(s));
}
