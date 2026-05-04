#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 6/Hedge Maze

    Summary
    Maze Curator

    Description
    1. Fill some of the empty areas with hedges, thus forming a maze.
    2. The maze should be one tile wide. It can branch itself, but not close in a loop.
    3. There should be a path between the two gates. This path should pass on
       all the steps and not on any fountain.
    4. On the board there can't be a 2x2 area all made of hedges or all without hedges (empty).
    5. Tiles with any icon count as empty and cannot be filled with hedges.
*/

namespace puzzles::HedgeMaze{

/*
    Facts
*/

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_HEDGE = 'H';
constexpr auto PUZ_GATE = 'G';
constexpr auto PUZ_STEP = 'S';
constexpr auto PUZ_FOUNTAIN = 'F';

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::WallsOffset4;
constexpr array<Position, 4> offset3 = Position::Square2x2Offset;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    vector<Position> m_gates;
    set<int> m_iconless_areas;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const puz_game* game, const Position& p_start)
        : m_game(game) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
        if (!walls.contains(p_wall))
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            m_cells.push_back(ch);
            if (ch == PUZ_GATE)
                m_gates.push_back(p);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        m_areas.emplace_back(smoves.begin(), smoves.end());
        bool iconless = true;
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (cells(p) != PUZ_SPACE)
                iconless = false;
        }
        if (iconless)
            m_iconless_areas.insert(n);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, int n);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_iconless_areas.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    set<int> m_iconless_areas;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_iconless_areas(g.m_iconless_areas)
{
}

// 4. From any location, there must only be one route to the treasure.
bool puz_state::is_interconnected() const
{
    return true;
}

bool puz_state::make_move(int i, int n)
{
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
//    int i = boost::min_element(m_cells, [](const puz_cell& cl1, const puz_cell& cl2) {
//        auto f = [](const puz_cell& cl) {
//            int sz = cl.size();
//            return sz == 1 ? 100 : sz;
//        };
//        return f(cl1) < f(cl2);
//    }) - m_cells.begin();
//    auto& cl = m_cells[i];
//    for (int n : cl)
//        if (!children.emplace_back(*this).make_move(i, n))
//            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
//    for (int r = 0; r < sidelen(); ++r) {
//        for (int c = 0; c < sidelen(); ++c)
//            switch (int n = cells({r, c})[0]) {
//            case PUZ_WALL:
//                out << PUZ_WALL_CHAR << ' ';
//                break;
//            case PUZ_TRESURE:
//                out << PUZ_TRESURE_CHAR << ' ';
//                break;
//            default:
//                out << dirs[n] << ' ';
//                break;
//            }
//        println(out);
//    }
    return out;
}

}

void solve_puz_HedgeMaze()
{
    using namespace puzzles::HedgeMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HedgeMaze.xml", "Puzzles/HedgeMaze.txt", solution_format::GOAL_STATE_ONLY);
}
