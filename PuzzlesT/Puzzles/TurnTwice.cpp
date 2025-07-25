#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
#include <boost/math/special_functions/sign.hpp>

/*
    iOS Game: 100 Logic Games/Puzzle Set 15/Turn Twice

    Summary
    Think and Turn Twice (or more)

    Description
    1. In an effort to complicate signposts, you're given the task to have
       signposts reach other by no less than two turns.
    2. In other words, you have to place walls on the board so that a maze of
       signposts is formed. In this maze:
    3. In order to go from one signpost to the other, you have to turn at least
       twice.
    4. Walls can't touch horizontally or vertically.
    5. All the signposts and empty spaces must form an orthogonally continuous
       area.
*/

namespace puzzles::TurnTwice{

using boost::math::sign;

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SIGNPOST = 'S';
constexpr auto PUZ_WALL = 'W';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<Position> m_signposts;
    vector<vector<Position>> m_paths;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_WALL);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_WALL);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_SIGNPOST);
                m_signposts.emplace_back(r, c);
            }
        }
        m_cells.push_back(PUZ_WALL);
    }
    m_cells.append(m_sidelen, PUZ_WALL);

    Position os0;
    int sz = m_signposts.size();
    for (int i = 0; i < sz; ++i) {
        auto p1 = m_signposts[i];
        for (int j = i + 1; j < sz; ++j) {
            auto p2 = m_signposts[j];
            int sz2 = p1.first == p2.first || p1.second == p2.second ? 1 : 2;
            for (int k = 0; k < sz2; ++k) {
                vector<Position> path;
                for (auto p = p1;;) {
                    Position os1(sign(p2.first - p.first), 0);
                    Position os2(0, sign(p2.second - p.second));
                    Position os = k == 0 && os1 != os0 || k == 1 && os2 == os0 ? os1 : os2;
                    p += os;
                    if (p == p2) break;
                    if (cells(p) != PUZ_SPACE) goto next_k;
                    path.push_back(p);
                }
                m_paths.push_back(path);
            next_k:;
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_paths.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    vector<vector<Position>> m_paths;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g) ,m_cells(g.m_cells), m_paths(g.m_paths)
{
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

inline bool is_not_wall(char ch) { return ch != PUZ_WALL; }

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        if (auto p = *this + os; is_not_wall(m_state->cells(p))) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// 5. All the signposts and empty spaces must form an orthogonally continuous area.
bool puz_state::is_interconnected() const
{
    int i = boost::find_if(m_cells, is_not_wall) - m_cells.begin();
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return smoves.size() == boost::count_if(m_cells, is_not_wall);
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_WALL;
    if (!is_interconnected())
        return false;

    int sz = m_paths.size();
    boost::remove_erase_if(m_paths, [&](const vector<Position>& path) {
        return boost::algorithm::any_of_equal(path, p);
    });

    // 4. Walls can't touch horizontally or vertically.
    for (auto& os : offset) {
        auto p2 = p + os;
        if (char& ch = cells(p2); ch == PUZ_SPACE) {
            ch = PUZ_EMPTY;
            for (auto& path : m_paths)
                boost::remove_erase(path, p2);
        }
    }
    boost::remove_erase_if(m_paths, [&](const vector<Position>& path) {
        return path.empty();
    });
    m_distance = sz - m_paths.size();

    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& path = *boost::min_element(m_paths, [](
        const vector<Position>& path1, const vector<Position>& path2) {
        return path1.size() < path2.size();
    });

    for (auto& p : path)
        if (children.push_back(*this); !children.back().make_move(p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            char ch = cells({r, c});
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TurnTwice()
{
    using namespace puzzles::TurnTwice;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TurnTwice.xml", "Puzzles/TurnTwice.txt", solution_format::GOAL_STATE_ONLY);
}
