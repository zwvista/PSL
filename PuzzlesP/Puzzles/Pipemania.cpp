#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 5/Pipemania

    Summary
    Back to the 80s

    Description
    1. The former contractor for your present client left the work unfinished.
       In order not to waste what has bee done, you should complete the pipe
       loop, using the pieces available.
    2. Complete the board using all the tiles adn form a single closed loop.
    3. The loop can cross itself.
*/

namespace puzzles::Pipemania{

#define PUZ_BLOCK           'B'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┼ ┐  ─  ┌  ┘  │  └
    15, 12, 10, 6, 9, 5, 3,
};

const Position offset[] = {
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
    map<Position, int> m_pos2dot;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_pos2dot[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
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
    bool make_move_dot(const Position& p, int n);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

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
            if (g.m_pos2dot.count(p) != 0)
                dt.push_back(g.m_pos2dot.at(p));
            else
                for (int lineseg : linesegs_all)
                    if ([&] {
                        for (int i = 0; i < 4; ++i)
                            // The line segment cannot lead to a position outside the board 
                            if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                                return false;
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
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                if (dt.size() == 1 && m_finished.count(p) == 0)
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
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            auto& lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                // go ahead if the line segment does not lead a way back
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // so we should have exhausted the line segments 
                return !has_branch && rng.empty();
            if (rng.count(p2) == 0) {
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

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << ' ' << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
        }
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            // draw vert-lines
            out << (is_lineseg_on(dots({ r, c })[0], 2) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Pipemania()
{
    using namespace puzzles::Pipemania;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Pipemania.xml", "Puzzles/Pipemania.txt", solution_format::GOAL_STATE_ONLY);
}
