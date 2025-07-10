#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 17/Nooks

    Summary
    ...in a forest

    Description
    1. Fill some tiles with hedges, so that each number (where someone is playing hide and seek)
       finds itself in the nook.
    2. a Nook is a dead end, one tile wide, with a number in it.
    3. a Nook contains a number that shows you how many tiles can be seen in a straight line from
       there, including the tile itself.
    4. The resulting maze should be a single one-tile path connected horizontally or vertically
       where there are no 2x2 areas of the same type (hedge or path).
*/

namespace puzzles::Nooks{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_NOOK = 'N';
constexpr auto PUZ_HEDGE = '=';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_move
{
    int m_dir;
    set<Position> m_hedge, m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    map<Position, vector<puz_move>> m_pos2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_NOOK);
                m_pos2num[p] = ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, num] : m_pos2num) {
        auto& moves = m_pos2moves[p];
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            if (set<Position> hedges, empties; [&] {
                for (int j = 0; j < 4; ++j) {
                    if (j == i) continue;
                    Position p2 = p + offset[j];
                    if (char ch = cells(p2); ch != PUZ_BOUNDARY)
                        hedges.insert(p2);
                }
                int n = 1;
                for (auto p2 = p + os; cells(p2) != PUZ_BOUNDARY && n <= num; p2 += os, ++n)
                    empties.insert(p2);
                return n == num;
            }())
                moves.push_back({i, hedges, empties});
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        //boost::remove_erase_if(area_ids, [&](int id) {
        //    auto& [rng, _2, rng2D] = m_game->m_areas[id];
        //    return !boost::algorithm::all_of(rng2D, [&](const set<Position>& rng2) {
        //        return boost::algorithm::all_of(rng2, [&](const Position& p2) {
        //            return cells(p2) == PUZ_SPACE ||
        //                boost::algorithm::any_of_equal(rng, p2);
        //        });
        //    });
        //});

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids[0]), 1;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

inline bool is_maze(char ch) { return ch != PUZ_HEDGE && ch != PUZ_BOUNDARY; }

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; is_maze(m_state->cells(p2))) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

// 4. The resulting maze should be a single one-tile path connected horizontally or vertically
bool puz_state::is_interconnected() const
{
    int i = m_cells.find(PUZ_NOOK);
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        { this, {i / sidelen(), i % sidelen()} });
    return boost::count_if(smoves, [&](const Position& p) {
        return is_maze(cells(p));
    }) == boost::count_if(m_cells, is_maze);
}

void puz_state::make_move2(const Position& p, int n)
{
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0; r < sidelen(); ++r) {
    //    for (int c = 0; c < sidelen(); ++c)
    //        if (Position p(r, c); !m_game->m_stones.contains(p))
    //            out << ".. ";
    //        else {
    //            int n = boost::find(m_path, p) - m_path.begin() + 1;
    //            out << format("{:2} ", n);
    //        }
    //    println(out);
    //}
    return out;
}

}

void solve_puz_Nooks()
{
    using namespace puzzles::Nooks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Nooks.xml", "Puzzles/Nooks.txt", solution_format::GOAL_STATE_ONLY);
}
