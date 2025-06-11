#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 3/Crosstown Traffic

    Summary
    looks like pipes made of asphalt

    Description
    1. Draw a circuit (looping road)
    2. The road may cross itself, but otherwise does not touch or retrace itself.
    3. The numbers along the edge indicate the stretch of the nearest section
       of road from that point, in corresponding row or column.
    4. For example if the first stretch of road is curve, straight and curve
       the number of that hint is 3.
    5. Another example: if the first stretch of road is curve and curve,
       the number of that hint is 2.
    6. Not all tiles should be used. In some levels some part of the board
       can remain unused.
*/

namespace puzzles::CrosstownTraffic{

constexpr auto PUZ_UNKNOWN = -1;

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
constexpr int lineseg_cross = 15;
const vector<int> linesegs_all = {
    // ┼ ┐  ─  ┌  ┘  │  └
    15, 12, 10, 6, 9, 5, 3,
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_hint
{
    int m_num;
    map<int, vector<Position>> m_num2rng;
};

using puz_dot = vector<int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    // key: 0 -- sidelen - 1: row left
    //      sidelen -- sidelen * 2 - 1: row right 
    //      sidelen * 2 -- sidelen * 3 - 1: column top 
    //      sidelen * 3 -- sidelen * 4 - 1: column bottom
    map<int, puz_hint> m_rc2hint;
    // key: is_row, num, is_left
    map<tuple<bool, int, bool>, vector<vector<int>>> m_pattern2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 2)
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen + 2; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen + 2; ++c)
            if (char ch = str[c]; ch != ' ')
                if (c == 0 || c == m_sidelen + 1) {
                    int idx = r - 1 + (c == 0 ? 0 : m_sidelen);
                    auto& [num, num2rng] = m_rc2hint[idx];
                    num = ch - '0';
                    for (int i = num == 0 ? m_sidelen : num; i <= m_sidelen; ++i) {
                        auto& rng = num2rng[i];
                        for (int j = 0; j < i; ++j)
                            rng.emplace_back(r - 1, c == 0 ? j : j + m_sidelen - i);
                    }
                } else if (r == 0 || r == m_sidelen + 1) {
                    int idx = c - 1 + m_sidelen * 2 + (r == 0 ? 0 : m_sidelen);
                    auto& [num, num2rng] = m_rc2hint[idx];
                    num = ch - '0';
                    for (int i = num == 0 ? m_sidelen : num; i <= m_sidelen; ++i) {
                        auto& rng = num2rng[i];
                        for (int j = 0; j < i; ++j)
                            rng.emplace_back(r == 0 ? j : j + m_sidelen - i, c - 1);
                    }
                }
    }
    for (auto& [rc, hint] : m_rc2hint) {
        auto& [num, num2rng] = hint;
        bool is_row = rc < m_sidelen * 2;
        bool is_left = rc - (is_row ? 0 : m_sidelen * 2) < m_sidelen;
        auto& perms = m_pattern2perms[{is_row, num, is_left}];
        if (!perms.empty()) continue;
        for (auto& [num2, rng] : num2rng)
            for (int i = 0; i < (1 << num2); ++i) {
                vector<int> perm;
                for (int j = 0; j < num2; ++j) {
                    bool is_not_road = is_left ? j < num2 - num : j >= num;
                    bool is_road_left = j == (is_left ? num2 - num : 0);
                    bool is_road_right = j == (is_left ? num2 - 1 : num - 1);
                    bool is_on = (i & (1 << j)) != 0;
                    perm.push_back(
                        is_not_road ? is_on ? lineseg_off : is_row ? 5 : 10 :
                        is_road_left ? is_on ? 6 : is_row ? 3 : 12 :
                        is_road_right ? is_on ? 9 : is_row ? 12 : 3 :
                        is_on ? 15 : is_row ? 10 : 5
                    );
                }
                perms.push_back(perm);
            }
    }
}

struct puz_state : vector<puz_dot>
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move_hint(int rc, int n);
    void make_move_hint2(int rc, int n);
    bool make_move_dot(const Position& p, int n);
    int find_matches(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, vector<int>> m_matches;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count, {lineseg_off}), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            for (int lineseg : linesegs_all)
                if ([&] {
                    for (int i = 0; i < 4; ++i)
                        // A line segment cannot go beyond the boundaries of the board 
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& [rc, hint] : g.m_rc2hint) {
        auto& [num, num2rng] = hint;
        bool is_row = rc < sidelen() * 2;
        bool is_left = rc - (is_row ? 0 : sidelen() * 2) < sidelen();
        auto& perms = g.m_pattern2perms.at({is_row, num, is_left});
        auto& perm_ids = m_matches[rc];
        perm_ids.resize(perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [rc, perm_ids] : m_matches) {
        auto& [num, num2rng] = m_game->m_rc2hint.at(rc);
        bool is_row = rc < sidelen() * 2;
        bool is_left = rc - (is_row ? 0 : sidelen() * 2) < sidelen();
        auto& perms = m_game->m_pattern2perms.at({is_row, num, is_left});

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            int sz = perm.size();
            auto& rng = num2rng.at(sz);
            for (int i = 0; i < sz; ++i)
                if (boost::algorithm::none_of_equal(dots(rng[i]), perm[i]))
                    return true;
            return false;
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(rc, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move_hint2(int rc, int n)
{
    auto& [num, num2rng] = m_game->m_rc2hint.at(rc);
    bool is_row = rc < sidelen() * 2;
    bool is_left = rc - (is_row ? 0 : sidelen() * 2) < sidelen();
    auto& perms = m_game->m_pattern2perms.at({is_row, num, is_left});
    auto& perm = perms[n];
    int sz = perm.size();
    auto& rng = num2rng.at(sz);
    for (int i = 0; i < sz; ++i)
        dots(rng[i]) = {perm[i]};
    m_matches.erase(rc);
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                if (dt.size() == 1 && !m_finished.contains(p))
                    newly_finished.insert(p);
            }

        if (newly_finished.empty())
            return n;

        n = 1;
        for (const auto& p : newly_finished) {
            int lineseg = dots(p)[0];
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                if (!is_valid(p2))
                    continue;
                auto& dt = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(p);
        }
        m_distance += newly_finished.size();
    }
}

bool puz_state::check_loop() const
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (dt.size() == 1 && dt[0] != lineseg_off)
                rng.insert(p);
        }

    bool has_branch = false;
    while (!rng.empty()) {
        auto p = *boost::find_if(rng, [&](const Position& p2) {
            return dots(p2)[0] != lineseg_cross;
        });
        auto p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            auto& lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n && (lineseg != lineseg_cross || i == n)) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p && lineseg != lineseg_cross)
                // we have a loop here,
                // and we are supposed to have exhausted the line segments
                return !has_branch && rng.empty();
            if (!rng.contains(p2)) {
                has_branch = true;
                break;
            }
        }
    }
    return true;
}

bool puz_state::make_move_hint(int rc, int n)
{
    m_distance = 0;
    make_move_hint2(rc, n);
    for (;;) {
        int m;
        while ((m = find_matches(false)) == 1);
        if (m == 0)
            return false;
        m = check_dots(false);
        if (m != 1)
            return m == 2;
        if (!check_loop())
            return false;
    }
}

bool puz_state::make_move_dot(const Position& p, int n)
{
    m_distance = 0;
    auto& dt = dots(p);
    dt = {dt[n]};
    int m = check_dots(false);
    return m == 1 ? check_loop() : m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [rc, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const int, vector<int>>& kv1,
            const pair<const int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (auto& n : perm_ids) {
            children.push_back(*this);
            if (!children.back().make_move_hint(rc, n))
                children.pop_back();
        }
    } else {
        int i = boost::min_element(*this, [&](const puz_dot& dt1, const puz_dot& dt2) {
            auto f = [](const puz_dot& dt) {
                int sz = dt.size();
                return sz == 1 ? 100 : sz;
                };
            return f(dt1) < f(dt2);
        }) - begin();
        auto& dt = (*this)[i];
        Position p(i / sidelen(), i % sidelen());
        for (int n = 0; n < dt.size(); ++n) {
            children.push_back(*this);
            if (!children.back().make_move_dot(p, n))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int c = 0; c < sidelen(); ++c)
        if (auto it = m_game->m_rc2hint.find(c + m_game->m_sidelen * 2);
            it != m_game->m_rc2hint.end())
            out << ' ' << it->second.m_num;
        else
            out << "  ";
    println(out);
    for (int r = 0;; ++r) {
        if (auto it = m_game->m_rc2hint.find(r);
            it != m_game->m_rc2hint.end())
            out << it->second.m_num;
        else
            out << ' ';
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            out << '.';
            if (c == sidelen() - 1) break;
            out << (is_lineseg_on(dots({r, c})[0], 1) ? '-' : ' ');
        }
        if (auto it = m_game->m_rc2hint.find(r + m_game->m_sidelen);
            it != m_game->m_rc2hint.end())
            out << it->second.m_num;
        else
            out << ' ';
            out << ' ';
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            out << ' ';
            // draw vert-lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? '|' : ' ');
            if (c == sidelen() - 1) break;
        }
        println(out);
    }
    for (int c = 0; c < sidelen(); ++c)
        if (auto it = m_game->m_rc2hint.find(c + m_game->m_sidelen * 3);
            it != m_game->m_rc2hint.end())
            out << ' ' << it->second.m_num;
        else
            out << "  ";
    println(out);
    return out;
}

}

void solve_puz_CrosstownTraffic()
{
    using namespace puzzles::CrosstownTraffic;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CrosstownTraffic.xml", "Puzzles/CrosstownTraffic.txt", solution_format::GOAL_STATE_ONLY);
}
