#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 12/Sheep & Wolves

    Summary
    Where's a dog when you need one?

    Description
    1. Plays like SlitherLink:
    2. Draw a single looping path with the aid of the numbered hints. The
       path cannot have branches or cross itself.
    3. Each number tells you on how many of its four sides are touched by
       the path.
    4. With this added rule:
    5. In the end all the sheep must be corralled inside the loop, while
       all the wolves must be outside.
*/

namespace puzzles::SheepAndWolves{

constexpr auto PUZ_UNKNOWN = -1;
constexpr auto PUZ_LINE_OFF = '0';
constexpr auto PUZ_LINE_ON = '1';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SHEEP = 'S';
constexpr auto PUZ_WOLF = 'W';

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
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

/*
    cell(0,0)        cell(0,1)
             dot(0,0)
    cell(1,0)        cell(1,1)
*/
constexpr Position offset2[] = {
    {0, 0}, {0, 1},    // n
    {0, 1}, {1, 1},    // e
    {1, 1}, {1, 0},    // s
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
constexpr puz_line_info lines_info[] = {
    {{-1, -1}, 1}, {{-1, 0}, 3},       // n
    {{-1, 0}, 2}, {{0, 0}, 0},         // e
    {{0, 0}, 3}, {{0, -1}, 1},         // s
    {{0, -1}, 0}, {{-1, -1}, 2},       // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, int> m_pos2num;
    map<int, vector<int>> m_num2perms;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * (m_sidelen + 1) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 1)
, m_dot_count(m_sidelen * m_sidelen)
{
    // 5. In the end all the sheep must be corralled inside the loop, while
    // all the wolves must be outside.
    m_cells.append(m_sidelen + 1, PUZ_WOLF);
    for (int r = 1; r < m_sidelen; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_WOLF);
        for (int c = 1; c < m_sidelen; ++c) {
            char ch = str[c - 1];
            bool b = isdigit(ch);
            m_cells.push_back(b ? PUZ_SPACE : ch);
            m_pos2num[{r, c}] = b ? ch - '0' : PUZ_UNKNOWN;
        }
        m_cells.push_back(PUZ_WOLF);
    }
    m_cells.append(m_sidelen + 1, PUZ_WOLF);

    // 3. Each number tells you on how many of its four sides are touched
    // by the path.
    auto& perms_unknown = m_num2perms[PUZ_UNKNOWN];
    for (int i = 0; i < 4; ++i) {
        auto& perms = m_num2perms[i];
        auto indicator = string(4 - i, PUZ_LINE_OFF) + string(i, PUZ_LINE_ON);
        do {
            int perm = 0;
            for (int j = 0; j < 4; ++j)
                if (indicator[j] == PUZ_LINE_ON)
                    perm += (1 << j);
            perms.push_back(perm);
            perms_unknown.push_back(perm);
        } while(boost::next_permutation(indicator));
    }
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
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    int check_cells(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    vector<puz_dot> m_dots;
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished_dots;
    set<Position> m_finished_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count, {lineseg_off}), m_game(&g)
, m_cells(g.m_cells)
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

    for (auto& [p, n] : g.m_pos2num) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_num2perms.at(n).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    while ((check_cells(true), check_dots(true)) == 1);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            for (int i = 0; i < 8; ++i) {
                auto& [os, j] = lines_info[i];
                const auto& dt = dots(p + os);
                bool is_on = is_lineseg_on(perm, i / 2);
                if (boost::algorithm::none_of(dt, [=](int lineseg) {
                    return is_lineseg_on(lineseg, j) == is_on;
                }))
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
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
                bool is_on = ch != ch2;
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
        set<pair<Position, int>> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                for (int i = 0; i < 4; ++i)
                    if (!m_finished_dots.contains({p, i}) && (
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
                if (!init && dt.empty())
                    return 0;
            }
            m_finished_dots.insert(kv);

            char& ch1 = cells(p + offset2[i * 2]);
            char& ch2 = cells(p + offset2[i * 2 + 1]);
            bool is_on = is_lineseg_on(lineseg, i);
            auto f = [=](char& chA, char& chB) {
                // If the line segment is on,
                // the two adjacent cells bordering it must belong to the same group
                // Otherwise, they must belong to the two different groups
                chA = !is_on ? chB :
                    chB == PUZ_SHEEP ? PUZ_WOLF : PUZ_SHEEP;
            };
            if ((ch1 == PUZ_SPACE) != (ch2 == PUZ_SPACE))
                ch1 == PUZ_SPACE ? f(ch1, ch2) : f(ch2, ch1);
        }
        //m_distance += newly_finished.size();
    }
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    for (int i = 0; i < 8; ++i) {
        auto& [os, j] = lines_info[i];
        auto& dt = dots(p + os);
        bool is_on = is_lineseg_on(perm, i / 2);
        boost::remove_erase_if(dt, [=](int lineseg) {
            return is_lineseg_on(lineseg, j) != is_on;
        });
    }

    ++m_distance;
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
            if (dt.size() != 1 && m_matches.empty())
                return false;
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

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
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

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (is_lineseg_on(dots({r, c})[0], 1) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (is_lineseg_on(dots(p)[0], 2) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            auto p2 = p + Position(1, 1);
            int n = m_game->m_pos2num.at(p2);
            if (n == PUZ_UNKNOWN)
                out << m_game->cells(p2);
            else
                out << n;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_SheepAndWolves()
{
    using namespace puzzles::SheepAndWolves;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SheepAndWolves.xml", "Puzzles/SheepAndWolves.txt", solution_format::GOAL_STATE_ONLY);
}
