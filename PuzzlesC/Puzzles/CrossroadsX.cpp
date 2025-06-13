#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Crossroads X

    Summary
    Cross at Ten

    Description
    1. Place a number in each region from 0 to 9.
    2. When four regions borders intersect (a spot where four lines meet),
       the sum of those 4 regions must be 10.
    3. No two orthogonally adjacent regions can have the same number.
*/

namespace puzzles::CrossroadsX{

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

struct puz_numbers : set<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        for (int i = 0; i < num; ++i)
            insert(i + '1');
    }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_size_of_tatami;
    int m_tatami_count;
    vector<vector<Position>> m_area_pos;
    puz_numbers m_numbers;
    map<Position, char> m_pos2num;
    map<Position, int> m_pos2tatami;
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
    , m_area_pos(m_sidelen * 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        m_area_pos.emplace_back();
        for (auto& p : smoves) {
            m_pos2tatami[p] = n;
            m_area_pos.back().push_back(p);
            m_area_pos[p.first].push_back(p);
            m_area_pos[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }
    m_size_of_tatami = m_area_pos[m_sidelen * 2].size();
    m_tatami_count = m_area_pos.size() - m_sidelen * 2;
    m_numbers = puz_numbers(m_size_of_tatami);
}

// second.key : all char numbers used to fill a position
// second.value : the number of remaining times that the key char number can be used in the area
struct puz_area : pair<int, map<char, int>>
{
    puz_area() {}
    puz_area(int index, const puz_numbers& numbers, int num_times_appear)
        : pair<int, map<char, int>>(index, map<char, int>()) {
        for (char ch : numbers)
            second.emplace(ch, num_times_appear);
    }
    bool fill_cells(const Position& p, char ch) { return --second.at(ch); }
};

struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int index, int sz, const puz_numbers& numbers, int num_times_appear) {
        for (int i = 0; i < sz; i++)
            emplace_back(index++, numbers, num_times_appear);
    }
};

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, char ch);
    void remove_pair(const Position& p, char ch) {
        auto i = m_pos2nums.find(p);
        if (i != m_pos2nums.end())
            i->second.erase(ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::range::count(*this, PUZ_SPACE);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    puz_group m_grp_tatamis;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
    map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
    , m_grp_tatamis(g.m_sidelen * 2, g.m_tatami_count, g.m_numbers, 1)
    , m_grp_rows(0, g.m_sidelen, g.m_numbers, g.m_sidelen / g.m_size_of_tatami)
    , m_grp_cols(g.m_sidelen, g.m_sidelen, g.m_numbers, g.m_sidelen / g.m_size_of_tatami)
{
    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c)
            m_pos2nums[{r, c}] = g.m_numbers;

    for (auto& [p, ch] : g.m_pos2num)
        make_move(p, ch);
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2nums.erase(p);

    auto areas = {
        &m_grp_tatamis[m_game->m_pos2tatami.at(p)],
        &m_grp_rows[p.first],
        &m_grp_cols[p.second]
    };
    for (puz_area* a : areas)
        if (a->fill_cells(p, ch) == 0)
            for (auto& p2 : m_game->m_area_pos[a->first])
                remove_pair(p2, ch);

    // no touch
    for (auto& os : offset) {
        auto p2 = p + os;
        if (is_valid(p2))
            remove_pair(p2, ch);
    }

    return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, puz_numbers>& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, nums] = *boost::min_element(m_pos2nums, [](
        const pair<const Position, puz_numbers>& kv1,
        const pair<const Position, puz_numbers>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : nums) {
        children.push_back(*this);
        if (!children.back().make_move(p, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CrossroadsX()
{
    using namespace puzzles::CrossroadsX;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/CrossroadsX.xml", "Puzzles/CrossroadsX.txt", solution_format::GOAL_STATE_ONLY);
}
