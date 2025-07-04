#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Snake Maze

    Summary
    Find the snakes using the given hints.

    Description
    1. A Snake is a path of five tiles, numbered 1-2-3-4-5, where 1 is the head and 5 the tail.
       The snake's body segments are connected horizontally or vertically.
    2. A snake cannot see another snake or it would attack it. A snake sees straight in the
       direction 2-1, that is to say it sees in front of the number 1.
    3. A snake cannot touch another snake horizontally or vertically.
    4. Arrows show you the closest piece of Snake in that direction (before another arrow or the edge).
    5. Arrows with zero mean that there is no Snake in that direction.
    6. arrows block snake sight and also block other arrows hints.
*/

namespace puzzles::SnakeMaze{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_BLOCK = 'O';
constexpr auto PUZ_HINT = 'H';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const string_view dirs = "^>v<";

using puz_hint = pair<int, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, puz_hint> m_pos2hint;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p)
        : m_game(&game), m_p(p) { make_move(p); }

    void make_move(const Position& p) { push_back(m_p = p); }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game = nullptr;
    Position m_p;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BLOCK);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            switch (Position p(r, c); char ch = str[c * 2 - 2]) {
            case PUZ_SPACE:
            case PUZ_BLOCK:
                m_cells.push_back(ch);
                break;
            default:
                m_pos2hint[p] = {ch - '0', dirs.find(str[c * 2 - 1])};
            }
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

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
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, area_ids] : m_matches) {
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
            switch(area_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    //auto& [rng, names, rng2D] = m_game->m_areas[n];
    //for (int i = 0; i < names.size(); ++i) {
    //    auto& rng2 = rng2D[i];
    //    char ch2 = names[i];
    //    for (auto& p2 : rng2)
    //        cells(p2) = ch2, ++m_distance, m_matches.erase(p2);
    //}
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, area_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : area_ids)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    return out;
}

}

void solve_puz_SnakeMaze()
{
    using namespace puzzles::SnakeMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SnakeMaze.xml", "Puzzles/SnakeMaze.txt", solution_format::GOAL_STATE_ONLY);
}
