#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 2/FlowerBeds

    Summary
    Reverse Gardener

    Description
    1. The board represents a garden where flowers are scattered around.
    2. Your task as a gardener is to divide the garden in rectangular (or square)
       flower beds.
    3. Each flower bed should contain exactly one flower.
    4. Contiguous flower beds can't have the same area extension.
    5. Green squares are hedges that can't be included in flower beds.
*/

namespace puzzles::FlowerBeds{

#define PUZ_SPACE        ' '
#define PUZ_FLOWER       'F'
#define PUZ_HEDGE        'H'

const Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const Position offset2[] = {
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
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (int h = 1; h <= m_sidelen - r; ++h)
                for (int w = 1; w <= m_sidelen - c; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r, c), br = tl + box_sz;
                    vector<Position> rng;
                    for (int r2 = tl.first; r2 <= br.first; ++r2)
                        for (int c2 = tl.second; c2 <= br.second; ++c2)
                            if (Position p(r2, c2); cells(p) == PUZ_SPACE)
                                rng.push_back(p);
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    set<Position> m_horz_walls, m_vert_walls;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);
    for (int i = 0; i < g.m_areas.size(); ++i)
        m_matches[i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& area = m_game->m_areas[kv.first];
        auto& perm_ids = kv.second;

        string chars;
        for (auto& p : area)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, m_game->m_perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(kv.second.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(kv.first, kv.second.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    auto& perm = m_game->m_perms[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = area[k];
        char& ch = cells(p);
        ch = perm[k];

        // 4. When two plants are orthogonally adjacent across an area, they must be different.
        for (auto& os : offset) {
            auto p2 = p + os;
            if (is_valid(p2) && m_game->m_pos2area.at(p2) != i && cells(p2) == ch)
                return false;
        }
    }

    ++m_distance;
    m_matches.erase(i);
    return true;
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
            out << (m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_FlowerBeds()
{
    using namespace puzzles::FlowerBeds;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FlowerBeds.xml", "Puzzles/FlowerBeds.txt", solution_format::GOAL_STATE_ONLY);
}
