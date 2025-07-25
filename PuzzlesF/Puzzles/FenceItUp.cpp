#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/Fence It Up

    Summary
    Now with Fences

    Description
    1. A simple puzzle where you have to divide the Board into enclosed
       areas by Fences.
    2. Each area must contain one number and the number tells you the length
       of the perimeter of the area.
*/

namespace puzzles::FenceItUp{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SINGLE = '.';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

// first: a char used to represent the area: a, b, c...
// second: the length of the perimeter of the area
using puz_area_info = pair<char, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_area_info> m_pos2info;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    char ch = 'a';
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            auto s = str.substr(c * 2 - 2, 2);
            int n = stoi(string(s));
            if (n > 4)
                m_pos2info[{r, c}] = {ch, n};
            m_cells.push_back(n == 0 ? PUZ_SPACE :
                n == 4 ? PUZ_SINGLE : ch++);
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_area
{
    set<Position> m_inner, m_outer;
    int m_perimeter_len = 4;
    bool m_ready = false;
    bool operator<(const puz_area& x) const {
        return tie(m_ready, m_perimeter_len) <
            tie(x.m_ready, x.m_perimeter_len);
    }
};

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& pnum, const Position& p);
    void make_move2(const Position& pnum, const Position& p);
    int adjust_area(bool init);
    int get_perimeter_len(const puz_area& area, const Position& p) {
        int n = boost::count_if(offset, [&](const Position& os) {
            return area.m_inner.contains(p + os);
        });
        return area.m_perimeter_len - n + (4 - n);
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, puz_area> m_pos2area;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [pnum, info] : g.m_pos2info)
        m_pos2area[pnum].m_inner.insert(pnum);
    adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
    for (auto& [pnum, area] : m_pos2area) {
        auto& info = m_game->m_pos2info.at(pnum);

        auto& outer = area.m_outer;
        if (area.m_ready && outer.empty()) continue;

        outer.clear();
        for (auto& p : area.m_inner)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (cells(p2) == PUZ_SPACE &&
                    get_perimeter_len(area, p2) <= info.second)
                    outer.insert(p2);
            }

        if (!init)
            switch(outer.size()) {
            case 0:
                return area.m_ready ? 1 : 0;
            case 1:
                return make_move2(pnum, *outer.begin()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& pnum, const Position& p)
{
    auto& info = m_game->m_pos2info.at(pnum);
    cells(p) = info.first;

    auto& area = m_pos2area.at(pnum);
    area.m_inner.insert(p);
    area.m_perimeter_len = get_perimeter_len(area, p);
    area.m_ready = area.m_perimeter_len == info.second;

    ++m_distance;
}

bool puz_state::make_move(const Position& pnum, const Position& p)
{
    m_distance = 0;
    make_move2(pnum, p);
    int m;
    while ((m = adjust_area(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [pnum, area] = *boost::min_element(m_pos2area, [](
        const pair<const Position, puz_area>& kv1,
        const pair<const Position, puz_area>& kv2) {
        return kv1.second < kv2.second;
    });
    for (auto& p : area.m_outer)
        if (children.push_back(*this); !children.back().make_move(pnum, p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) == PUZ_SINGLE) {
                for (int i = 0; i < 4; ++i) {
                    auto p_wall = p + offset2[i];
                    auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                    walls.insert(p_wall);
                }
            }
        }
    for (auto& [p, area] : m_pos2area)
        for (auto& p : area.m_inner)
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (!area.m_inner.contains(p2))
                    walls.insert(p_wall);
            }

    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            if (auto it = m_game->m_pos2info.find(p); it == m_game->m_pos2info.end())
                out << (cells(p) == PUZ_SINGLE ? " 4" : " .");
            else
                out << format("{:2}", it->second.second);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FenceItUp()
{
    using namespace puzzles::FenceItUp;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FenceItUp.xml", "Puzzles/FenceItUp.txt", solution_format::GOAL_STATE_ONLY);
}
