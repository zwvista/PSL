#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/Traffic Warden Revenge

    Summary
    But the ...oh well

    Description
    1. Draw a single non intersecting loop passing through all traffic lights.
    2. Green light means the road that extends from there is of equal length
       in both directions.
    3. Red light means they are not.
    4. A number tells you the sum of the length or road extending from that
       traffic light, be it green or red.
*/

namespace puzzles::TrafficWardenRevenge{

constexpr auto PUZ_GREEN = 'G';
constexpr auto PUZ_RED = 'R';
constexpr int PUZ_UNKNOWN = -1;
constexpr int PUZ_UNKNOWN_10 = -2;

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_light
{
    char m_kind;
    int m_sum;
};

struct puz_path
{
    // elem 1: first direction
    // elem 2: second direction
    vector<vector<Position>> m_rng2D_straight = vector<vector<Position>>(2);
    // elem 1: first direction
    // elem 2: second direction
    vector<Position> m_rng_turn = vector<Position>(2);
    int m_lineseg_light;
    // elem 1: first direction
    // elem 2: second direction
    vector<int> m_linesegs_straight = vector<int>(2);
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, puz_light> m_pos2light;
    map<Position, vector<puz_path>> m_pos2paths;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (auto s = str.substr(c * 2, 2); s != "  ")
                // R1 and B1 is impossible. They represent a number between 10 and 19.
                m_pos2light[{r, c}] = {s[0], s[1] == '1' ? PUZ_UNKNOWN_10 : isdigit(s[1]) ? s[1] - '0' : PUZ_UNKNOWN};
    }
    for (auto& [p, light] : m_pos2light) {
        auto& [kind, sum] = light;
        auto& paths = m_pos2paths[p];
        vector<vector<int>> perms;
        // 2. Green light means the road that extends from there is of equal length
        // in both directions.
        // 3. Red light means they are not.
        int nMax = sum == PUZ_UNKNOWN || sum == PUZ_UNKNOWN_10 ? m_sidelen - 1 : min(sum, m_sidelen - 1);
        for (int n1 = 1; n1 <= nMax; ++n1)
            for (int n2 = 1; n2 <= nMax; ++n2)
                if ((sum == PUZ_UNKNOWN || sum == PUZ_UNKNOWN_10 && n1 + n2 >= 10 || n1 + n2 == sum) && (n1 == n2) == (kind == PUZ_GREEN))
                    perms.push_back({n1, n2});
        for (int lineseg : linesegs_all) {
            vector<int> dirs;
            for (int i = 0; i < 4; ++i)
                if (is_lineseg_on(lineseg, i))
                    dirs.push_back(i);
            for (auto& perm : perms) {
                puz_path path;
                auto& [rng2D_straight, rng_turn, ls_light, lss_straight] = path;
                ls_light = lineseg;
                if ([&] {
                    for (int i = 0; i < 2; ++i) {
                        int dir = dirs[i], n = perm[i];
                        lss_straight[i] = dir % 2 == 0 ? 5 : 10;
                        auto& os = offset[dir];
                        auto p2 = p + os;
                        for (int j = 1;; ++j, p2 += os) {
                            if (!is_valid(p2))
                                return false;
                            if (j == n) {
                                rng_turn[i] = p2;
                                break;
                            }
                            rng2D_straight[i].push_back(p2);
                        }
                    }
                    return true;
                }())
                    paths.push_back(path);
            }
        }
    }
}

using puz_dot = vector<int>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots); 
    }
    bool make_move_light(const Position& p, int n);
    void make_move_light2(const Position& p, int n);
    bool make_move_dot(const Position& p, int n);
    int find_matches(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_game->m_dot_count * 4 - m_finished.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_dot> m_dots;
    // key: the position of the dot
    // value: the index of the permutation
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_dots(g.m_dot_count, {lineseg_off})
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            for (int lineseg : linesegs_all)
                if ([&]{
                    for (int i = 0; i < 4; ++i)
                        // A line segment cannot go beyond the boundaries of the board
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& [p, paths] : m_game->m_pos2paths) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(paths.size());
        boost::iota(perm_ids, 0);
    }
    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, path_ids] : m_matches) {
        auto& paths = m_game->m_pos2paths.at(p);
        boost::remove_erase_if(path_ids, [&](int id) {
            auto& [rng2D_straight, rng_turn, ls_light, lss_straight] = paths[id];
            for (int i = 0; i < 2; ++i)
                if (boost::algorithm::any_of(rng2D_straight[i], [&](const Position& p2) {
                    return boost::algorithm::none_of_equal(dots(p2), lss_straight[i]);
                }) || boost::algorithm::all_of_equal(dots(rng_turn[i]), lss_straight[i]))
                    return true;
            return boost::algorithm::none_of_equal(dots(p), ls_light);
        });

        if (!init)
            switch(path_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_light2(p, path_ids.front()), 1;
            }
    }
    return 2;
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<pair<Position, int>> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                for (int i = 0; i < 4; ++i)
                    if (!m_finished.contains({p, i}) && (
                        boost::algorithm::all_of(dt, [=](int lineseg) {
                        return is_lineseg_on(lineseg, i);
                    }) || boost::algorithm::all_of(dt, [=](int lineseg) {
                        return !is_lineseg_on(lineseg, i);
                    })))
                        newly_finished.emplace(p, i);
            }

        if (newly_finished.empty())
            return n;

        n = 1;
        for (const auto& kv : newly_finished) {
            auto& [p, i] = kv;
            auto& dt = dots(p);
            if (dt.empty())
                return 0;
            int lineseg = dt[0];
            auto p2 = p + offset[i];
            if (is_valid(p2)) {
                auto& dt2 = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt2, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt2.empty())
                    return 0;
            }
            m_finished.insert(kv);
        }
        m_distance += newly_finished.size();
    }
}

void puz_state::make_move_light2(const Position& p, int n)
{
    auto& [rng2D_straight, rng_turn, ls_light, lss_straight] = m_game->m_pos2paths.at(p)[n];
    dots(p) = {ls_light};
    for (int i = 0; i < 2; ++i) {
        int lineseg = lss_straight[i];
        for (auto& p2 : rng2D_straight[i])
            dots(p2) = {lineseg};
        boost::remove_erase(dots(rng_turn[i]), lineseg);
    }

    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move_light(const Position& p, int n)
{
    m_distance = 0;
    make_move_light2(p, n);
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
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            int lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
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
        auto& [p, path_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : path_ids)
            if (!children.emplace_back(*this).make_move_light(p, n))
                children.pop_back();
    } else {
        int i = boost::min_element(m_dots, [&](const puz_dot& dt1, const puz_dot& dt2) {
            auto f = [](const puz_dot& dt) {
                int sz = dt.size();
                return sz == 1 ? 100 : sz;
            };
            return f(dt1) < f(dt2);
        }) - m_dots.begin();
        auto& dt = m_dots[i];
        Position p(i / sidelen(), i % sidelen());
        for (int n = 0; n < dt.size(); ++n)
            if (!children.emplace_back(*this).make_move_dot(p, n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (auto it = m_game->m_pos2light.find(p); it == m_game->m_pos2light.end())
                out << "...";
            else {
                auto& [kind, sum] = it->second;
                out << format("{}{:2}", kind, sum);
            }
            out << (is_lineseg_on(dt[0], 1) ? "===" : "   ");
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| |   " : "      ");
        println(out);
    }
    return out;
}

}

void solve_puz_TrafficWardenRevenge()
{
    using namespace puzzles::TrafficWardenRevenge;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TrafficWardenRevenge.xml", "Puzzles/TrafficWardenRevenge.txt", solution_format::GOAL_STATE_ONLY);
}
