#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 4/Guess the Labyrinth

    Summary
    Before solving it

    Description
    1. There is a hidden Labyrinth in the board.
    2. The Labyrinth is a one-square wide path which doesn't branch out and
       that forms a closed loop
    3. The intersections where three lines meet are marked with a dot
*/

namespace puzzles::GuessTheLabyrinth{

constexpr auto PUZ_POST = 'O';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
const vector<vector<int>> linesegs_all_border = {
    {6}, // Ñ°
    {12}, // Ñ¢
    {14, 10}, // Ñ¶ Ñü
    {3}, // Ñ§
    {9}, // Ñ£
    {11, 10}, // Ñ® Ñü
    {7, 5}, // Ñ• Ñ†
    {13, 5}, // Ñß Ñ†
};
const vector<int> linesegs_all_inside = {
    // ┐  ─  ┌  ┘  │  └
    lineseg_off, 12, 10, 6, 9, 5, 3, 1, 2, 4, 8
};
const vector<int> linesegs_all_post = {
    // ┴  ├  ┬  ┤
    lineseg_off, 11, 7, 14, 13,
};

constexpr array<Position, 4> offset = Position::Directions4;

constexpr Position offset2[] = {
    {0, 0},        // o
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
};

struct puz_game
{
    string m_id;
    Position m_size;
    int m_dot_count;
    set<Position> m_posts;
    set<Position> m_all_positions;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_size(strs.size() / 2 + 1, strs[0].length() / 2 + 1)
, m_dot_count(rows() * cols())
{
    for (int r = 0; r < rows(); ++r) {
        string_view str_h = strs[r * 2];
        // posts
        for (int c = 0; c < cols(); ++c)
            if (str_h[c * 2] == PUZ_POST)
                m_posts.emplace(r, c);
    }
    for (int r = 0; r < rows() - 1; ++r)
        for (int c = 0; c < cols() - 1; ++c)
            m_all_positions.emplace(r, c);
}

using puz_dot = vector<int>;

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * cols() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
    bool make_move_dot(const Position& p, int n);
    int check_dots(bool init);
    bool check_route() const;

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
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (r > 0 && c > 0 && r < rows() - 1 && c < cols() - 1)
                dt = g.m_posts.contains(p) ? linesegs_all_post : linesegs_all_inside;
            else {
                int n =
                    r == 0 ? (c == 0 ? 0 : c == cols() - 1 ? 1 : 2) :
                    r == rows() - 1 ? (c == 0 ? 3 : c == cols() - 1 ? 4 : 5) :
                    c == 0 ? 6 : 7;
                dt = linesegs_all_border[n];
                if (dt.size() > 1)
                    dt = {dt[g.m_posts.contains(p) ? 0 : 1]};
            }
        }

    check_dots(true);
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<pair<Position, int>> newly_finished;
        for (int r = 0; r < rows(); ++r)
            for (int c = 0; c < cols(); ++c) {
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

bool puz_state::check_route() const
{
    map<Position, vector<int>> pos2dirs;
    for (auto& p : m_game->m_all_positions) {
        int num = 2;
        int max_possible = 0;
        int min_guaranteed = 0;
        vector<int> dirs;
        for (int i = 0; i < 4; ++i) {
            auto& dt = dots(p + offset2[i]);
            int d = (i + 1) % 4;
            if (boost::algorithm::any_of(dt, [&](int lineseg) {
                return !is_lineseg_on(lineseg, d);
            })) max_possible++;
            if (boost::algorithm::all_of(dt, [&](int lineseg) {
                return !is_lineseg_on(lineseg, d);
            })) min_guaranteed++, dirs.push_back(i);
        }
        // Prune if we can't possibly reach the target
        if (max_possible < num) return false;
        // Prune if we have already exceeded the target
        if (min_guaranteed > num) return false;
        // Final check
        if (is_goal_state() && min_guaranteed != num) return false;
        if (min_guaranteed == num)
            pos2dirs[p] = dirs;
    }

    bool has_branch = false;
    while (!pos2dirs.empty()) {
        auto p = pos2dirs.begin()->first, p2 = p;
        for (int n = -1;;) {
            auto dirs = pos2dirs.at(p2);
            pos2dirs.erase(p2);
            for (int i : dirs)
                // proceed only if the route does not revisit the previous position
                if ((i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // and we are supposed to have exhausted the line segments
                return !has_branch && pos2dirs.empty();
            if (!pos2dirs.contains(p2)) {
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
    return m == 1 ? check_route() : m == 2;
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
    Position p(i / cols(), i % cols());
    for (int n = 0; n < dt.size(); ++n)
        if (!children.emplace_back(*this).make_move_dot(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            out << (m_game->m_posts.contains(p) ? PUZ_POST : ' ')
                << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
        }
        println(out);
        if (r == rows() - 1) break;
        for (int c = 0; c < cols(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_GuessTheLabyrinth()
{
    using namespace puzzles::GuessTheLabyrinth;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/GuessTheLabyrinth.xml", "Puzzles/GuessTheLabyrinth.txt", solution_format::GOAL_STATE_ONLY);
}
