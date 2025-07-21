#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 1/Digit Worms

    Summary
    Or a hand of worms

    Description
    1. Fill each area with numbers from 1 to the area size, putting them like
       a snake, or worm, in succession.
    2. No number must be orthogonally or diagonally touching the same number
       from another area.
*/

namespace puzzles::DigitWorms{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_moves
{
    vector<char> m_nums;
    vector<vector<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    vector<set<Position>> m_areas;
    map<Position, int> m_pos2area;
    vector<puz_moves> m_area2moves;
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

struct puz_state3 : vector<Position>
{
    puz_state3(const puz_game* game, const set<Position>& area, const Position& p)
        : m_game(game), m_area(&area) {make_move(p);}

    bool is_goal_state() const { return size() == m_area->size(); }
    void make_move(const Position& p) { push_back(p); }
    void gen_children(list<puz_state3>& children) const;
    unsigned int get_distance(const puz_state3& child) const { return 1; }

    const puz_game* m_game;
    const set<Position>* m_area;
};

void puz_state3::gen_children(list<puz_state3>& children) const {
    for (int i = 0; i < 4; ++i) {
        auto p2 = back() + offset[i * 2];
        if (boost::algorithm::none_of_equal(*this, p2) && m_area->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
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
            m_cells.push_back(str_v[c * 2 + 1]);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& area = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            area.insert(p);
            rng.erase(p);
        }
    }

    for (auto& area : m_areas) {
        auto& [nums, perms] = m_area2moves.emplace_back();
        nums.resize(area.size());
        boost::iota(nums, '1');
        for (auto& p : area) {
            puz_state3 sstart(this, area, p);
            list<list<puz_state3>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state3, false, false>::find_solution(sstart, spaths); found)
                perms.push_back(spaths.back().back());
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
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_area2moves.size(); ++i) {
        auto& v = m_matches[i];
        v.resize(g.m_area2moves[i].m_perms.size());
        boost::iota(v, 0);
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& [nums, perms] = m_game->m_area2moves[area_id];
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(perms[id], nums, [&](const Position& p, char ch2) {
                // 2. No number must be orthogonally or diagonally touching the same number
                // from another area.
                if (boost::algorithm::none_of(offset, [&](const Position& os) {
                    auto p2 = p + os;
                    return is_valid(p2) && cells(p2) == ch2;
                }))
                    return true;
                char ch1 = cells(p);
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& [nums, perms] = m_game->m_area2moves[i];
    auto& perm = perms[j];
    for (int k = 0; k < perm.size(); ++k)
        cells(perm[k]) = nums[k];

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

void solve_puz_DigitWorms()
{
    using namespace puzzles::DigitWorms;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DigitWorms.xml", "Puzzles/DigitWorms.txt", solution_format::GOAL_STATE_ONLY);
}
