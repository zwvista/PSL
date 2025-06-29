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

constexpr Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_walls;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_WALL);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_WALL);
        for (int c = 1; c < m_sidelen - 1; ++c)
            switch(Position p(r, c); char ch = str[c - 1]) {
            case PUZ_SPACE:
            case PUZ_WALL:
                m_cells.push_back(ch);
                break;
            default:
                m_cells.push_back(PUZ_WALL);
                m_walls[p] = ch - '0';
                break;
            }
        m_cells.push_back(PUZ_WALL);
    }
    m_cells.append(m_sidelen, PUZ_WALL);
}

// first: all positions occupied by the area
// second: all permutations of the bulbs
using puz_area = pair<vector<Position>, vector<string>>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_areas) < tie(x.m_cells, x.m_areas);
    }
    bool make_move_space(const Position& p);
    bool make_move_area(int i, const string& perm);
    bool make_move(const Position& p, char ch_p);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count_if(m_cells, [](char ch) {
            return ch == PUZ_SPACE || ch == PUZ_UNLIT;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    vector<puz_area> m_areas;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells)
, m_game(&g)
{
    for (auto& [p, n] : g.m_walls) {
        vector<Position> rng;
        for (auto& os : offset) {
            auto p2 = p + os;
            if (cells(p2) != PUZ_WALL)
                rng.push_back(p2);
        }

        vector<string> perms;
        string perm;
        for (int i = 0; i < rng.size() - n; ++i)
            perm.push_back(PUZ_UNLIT);
        for (int i = 0; i < n; ++i)
            perm.push_back(PUZ_BULB);
        do
            perms.push_back(perm);
        while (boost::next_permutation(perm));
        m_areas.emplace_back(rng, perms);
    }
}

bool puz_state::make_move_space(const Position& p)
{
    m_distance = 0;
    return make_move(p, PUZ_BULB);
}

bool puz_state::make_move_area(int i, const string& perm)
{
    m_distance = 0;

    const auto& rng = m_areas[i].first;
    for (int j = 0; j < rng.size(); ++j)
        if (!make_move(rng[j], perm[j]))
            return false;

    m_areas.erase(m_areas.begin() + i);
    return true;
}

bool puz_state::make_move(const Position& p, char ch_p)
{
    char& ch = cells(p);
    if (ch_p == PUZ_UNLIT) {
        if (ch == PUZ_BULB)
            return false;
        if (ch == PUZ_SPACE)
            ch = ch_p;
    } else {
        //PUZ_BULB
        if (ch != PUZ_SPACE)
            return ch == ch_p;
        ch = ch_p;
        ++m_distance;
        for (auto& os : offset)
            if (![&]{
                for (auto p2 = p + os; ; p2 += os)
                    switch(char& ch2 = cells(p2)) {
                    case PUZ_BULB:
                        return false;
                    case PUZ_WALL:
                        return true;
                    case PUZ_UNLIT:
                    case PUZ_SPACE:
                        ch2 = PUZ_LIT;
                        ++m_distance;
                        break;
                    }
            }())
                return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_areas.empty()) {
        vector<Position> rng;
        for (int r = 1; r < sidelen() - 1; ++r)
            for (int c = 1; c < sidelen() - 1; ++c) {
                Position p(r, c);
                if (cells(p) == PUZ_SPACE)
                    rng.push_back(p);
            }
        for (const Position& p : rng)
            if (children.push_back(*this); !children.back().make_move_space(p))
                children.pop_back();
    } else {
        int i = boost::min_element(m_areas, [](const puz_area& a1, const puz_area& a2) {
            return a1.second.size() < a2.second.size();
        }) - m_areas.begin();
        for (auto& perm : m_areas[i].second)
            if (children.push_back(*this); !children.back().make_move_area(i, perm))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            auto i = m_game->m_walls.find(p);
            out << (i == m_game->m_walls.end() ? cells(p) : char(i->second + '0')) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_LightenUp()
{
    using namespace puzzles::LightenUp;
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
    //    "Puzzles/LightenUp.xml", "Puzzles/LightenUp.txt", solution_format::GOAL_STATE_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, false, true>>(
        "Puzzles/LightenUp2.xml", "Puzzles/LightenUp2.txt", solution_format::GOAL_STATE_ONLY);
}

void solve_puz_LightenUpTest()
{
    using namespace puzzles::LightenUp;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, false, true>>(
        "../Test.xml", "../Test.txt", solution_format::GOAL_STATE_ONLY);
}
