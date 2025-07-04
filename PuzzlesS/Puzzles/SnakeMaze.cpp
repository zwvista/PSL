#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
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
constexpr auto PUZ_BLOCK = 'B';
constexpr auto PUZ_HINT = 'H';
constexpr auto PUZ_SNAKE_SIZE = 5;

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const string_view dirs = "^>v<";

using puz_hint = pair<int, Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, puz_hint> m_pos2hint;
    vector<vector<Position>> m_snakes;
    map<Position, vector<int>> m_pos2snake_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : map<int, Position>
{
    puz_state2(const puz_game& game, int n, const Position& p, const set<Position>& empties)
        : m_game(&game), m_empties(&empties) { make_move(n, p); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of(*this, [&](const pair<const int, Position>& kv) {
            return kv.second == p;
        });
    }

    bool is_goal_state() const { return size() == PUZ_SNAKE_SIZE; }
    void make_move(int n, const Position& p) { emplace(n, p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    const set<Position>* m_empties;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    auto f = [&](int n, const Position& p) {
        for (int i = 0; i < 4; ++i)
            if (auto p2 = p + offset[i]; 
                m_game->cells(p2) == PUZ_SPACE && !m_empties->contains(p2) && !is_self(p2) &&
                boost::algorithm::all_of(offset, [&](const Position& os) {
                auto p3 = p2 + os;
                return p3 == p || !is_self(p3);
            })) {
                children.push_back(*this);
                children.back().make_move(n, p2);
            }
    };
    auto& [n, p] = *begin();
    if (n > 1)
        f(n - 1, p);
    else {
        auto& [n, p] = *rbegin();
        f(n + 1, p);
    }
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
                m_pos2hint[p] = {ch - '0', offset[dirs.find(str[c * 2 - 1])]};
                m_cells.push_back(PUZ_HINT);
                break;
            }
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

    for (auto& [p, hint] : m_pos2hint) {
        auto& [n, os] = hint;
        vector<Position> rng;
        for (auto p2 = p + os; cells(p2) == PUZ_SPACE; p2 += os)
            rng.push_back(p2);
        if (n == 0)
            for (auto& p2 : rng)
                cells(p2) = PUZ_EMPTY;
        else
            for (auto it = rng.begin(); it != rng.end(); ++it) {
                set<Position> empties(rng.begin(), it);
                puz_state2 sstart(*this, n, *it, empties);
                list<list<puz_state2>> spaths;
                if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                    // save all goal states as permutations
                    // A goal state is a snake formed from the number
                    for (auto& spath : spaths) {
                        int n2 = m_snakes.size();
                        for (auto& v = m_snakes.emplace_back(); auto& [_1, p2] : spath.back())
                            v.push_back(p2);
                        m_pos2snake_ids[p].push_back(n2);
                    }
            }
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
