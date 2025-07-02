#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Light Connect Puzzle!

    Summary
    Connect Lamps and Batteries

    Description
*/

namespace puzzles::LightConnect{

constexpr auto PUZ_PIPE_OFF = '0';
constexpr auto PUZ_PIPE_I = 'I';
constexpr auto PUZ_PIPE_L = 'L';
constexpr auto PUZ_PIPE_3 = '3';
constexpr auto PUZ_BATTERY = 'B';
constexpr auto PUZ_LAMP = 'P';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

constexpr int lineseg_off = 0;
inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<vector<int>> linesegs_all = {
    {lineseg_off},
    // ╵(up) ╶(right) ╷(down) ╴(left)
    {1, 2, 4, 8},
    // └  ┌  ┐  ┘
    {3, 6, 12, 9},
    // │  ─
    {5, 10},
    // ┴  ├  ┬  ┤
    {11, 7, 14, 13},
};

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

using puz_dot = vector<int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    string m_cells;
    set<Position> m_batteries, m_lamps;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            switch (Position p(r, c);  str[c]) {
            case PUZ_BATTERY: m_batteries.insert(p); break;
            case PUZ_LAMP: m_lamps.insert(p); break;
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
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_dots < x.m_dots; }
    bool make_move_dot(const Position& p, int n);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count * 4 - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<puz_dot> m_dots;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = m_game->cells(p);
            auto& linesegs_all2 = linesegs_all[
                ch == PUZ_BATTERY || ch == PUZ_LAMP ? 1 :
                ch == PUZ_PIPE_L ? 2 :
                ch == PUZ_PIPE_I ? 3 :
                ch == PUZ_PIPE_3 ? 4 :
                0];
            auto& dt = dots(p);
            for (int lineseg : linesegs_all2)
                if ([&]{
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i];
                        // A line segment cannot go beyond the boundaries of the board
                        if (!is_valid(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    check_dots(true);
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
            int lineseg = dots(p)[0];
            auto p2 = p + offset[i];
            if (is_valid(p2)) {
                auto& dt = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(kv);
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
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            auto& lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // loop is not allowed here
                return false;
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
        if (children.push_back(*this); !children.back().make_move_dot(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            char ch = m_game->cells(p);
            ch = ch == PUZ_BATTERY || ch == PUZ_LAMP ? ch : '.';
            out << ch << (is_lineseg_on(dt[0], 1) ? '-' : ' ');
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_LightConnect()
{
    using namespace puzzles::LightConnect;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LightConnect.xml", "Puzzles/LightConnect.txt", solution_format::GOAL_STATE_ONLY);
}
