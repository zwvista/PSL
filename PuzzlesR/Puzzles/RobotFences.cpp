#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 13/Robot Fences

    Summary
    BZZZZliip ...cows?

    Description
    1. A bit like Robot Crosswords, you need to fill each region with a
       randomly ordered sequence of numbers.
    2. Numbers can only be in range 1 to N where N is the board size.
    3. No same number can appear in the same row or column.
*/

namespace puzzles::RobotFences{

constexpr auto PUZ_SPACE = ' ';

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

struct puz_area
{
    vector<Position> m_range;
    string m_nums;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_area> m_areas;
    string m_start;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
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
        m_areas.resize(m_sidelen * 2 + n + 1);
        for (auto& p : smoves) {
            auto areas = {&m_areas[p.first], &m_areas[m_sidelen + p.second], &m_areas.back()};
            char ch = cells(p);
            if (ch == PUZ_SPACE)
                for (auto* a : areas)
                    a->m_range.push_back(p);
            else
                for (auto* a : areas)
                    a->m_nums.push_back(ch);
            rng.erase(p);
        }
    }
    for (int i = 0; i < m_areas.size(); ++i) {
        auto& area = m_areas[i];
        auto& rng = area.m_range;
        auto& nums = area.m_nums;
        int sz = rng.size() + nums.size();
        string nums_all(sz, PUZ_SPACE);
        int n1 = 1, n2 = m_sidelen + 1 - sz;
        if (!nums.empty()) {
            boost::sort(nums);
            if (i >= m_sidelen * 2) {
                n1 = max(n1, nums.back() - '0' - sz + 1);
                n2 = min(n2, nums.front() - '0');
            }
        }
        string perm(rng.size(), PUZ_SPACE);
        auto& perms = area.m_perms;
        for (int j = n1; j <= n2; ++j) {
            boost::iota(nums_all, j + '0');
            boost::set_difference(nums_all, nums, perm.begin());
            do
                perms.push_back(perm);
            while (boost::next_permutation(perm));
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        auto& perm_ids = m_matches[i];
        auto& area = g.m_areas[i];
        perm_ids.resize(area.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}


int puz_state::find_matches(bool init)
{
    for (auto& [n, perm_ids] : m_matches) {
        string nums;
        auto& area = m_game->m_areas[n];
        for (auto& p : area.m_range)
            nums.push_back(cells(p));

        auto& perms = area.m_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(n, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    auto& perm = area.m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area.m_range[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
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

void solve_puz_RobotFences()
{
    using namespace puzzles::RobotFences;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/RobotFences.xml", "Puzzles/RobotFences.txt", solution_format::GOAL_STATE_ONLY);
}
