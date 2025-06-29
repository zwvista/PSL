#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 14/Mini-Lits

    Summary
    Lits Jr.

    Description
    1. You play the game with triominos (pieces of three squares).
    2. The board is divided into many areas. You have to place a triomino
       into each area respecting these rules:
    3. No two adjacent (touching horizontally / vertically) triominos should
       be of equal shape & orientation.
    4. All the shaded cells should form a valid Nurikabe.
*/

namespace puzzles::MiniLits{

constexpr auto PUZ_SPACE = '.'    ;

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

const vector<vector<Position>> triominos = {
    // L
    {{0, 0}, {0, 1}, {1, 0}},
    {{0, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}},
    {{0, 1}, {1, 0}, {1, 1}},
    // I
    {{0, 0}, {0, 1}, {0, 2}},
    {{0, 0}, {1, 0}, {2, 0}},
};

// first: the position where the triomino will be placed
// second: the triomino id
using puz_piece = pair<Position, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2area;
    int m_area_count;
    // Dimension 1: area id
    // Dimension 2: all possible combinations
    vector<vector<puz_piece>> m_pieces;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2)
{
    set<Position> horz_walls, vert_walls, rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                vert_walls.insert(p);
            if (c == m_sidelen) break;
            rng.insert(p);
        }
    }

    for (m_area_count = 0; !rng.empty(); ++m_area_count) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({horz_walls, vert_walls, *rng.begin()});
        for (auto& p : smoves) {
            m_pos2area[p] = m_area_count;
            rng.erase(p);
        }
    }

    m_pieces.resize(m_area_count);
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < triominos.size(); ++i) {
                auto& t = triominos[i];
                int n = [&]{
                    int n = -1;
                    for (auto& os : t) {
                        auto p2 = p + os;
                        auto it = m_pos2area.find(p2);
                        if (it == m_pos2area.end())
                            // outside the board
                            return -1;
                        if (n == -1)
                            // the area id
                            n = it->second;
                        else if (n != it->second)
                            // must be in the same area
                            return -1;
                    }
                    return n;
                }();
                if (n != -1)
                    m_pieces[n].emplace_back(p, i);
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    bool is_interconnected() const;
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
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < g.m_area_count; ++i) {
        auto& perm_ids = m_matches[i];
        perm_ids.resize(g.m_pieces[i].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, piece_ids] : m_matches) {
        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& piece = m_game->m_pieces[area_id][id];
            auto& t = triominos[piece.second];
            char ch = piece.second + '0';
            for (auto& os : t) {
                auto p2 = piece.first + os;
                if (cells(p2) != PUZ_SPACE)
                    return true;
                // No two adjacent (touching horizontally / vertically) triominos should
                // be of equal shape & orientation.
                for (auto& os2 : offset) {
                    auto p3 = p2 + os2;
                    if (is_valid(p3) && ch == cells(p3))
                        return true;
                }
            }
            return false;
        });

        if (!init)
            switch(piece_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, piece_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& a) : m_area(&a) { make_move(*a.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_area;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->contains(p)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// All the shaded cells should form a valid Nurikabe.
bool puz_state::is_interconnected() const
{
    set<Position> area;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SPACE)
                area.insert(p);
        }

    auto smoves = puz_move_generator<puz_state3>::gen_moves(area);
    return smoves.size() == area.size();
}

bool puz_state::make_move2(int i, int j)
{
    auto& piece = m_game->m_pieces[i][j];
    auto& t = triominos[piece.second];

    for (int k = 0; k < t.size(); ++k)
        cells(piece.first + t[k]) = piece.second + '0';

    ++m_distance;
    m_matches.erase(i);

    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            vector<Position> rng = {{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
            if (boost::algorithm::all_of(rng, [&](const Position& p) {
                return cells(p) != PUZ_SPACE;
            }))
                return false;
        }
    return !is_goal_state() || is_interconnected();
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if (!make_move2(i, j))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [area_id, piece_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : piece_ids) {
        children.push_back(*this);
        if (!children.back().make_move(area_id, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_MiniLits()
{
    using namespace puzzles::MiniLits;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MiniLits.xml", "Puzzles/MiniLits.txt", solution_format::GOAL_STATE_ONLY);
}
