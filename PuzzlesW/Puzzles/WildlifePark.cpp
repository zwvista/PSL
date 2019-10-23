#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Wildlife Park

    Summary
    One rises, the other falls

    Description
    1. At the last game of Wildlife Football, Lemurs accused Giraffes of cheating,
       Monkeys ran away with the coin after the toss and Lions ate the ball.
    2. So you're given the task to raise some fencing between the different species,
       while spirits cool down.
    3. Fences should encompass at least one animal of a certain species, and all
       animals of a certain species must be in the same enclosure.
    4. There can't be empty enclosures.
    5. Where three or four fences meet, a fence post is put in place. On the Park
       all posts are already marked with a dot.
*/

namespace puzzles::WildlifePark{

#define PUZ_SPACE            ' '
#define PUZ_POST             'O'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // „¢  „Ÿ  „¡  „£  „   „¤
    12, 10, 6, 9, 5, 3,
};
const vector<int> linesegs_all3 = {
    // „¦  „§  „¨  „¥  „©
    14, 13, 11, 7, 15,
};

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    string m_start;
    set<Position> m_posts;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * (m_sidelen - 1) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 + 1)
    , m_dot_count(m_sidelen* m_sidelen)
{
    for (int r = 0;; ++r) {
        auto& str_h = strs[r * 2];
        // posts
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2] == PUZ_POST)
                m_posts.insert({r, c});
        if (r == m_sidelen - 1) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
        }
    }
}

typedef vector<int> puz_dot;

struct puz_state : vector<puz_dot>
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int check_dots(bool init);

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
            bool is_dot3 = g.m_posts.count(p) != 0;
            if (r > 0 && c > 0 && r < sidelen() - 1 && c < sidelen() - 1)
                if (is_dot3)
                    dt = linesegs_all3;
                else {
                    dt.push_back(lineseg_off);
                    dt.insert(dt.end(), linesegs_all.begin(), linesegs_all.end());
                }
            else {
                int n =
                    r == 0 ? (c == 0 ? 6 : c == sidelen() - 1 ? 12 : is_dot3 ? 14 : 10) :
                    r == sidelen() - 1 ? (c == 0 ? 3 : c == sidelen() - 1 ? 9 : is_dot3 ? 11 : 10) :
                    c == 0 ? (is_dot3 ? 7 : 5) : (is_dot3 ? 13 : 5);
                dt.push_back(n);
            }
        }

    auto f = [&](int r, int c, bool b, int d) {
        boost::remove_erase_if(dots({r, c}), [=](int lineseg) {
            return b == is_lineseg_on(lineseg, d);
        });
    };
    for (int r = 0; r < sidelen() - 2; ++r)
        for (int c = 0; c < sidelen() - 2; ++c) {
            Position p1(r, c), p2(r, c + 1), p3(r + 1, c);
            char ch1 = g.cells(p1), ch2 = g.cells(p2), ch3 = g.cells(p3);
            if (ch1 != PUZ_SPACE && ch2 != PUZ_SPACE) {
                bool b = ch1 == ch2;
                f(r, c + 1, b, 2);
                f(r + 1, c + 1, b, 0);
            }
            if (ch1 != PUZ_SPACE && ch3 != PUZ_SPACE) {
                bool b = ch1 == ch3;
                f(r + 1, c, b, 1);
                f(r + 1, c + 1, b, 3);
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

void puz_state::make_move2(int i, int j)
{

    ++m_distance;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    return m == 2;
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
    //for (int n = 0; n < dt.size(); ++n) {
    //    children.push_back(*this);
    //    if (!children.back().make_move_dot(p, n))
    //        children.pop_back();
    //}
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0;; ++r) {
    //    // draw horz-walls
    //    for (int c = 0; c < sidelen(); ++c)
    //        out << (m_game->m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
    //    out << endl;
    //    if (r == sidelen()) break;
    //    for (int c = 0;; ++c) {
    //        Position p(r, c);
    //        // draw vert-walls
    //        out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
    //        if (c == sidelen()) break;
    //        out << cells(p);
    //    }
    //    out << endl;
    //}
    return out;
}

}

void solve_puz_WildlifePark()
{
    using namespace puzzles::WildlifePark;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WildlifePark.xml", "Puzzles/WildlifePark.txt", solution_format::GOAL_STATE_ONLY);
}
