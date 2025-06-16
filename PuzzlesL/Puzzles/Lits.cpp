#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 12/Lits

    Summary
    Tetris without the fat guy

    Description
    1. You play the game with all the Tetris pieces, except the square one.
    2. So in other words you use pieces of four squares (tetrominoes) in the
       shape of L, I, T and S, which can also be rotated or reflected (mirrored).
    3. The board is divided into many areas. You have to place a tetromino
       into each area respecting these rules:
    4. No two adjacent (touching horizontally / vertically) tetromino should
       be of equal shape, even counting rotations or reflections.
    5. All the shaded cells should form a valid Nurikabe (hence no fat guy).
*/

namespace puzzles::Lits{

constexpr auto PUZ_SPACE = '.'    ;

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

const vector<vector<vector<Position>>> tetrominoes = {
    { // L
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
        {{0, 1}, {1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, 1}, {0, 2}, {1, 0}},
        {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 2}, {1, 0}, {1, 1}, {1, 2}},
    },
    { // I
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
        {{0, 0}, {0, 1}, {0, 2}, {0, 3}},
    },
    { // T
        {{0, 0}, {0, 1}, {0, 2}, {1, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {2, 0}},
    },
    { // S
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
        {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
    },
};

struct puz_lit
{
    Position m_p;
    int m_index1, m_index2;
    puz_lit(const Position& p, int i, int j)
        : m_p(p), m_index1(i), m_index2(j) {}
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2area;
    int m_area_count;
    vector<vector<puz_lit>> m_lits;

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
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
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

    m_lits.resize(m_area_count);
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < tetrominoes.size(); ++i) {
                auto& t1 = tetrominoes[i];
                for (int j = 0; j < t1.size(); ++j) {
                    auto& t2 = t1[j];
                    int n = [&]{
                        int n = -1;
                        for (auto& os : t2) {
                            auto p2 = p + os;
                            auto it = m_pos2area.find(p2);
                            if (it == m_pos2area.end())
                                return -1;
                            if (n == -1)
                                n = it->second;
                            else if (n != it->second)
                                return -1;
                        }
                        return n;
                    }();
                    if (n != -1)
                        m_lits[n].emplace_back(p, i, j);
                }
            }
        }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    bool is_continuous() const;
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < g.m_area_count; ++i) {
        auto& perm_ids = m_matches[i];
        perm_ids.resize(g.m_lits[i].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, lit_ids] : m_matches) {
        boost::remove_erase_if(lit_ids, [&](int id) {
            auto& lit = m_game->m_lits[area_id][id];
            auto& t = tetrominoes[lit.m_index1][lit.m_index2];
            char ch = lit.m_index1 + '0';
            for (auto& os : t) {
                auto p2 = lit.m_p + os;
                if (cells(p2) != PUZ_SPACE)
                    return true;
                for (auto& os2 : offset) {
                    auto p3 = p2 + os2;
                    if (is_valid(p3) && ch == cells(p3))
                        return true;
                }
            }
            return false;
        });

        if (!init)
            switch(lit_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, lit_ids.front()) ? 1 : 0;
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

// 5. All the shaded cells should form a valid Nurikabe.
bool puz_state::is_continuous() const
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
    auto& lit = m_game->m_lits[i][j];
    auto& t = tetrominoes[lit.m_index1][lit.m_index2];

    for (int k = 0; k < t.size(); ++k)
        cells(lit.m_p + t[k]) = lit.m_index1 + '0';

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
    return !is_goal_state() || is_continuous();
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
    auto& [area_id, lit_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : lit_ids) {
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

void solve_puz_Lits()
{
    using namespace puzzles::Lits;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Lits.xml", "Puzzles/Lits.txt", solution_format::GOAL_STATE_ONLY);
}
