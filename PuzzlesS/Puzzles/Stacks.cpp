#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Stacks

    Summary
    Bubbling up

    Description
    1. Fill each area with every number ranging from 1 to the size of the area.
    2. Two orthogonally adjacent numbers must be different. 
    3. In one area, if a number is right above another, the upper one must be
       higher than the lower one. This only applies to numbers on top of each
       other in the same area.
*/

namespace puzzles::Stacks{

constexpr auto PUZ_SPACE = ' ';

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

struct puz_game
{
    string m_id;
    int m_sidelen;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    string m_cells;
    // all permutations
    map<int, vector<string>> m_size2perms;
    map<int, vector<int>> m_area2permids;
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

    const set<Position>* m_horz_walls, * m_vert_walls;
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
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_areas.emplace_back(smoves.begin(), smoves.end());
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }

    for (auto& area : m_areas) {
        int sz = area.size();
        auto& perms = m_size2perms[sz];
        if (!perms.empty()) continue;
        auto perm = string(sz, '0');
        boost::iota(perm, '1');
        do
            perms.push_back(perm);
        while (boost::next_permutation(perm));
    }

    for (int i = 0; i < m_areas.size(); ++i) {
        auto& area = m_areas[i];
        auto& perms = m_size2perms.at(area.size());
        vector<int> perm_ids(perms.size());
        boost::iota(perm_ids, 0);
        boost::remove_erase_if(perm_ids, [&](int j) {
            auto& perm = perms[j];
            for (int k = 0; k < area.size(); ++k) {
                auto& p1 = area[k];
                char ch1 = perm[k];
                for (int m = 0; m < area.size(); ++m) {
                    auto& p2 = area[m];
                    char ch2 = perm[m];
                    if (p1 - p2 == offset[0] && !(ch1 > ch2))
                        return true;
                }
            }
            return false;
        });
        m_area2permids[i] = perm_ids;
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
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
: m_game(&g), m_cells(g.m_cells), m_matches(g.m_area2permids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& area = m_game->m_areas[area_id];
        int area_size = area.size();
        auto& perms = m_game->m_size2perms.at(area.size());

        string chars;
        for (auto& p : area)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // 2. Two orthogonally adjacent numbers must be different.
            for (int k = 0; k < area.size(); ++k) {
                auto& p = area[k];
                auto ch1 = perm[k];
                for (auto& os : offset) {
                    auto p2 = p + os;
                    if (!is_valid(p2)) continue;
                    if (char ch2 = cells(p2); m_game->m_pos2area.at(p2) != area_id && ch1 == ch2)
                        return true;
                }
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_areas[i];
    auto& perm = m_game->m_size2perms.at(range.size())[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(range[k]) = perm[k];

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
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(area_id, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Stacks()
{
    using namespace puzzles::Stacks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Stacks.xml", "Puzzles/Stacks.txt", solution_format::GOAL_STATE_ONLY);
}
