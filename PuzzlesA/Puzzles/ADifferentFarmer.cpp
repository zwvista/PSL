#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 4/A Different Farmer

    Summary
    Not all farmers are created equal

    Description
    1. A Different Farmer plants fruits and vegetables in a different way.
    2. He places exactly one of each of the three fruits or vegetables in each field
       (marked area).
    3. The same plant cannot be placed in adjacent tiles, not even diagonally.
    4. All the plants must be connected horizontally or vertically.
*/

namespace puzzles::ADifferentFarmer{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';

constexpr Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},    // nw
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, int> m_pos2area;
    vector<vector<Position>> m_areas;
    map<int, vector<string>> m_num2perms;
    set<Position> m_horz_walls, m_vert_walls;

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
        auto p = *this + offset[i * 2];
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
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            m_areas.back().push_back(p);
            rng.erase(p);
        }
    }

    for (auto& area : m_areas) {
        int sz = area.size();
        auto& perms = m_num2perms[sz];
        if (!perms.empty()) continue;
        auto perm = string(sz - 3, PUZ_EMPTY) + "ABC";
        do
            perms.push_back(perm);
        while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);
    bool is_continuous() const;

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        vector<int> perm_ids(g.m_num2perms.at(g.m_areas[i].size()).size());
        boost::iota(perm_ids, 0);
        m_matches[i] = perm_ids;
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int i = kv.first;
        auto& area = m_game->m_areas[i];
        auto& perm_ids = kv.second;
        auto& perms = m_game->m_num2perms.at(area.size());

        string chars;
        for (auto& p : area)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int j) {
            auto& perm = perms[j];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // 3. The same plant cannot be placed in adjacent tiles, not even diagonally.
            for (int k = 0; k < perm.size(); ++k) {
                auto& p = area[k];
                char ch = perm[k];
                if (ch == PUZ_EMPTY) continue;
                for (auto& os : offset)
                    if (auto p2 = p + os; is_valid(p2) && m_game->m_pos2area.at(p2) != i && cells(p2) == ch)
                        return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(i, perm_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p2 = *this + offset[i * 2];
        if (m_rng->count(p2) != 0) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_continuous() const
{
    set<Position> a;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch != PUZ_EMPTY)
                a.insert(p);
        }

    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

bool puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    auto& perm = m_game->m_num2perms.at(area.size())[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
    return is_continuous();
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ADifferentFarmer()
{
    using namespace puzzles::ADifferentFarmer;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ADifferentFarmer.xml", "Puzzles/ADifferentFarmer.txt", solution_format::GOAL_STATE_ONLY);
}
