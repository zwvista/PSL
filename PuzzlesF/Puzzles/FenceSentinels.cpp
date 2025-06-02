#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 12/Fence Sentinels

    Summary
    We used to guard a castle, you know?

    Description
    1. The goal is to draw a single, uninterrupted, closed loop.
    2. The loop goes around all the numbers.
    3. The number tells you how many cells you can see horizontally or
       vertically from there, including the cell itself.

    Variant
    4. Some levels are marked 'Inside Outside'. In this case some numbers
       are on the outside of the loop.
*/

namespace puzzles::FenceSentinels{

constexpr auto PUZ_BOUNDARY = '+';
constexpr auto PUZ_INSIDE = 'I';
constexpr auto PUZ_OUTSIDE = 'O';
constexpr auto PUZ_SPACE = ' ';

inline bool is_inside(char ch) { return ch == PUZ_INSIDE; }

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

/*
    cell(0,0)        cell(0,1)
             dot(0,0)
    cell(1,0)        cell(1,1)
*/
constexpr Position offset2[] = {
    {0, 0}, {0, 1},    // n
    {0, 1}, {1, 1},        // e
    {1, 1}, {1, 0},        // s
    {1, 0}, {0, 0},    // w
};

/*
    dot(-1,-1) -> 1  3 <- dot(-1, 0)
      |                          |
      v                          v
      2                          2
                cell(0,0)
      0                          0
      ^                          ^
      |                          |
    dot( 0,-1) -> 1  3 <- dot( 0, 0)
*/
using puz_line_info = pair<Position, int>;
const puz_line_info lines_info[] = {
    {{-1, -1}, 1}, {{-1, 0}, 3},         // n
    {{-1, 0}, 2}, {{0, 0}, 0},         // e
    {{0, 0}, 3}, {{0, -1}, 1},         // s
    {{0, -1}, 0}, {{-1, -1}, 2},         // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    bool m_inside_outside;
    map<Position, int> m_pos2num;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_inside_outside(level.attribute("InsideOutside").as_int() == 1)
    , m_sidelen(strs.size() + 1)
    , m_dot_count(m_sidelen * m_sidelen)
{
    m_start.append(m_sidelen + 1, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen; ++c) {
            char ch = str[c - 1];
            if (ch != ' ') {
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_start.push_back(m_inside_outside ? PUZ_SPACE : PUZ_INSIDE);
            } else
                m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen + 1, PUZ_BOUNDARY);
}

using puz_dot = vector<int>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    char cells(const Position& p) const { return m_cells[p.first * (sidelen() + 1) + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * (sidelen() + 1) + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
    bool make_move_hint(const Position& p, const puz_dot& perm);
    bool make_move_hint2(const Position& p, const puz_dot& perm);
    bool make_move_hint3(const Position& p, const puz_dot& perm, int i, bool stopped);
    bool make_move_dot(const Position& p, int n);
    int find_matches(bool init);
    int check_cells(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_game->m_dot_count - m_finished_dots.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    vector<puz_dot> m_dots;
    map<Position, vector<vector<int>>> m_matches;
    set<Position> m_finished_dots, m_finished_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
, m_dots(g.m_dot_count, {lineseg_off})
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

    for (auto& [p, n] : g.m_pos2num)
        m_matches[p];

    find_matches(true);
    while ((check_cells(true), check_dots(true)) == 1);
}

int puz_state::find_matches(bool init)
{
    bool matches_changed = init;
    for (auto& [p, perms] : m_matches) {
        auto perms_old = perms;
        perms.clear();

        int sum = m_game->m_pos2num.at(p) - 1;
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            int n = 0;
            auto& nums = dir_nums[i];
            for (auto p2 = p; n <= sum; p2 += os) {
                auto f = [&](bool is_on) {
                    auto& [os2, j] = lines_info[i * 2];
                    const auto& dt = dots(p2 + os2);
                    return boost::algorithm::all_of(dt, [=](int lineseg) {
                        return is_lineseg_on(lineseg, j) == is_on;
                    });
                };
                if (cells(p2 + os) == PUZ_BOUNDARY || f(true)) {
                    // we have to stop here
                    nums.push_back(n);
                    break;
                } else if (!m_game->m_inside_outside &&
                    m_game->m_pos2num.contains(p2) && 
                    m_game->m_pos2num.contains(p2 + os) ||
                    f(false))
                    // we cannot stop here
                    ++n;
                else
                    // we can stop here
                    nums.push_back(n++);
            }
        }

        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum)
                            perms.push_back({n0, n1, n2, n3});

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, perms.front()) ? 1 : 0;
            default:
                matches_changed = matches_changed || perms != perms_old;
                break;
            }
    }
    if (!matches_changed)
        return 2;

    for (auto& [p, perms] : m_matches)
        for (int i = 0; i < 4; ++i) {
            auto f = [=](const vector<int>& v1, const vector<int>& v2) {
                return v1[i] < v2[i];
            };
            const auto& perm = *boost::min_element(perms, f);
            int n = boost::max_element(perms, f)->at(i);
            if (!make_move_hint3(p, perm, i, perm[i] == n))
                return 0;
        }
    return 1;
}

int puz_state::check_cells(bool init)
{
    int n = 2;
    for (int r = 1; r < sidelen(); ++r)
        for (int c = 1; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SPACE || m_finished_cells.contains(p))
                continue;
            m_finished_cells.insert(p);
            for (int i = 0; i < 4; ++i) {
                char ch2 = cells(p + offset[i]);
                if (ch2 == PUZ_SPACE)
                    continue;
                n = 1;
                bool is_on = is_inside(ch) != is_inside(ch2);
                for (int j = 0; j < 2; ++j) {
                    auto& [os, k] = lines_info[i * 2 + j];
                    auto& dt = dots(p + os);
                    boost::remove_erase_if(dt, [=](int lineseg) {
                        return is_lineseg_on(lineseg, k) != is_on;
                    });
                    if (!init && dt.empty())
                        return 0;
                }
            }
        }
    return n;
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
                if (dt.size() == 1 && !m_finished_dots.contains(p))
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
                boost::remove_erase_if(dt, [&, i](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt.empty())
                    return 0;
            }
            m_finished_dots.insert(p);

            for (int i = 0; i < 4; ++i) {
                char& ch1 = cells(p + offset2[i * 2]);
                char& ch2 = cells(p + offset2[i * 2 + 1]);
                bool is_on = is_lineseg_on(lineseg, i);
                auto f = [=](char& chA, char& chB) {
                    chA = !is_on ? 
                        is_inside(chB) ? PUZ_INSIDE : PUZ_OUTSIDE :
                        is_inside(chB) ? PUZ_OUTSIDE : PUZ_INSIDE;
                };
                if ((ch1 == PUZ_SPACE) != (ch2 == PUZ_SPACE))
                    ch1 == PUZ_SPACE ? f(ch1, ch2) : f(ch2, ch1);
            }
        }
        m_distance += newly_finished.size();
    }
}

bool puz_state::make_move_hint3(const Position& p, const puz_dot& perm, int i, bool stopped)
{
    int m = perm[i];
    auto p2 = p;
    auto& os = offset[i];
    for (int j = 0; j <= m; ++j, p2 += os) {
        bool is_on = j == m;
        if (is_on && (!stopped || 
            m_game->m_inside_outside && cells(p2 + os) == PUZ_BOUNDARY))
            break;
        for (int k = 0; k < 2; ++k) {
            auto& [os2, l] = lines_info[i * 2 + k];
            auto& dt = dots(p2 + os2);
            boost::remove_erase_if(dt, [&](int lineseg) {
                return is_lineseg_on(lineseg, l) != is_on;
            });
            if (dt.empty())
                return false;
        }
    }
    return true;
}

bool puz_state::make_move_hint2(const Position& p, const puz_dot& perm)
{
    for (int i = 0; i < 4; ++i)
        if (!make_move_hint3(p, perm, i, true))
            return false;

    m_distance++;
    m_matches.erase(p);
    return check_loop();
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
                return rng.empty();
            if (!rng.contains(p2))
                break;
        }
    }
    return true;
}

bool puz_state::make_move_hint(const Position& p, const puz_dot& perm)
{
    m_distance = 0;
    if (!make_move_hint2(p, perm))
        return false;
    for (;;) {
        int m;
        while ((m = find_matches(false)) == 1);
        if (m == 0)
            return false;
        m = check_cells(false);
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
        auto& [p, perms] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<vector<int>>>& kv1,
            const pair<const Position, vector<vector<int>>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (auto& perm : perms) {
            children.push_back(*this);
            if (!children.back().make_move_hint(p, perm))
                children.pop_back();
        }
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
        for (int n = 0; n < dt.size(); ++n) {
            children.push_back(*this);
            if (!children.back().make_move_dot(p, n))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c)
            out << (is_lineseg_on(dots({r, c})[0], 1) ? " --" : "   ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-lines
            out << (is_lineseg_on(dots(p)[0], 2) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            auto p2 = p + Position(1, 1);
            auto it = m_game->m_pos2num.find(p2);
            if (it == m_game->m_pos2num.end())
                out << "  ";
            else
                out << format("{:2}", it->second);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FenceSentinels()
{
    using namespace puzzles::FenceSentinels;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FenceSentinels.xml", "Puzzles/FenceSentinels.txt", solution_format::GOAL_STATE_ONLY);
}
