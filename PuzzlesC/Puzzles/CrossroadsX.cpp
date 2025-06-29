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
constexpr auto PUZ_UNKNOWN = -1;

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

constexpr Position offset3[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

struct puz_area
{
    int m_num = PUZ_UNKNOWN;
    vector<Position> m_rng;
    set<int> m_adjacent_ids;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_sum;
    vector<vector<int>> m_sum_perms;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, int> m_pos2num;
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    vector<vector<int>> m_crossroads;

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
    , m_sum(level.attribute("sum").as_int(10))
{
    for (int n1 = 0; n1 < 10; ++n1)
        for (int n2 = 0; n2 < 10; ++n2)
            for (int n3 = 0; n3 < 10; ++n3)
                for (int n4 = 0; n4 < 10; ++n4)
                    if (n1 + n2 + n3 + n4 == m_sum)
                        m_sum_perms.push_back({n1, n2, n3, n4});

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
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& [num, rng2, ids] = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                num = it->second;
        }
        rng2.append_range(smoves);
    }

    for (int i = 0; i < m_areas.size(); ++i) {
        auto& [num, rng2, ids] = m_areas[i];
        for (auto& p : rng2)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (auto it = m_pos2area.find(p2); it != m_pos2area.end())
                    if (int id = it->second; id != i)
                        ids.insert(id);
            }
    }

    for (int r = 0; r < m_sidelen - 1; ++r)
        for (int c = 0; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            set<int> ids;
            for (auto& os : offset3)
                ids.insert(m_pos2area.at(p + os));
            if (ids.size() == 4)
                m_crossroads.emplace_back(ids.begin(), ids.end());
        }
}

struct puz_state : vector<int>
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    int find_matches(bool init);
    bool make_move_crossroad(int i, int n);
    bool make_move_crossroad2(int i, int n);
    bool make_move_area(int i, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::count(*this, PUZ_UNKNOWN);}
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, set<int>> m_area2nums;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : vector<int>(g.m_sidelen * g.m_sidelen, PUZ_UNKNOWN), m_game(&g)
{
    set<int> nums;
    for (int i = 0; i < 10; ++i)
        nums.insert(i);

    for (int i = 0; i < g.m_areas.size(); ++i)
        m_area2nums[i] = nums;

    for (int i = 0; i < g.m_crossroads.size(); ++i) {
        auto& ids = g.m_crossroads[i];
        auto& v = m_matches[i];
        for (int j = 0; j < g.m_sum_perms.size(); ++j) {
            auto& perm = g.m_sum_perms[j];
            if ([&] {
                for (int k = 0; k < 3; ++k)
                    for (int m = k + 1; m < 4; ++m)
                        if (perm[k] == perm[m] &&
                            g.m_areas[ids[k]].m_adjacent_ids.contains(ids[m]))
                            return false;
                return true;
            }())
                v.push_back(j);
        }
    }

    for (int i = 0; i < g.m_areas.size(); ++i) {
        auto& [num, rng, ids] = g.m_areas[i];
        if (num != PUZ_UNKNOWN)
            make_move_area(i, num);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [i, perm_ids] : m_matches) {
        auto& area_ids = m_game->m_crossroads[i];

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = m_game->m_sum_perms[id];
            for (int i = 0; i < 4; ++i) {
                auto& nums = m_area2nums.at(area_ids[i]);
                if (boost::algorithm::none_of_equal(nums, perm[i]))
                    return true;
            }
            return false;
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_crossroad2(i, perm_ids.front()), 1;
            }
    }
    return 2;
}

bool puz_state::make_move_crossroad2(int i, int n)
{
    auto& ids = m_game->m_crossroads[i];
    auto& perm = m_game->m_sum_perms[n];
    for (int i = 0; i < 4; ++i)
        if (!make_move_area(ids[i], perm[i]))
            return false;
    m_matches.erase(i);
    return true;
}

bool puz_state::make_move_area(int i, int n)
{
    auto& [num, rng, ids] = m_game->m_areas[i];

    for (auto& p : rng) {
        int &n2 = cells(p);
        if (n2 == PUZ_UNKNOWN)
            n2 = n, ++m_distance;
    }
    m_area2nums.at(i) = {n};

    // 3. No two orthogonally adjacent regions can have the same number.
    for (int id : ids)
        if (auto it = m_area2nums.find(id); it != m_area2nums.end())
            it->second.erase(n);

    return boost::algorithm::none_of(m_area2nums, [](const pair<const int, set<int>>& kv) {
        return kv.second.empty();
    });
}

bool puz_state::make_move_crossroad(int i, int n)
{
    m_distance = 0;
    if (!make_move_crossroad2(i, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [i, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const int, vector<int>>& kv1,
            const pair<const int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (auto& n : perm_ids) {
            children.push_back(*this);
            if (!children.back().make_move_crossroad(i, n))
                children.pop_back();
        }
    }
    else {
        auto& [i, nums] = *boost::min_element(m_area2nums, [](
            const pair<const int, set<int>>& kv1,
            const pair<const int, set<int>>& kv2) {
            auto f = [](const pair<const int, set<int>>& kv) {
                int sz = kv.second.size();
                return sz == 1 ? 100 : sz;
            };
            return f(kv1) < f(kv2);
        });
        for (char n : nums) {
            children.push_back(*this);
            if (!children.back().make_move_area(i, n))
                children.pop_back();
        }
    }
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

void solve_puz_CrossroadsX()
{
    using namespace puzzles::CrossroadsX;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/CrossroadsX.xml", "Puzzles/CrossroadsX.txt", solution_format::GOAL_STATE_ONLY);
}
