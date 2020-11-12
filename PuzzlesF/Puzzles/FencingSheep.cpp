#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 4/Fencing Sheep

    Summary
    Fenceposts, Sheep and obviously wolves

    Description
    1. Divide the field in enclosures, so that sheep are separated from wolves.
    2. Each enclosure must contain either sheep or wolves (but not both) and
       must not be empty.
    3. The lines (fencing) of the enclosures start and end on the edges of the
       grid.
    4. Lines only turn at posts (dots).
    5. Lines can cross each other except posts (dots).
    6. Not all posts must be used.
*/

namespace puzzles::FencingSheep{

#define PUZ_SPACE            ' '
#define PUZ_POST             'O'
#define PUZ_SHEEP            'S'
#define PUZ_WOLF             'W'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<vector<int>> linesegs_all_border = {
    {6}, // „¡
    {12}, // „¢
    {14, 10}, // „¦ „Ÿ
    {3}, // „¤
    {9}, // „£
    {11, 10}, // „¨ „Ÿ
    {7, 5}, // „¥ „ 
    {13, 5}, // „§ „ 
};
const vector<int> linesegs_all_inside = {
    // „© „Ÿ „ 
    0, 15, 10, 5,
};
const vector<int> linesegs_all_post = {
    // „© „¢  „Ÿ  „¡  „£  „   „¤
    15, 12, 10, 6, 9, 5, 3,
};

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // o
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    string m_start;
    set<Position> m_posts;
    set<Position> m_chars_rng;

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
                m_posts.emplace(r, c);
        if (r == m_sidelen - 1) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
            if (ch != PUZ_SPACE)
                m_chars_rng.emplace(r, c);
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
    bool make_move(const Position& p, int n);
    int check_dots(bool init);
    bool is_connected() const;
    void check_connected();

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
            if (r > 0 && c > 0 && r < sidelen() - 1 && c < sidelen() - 1)
                dt = g.m_posts.count(p) != 0 ? linesegs_all_post : linesegs_all_inside;
            else {
                int n =
                    r == 0 ? (c == 0 ? 0 : c == sidelen() - 1 ? 1 : 2) :
                    r == sidelen() - 1 ? (c == 0 ? 3 : c == sidelen() - 1 ? 4 : 5) :
                    c == 0 ? 6 : 7;
                dt = linesegs_all_border[n];
            }
        }

    check_dots(true);
}

struct puz_state3 : Position
{
    puz_state3(puz_state& state, char ch, const Position& p_start) : m_state(&state), m_ch(ch) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    puz_state* m_state;
    const char m_ch;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p_dots = *this + offset2[i];
        int d = (i + 1) % 4;
        auto& dts = m_state->dots(p_dots);
        if (boost::algorithm::all_of(dts, [&](int dt) {
            return is_lineseg_on(dt, d);
        }))
            continue;
        auto p = *this + offset[i];
        char ch = m_state->m_game->cells(p);
        if (ch != PUZ_SPACE && ch != m_ch)
            boost::remove_erase_if(dts, [=](int lineseg) {
                return !is_lineseg_on(lineseg, d);
            });
        else if (boost::algorithm::all_of(dts, [&](int dt) {
            return !is_lineseg_on(dt, d);
        })) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

void puz_state::check_connected()
{
    auto rng = m_game->m_chars_rng;
    while (!rng.empty()) {
        list<puz_state3> smoves;
        auto& p = *rng.begin();
        puz_move_generator<puz_state3>::gen_moves({ *this, m_game->cells(p), p }, smoves);
        for (auto& p : smoves)
            rng.erase(p);
    }
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        check_connected();

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

struct puz_state2 : Position
{
    puz_state2(const puz_state& state, char ch, const Position& p_start) : m_state(&state), m_ch(ch) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const char m_ch;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p_dots = *this + offset2[i];
        int d = (i + 1) % 4;
        if (boost::algorithm::all_of(m_state->dots(p_dots), [&](int dt) {
            return is_lineseg_on(dt, d);
        }))
            continue;
        auto p = *this + offset[i];
        char ch = m_state->m_game->cells(p);
        if (ch == PUZ_SPACE || ch == m_ch) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::is_connected() const
{
    set<Position> rng_all;
    auto rng = m_game->m_chars_rng;
    while (!rng.empty()) {
        list<puz_state2> smoves;
        auto& p = *rng.begin();
        puz_move_generator<puz_state2>::gen_moves({ *this, m_game->cells(p), p }, smoves);
        for (auto& p : smoves) {
            rng.erase(p);
            rng_all.insert(p);
        }
    }
    return rng_all.size() == (sidelen() - 1) * (sidelen() - 1);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 1;
    dots(p) = { dots(p)[n] };
    int m = check_dots(false);
    return m != 0 && is_connected();
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
    int sz = dt.size();
    if (sz == 1) return;
    Position p(i / sidelen(), i % sidelen());
    for (int n = 0; n < sz; ++n) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0;; ++c) {
            Position p(r, c);
            out << (m_game->m_posts.count(p) != 0 ? PUZ_POST : ' ');
            if (c == sidelen() - 1) break; 
            out << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
        }
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-lines
            out << (is_lineseg_on(dots(p)[0], 2) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << m_game->cells(p);
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_FencingSheep()
{
    using namespace puzzles::FencingSheep;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FencingSheep.xml", "Puzzles/FencingSheep.txt", solution_format::GOAL_STATE_ONLY);
}
