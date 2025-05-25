#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 10/Mirrors

    Summary
    Zip, swish, zoom! Lasers on mirrors!

    Description
    1. The goal is to draw a single, continuous, non-crossing path that fills
       the entire board.
    2. Some tiles are already given and can contain Mirrors, which force the
       path to make a turn. Other tiles already contain a fixed piece of straight
       path.
    3. Your task is to fill the remaining board tiles with straight or 90 degree
       path lines, in the end connecting a single, continuous line.
    4. Please note you can make 90 degree turn even there are no mirrors.

    Variant
    5. In the Maze variant, the path isn't closed. You have two spots on the
       board which represent the start and end of the path.
*/

namespace puzzles::Mirrors{

constexpr auto PUZ_LINE_OFF = '0';
constexpr auto PUZ_LINE_ON = '1';
constexpr auto PUZ_BLOCK = "B ";
constexpr auto PUZ_SPOT = "S ";

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
const vector<int> lines_spot = {
    // ╵ ╶ ╷ ╴
    1, 2, 4, 8,
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, int> m_pos2lineseg;
    set<Position> m_spots;

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
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            auto s = str.substr(2 * c, 2);
            Position p(r, c);
            if (s == PUZ_SPOT)
                m_spots.insert(p);
            else if (s != "  ")
                m_pos2lineseg[{r, c}] =
                    s == PUZ_BLOCK ? lineseg_off :
                    (1 << (s[0] - '0')) | (1 << (s[1] - '0'));
        }
    }
}

typedef vector<int> puz_dot;

struct puz_state : vector<puz_dot>
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, int n);
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
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            auto it = g.m_pos2lineseg.find(p);
            if (it != g.m_pos2lineseg.end())
                dt = {it->second};
            else {
                auto& linesegs_all2 = g.m_spots.contains(p) ? lines_spot : linesegs_all;
                for (int lineseg : linesegs_all2)
                    if ([&]{
                        for (int i = 0; i < 4; ++i)
                            if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                                return false;
                        return true;
                    }())
                        dt.push_back(lineseg);
            }
        }

    check_dots(true);
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
                boost::remove_erase_if(dt, [&, i](int lineseg2) {
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

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    auto& dt = dots(p);
    dt = {dt[n]};
    return check_dots(false) != 0 && check_loop();
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
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // so we should have exhausted the line segments 
                return rng.empty() && m_game->m_spots.empty();
            if (!rng.contains(p2))
                break;
        }
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int n = boost::min_element(*this, [](const puz_dot& dt1, const puz_dot& dt2) {
        auto f = [](int sz) {return sz == 1 ? 1000 : sz; };
        return f(dt1.size()) < f(dt2.size());
    }) - begin();
    Position p(n / sidelen(), n % sidelen());
    auto& dt = dots(p);
    for (int i = 0; i < dt.size(); ++i) {
        children.push_back(*this);
        if (!children.back().make_move(p, i))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            out << (m_game->m_spots.contains(p) ? PUZ_SPOT :
                dt[0] == lineseg_off ? PUZ_BLOCK :
                is_lineseg_on(dt[0], 1) ? " -" : "  ");
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vert-lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_Mirrors()
{
    using namespace puzzles::Mirrors;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Mirrors.xml", "Puzzles/Mirrors.txt", solution_format::GOAL_STATE_ONLY);
}
