#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Culture Trip

    Summary
    Or how to make a culture trip complicated

    Description
    1. The board represents a City of Art and Culture, divided in neighborhoods.
    2. During a Culture Trip, in order to make everybody happy, you devise a
       convoluted method to visit a city:
    3. All neighborhoods must be visited exactly and only once.
    4. You have to set foot in a neighborhood only once and can't come back after
       you leave it.
    5. In a neighborhood, you either visit All Museums or All Monuments.
    6. If you visit Monuments, you can't pass over Museums and vice-versa.
    7. You have to alternate between neighborhoods where you visit Museums and
       those where you visit Monuments.
    8. After visiting Museums, you should visit Monuments, then Museums again, etc.
    9. The Trip must form a closed loop, in the end returning to the starting
       neighborhood.
*/

namespace puzzles{ namespace CultureTrip{

#define PUZ_MUSEUM          'M'
#define PUZ_MONUMENT        'T'
#define PUZ_SPACE           ' '
#define PUZ_VISIT_MUSEUM    0
#define PUZ_VISIT_MONUMENT  1

    // n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
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
    set<Position> m_horz_walls, m_vert_walls;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return (*this)[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (walls.count(p_wall) == 0) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2)
    , m_dot_count(m_sidelen * m_sidelen)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            rng.insert(p);
            m_start.push_back(str_v[c * 2 + 1]);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
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
    bool make_move_area(int i, int n);
    void make_move_area2(int i, int n);
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
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

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
                if ([&]{
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i];
                        // The line segment cannot lead to a position
                        // outside the board
                        if (!is_valid(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (int i = 0; i < g.m_areas.size(); ++i)
        m_matches[i] = {PUZ_VISIT_MUSEUM, PUZ_VISIT_MONUMENT};

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& visit_ids = kv.second;

        boost::remove_erase_if(visit_ids, [&](int id) {
            return boost::algorithm::any_of(m_game->m_areas[id], [&](const Position& p) {
                char ch = m_game->cells(p);
                if (ch == PUZ_SPACE) return false;
                auto& dt = dots(p);
                bool b1 = ch == PUZ_MUSEUM, b2 = id == PUZ_VISIT_MUSEUM;
                return b1 == b2 && (dt[0] != lineseg_off || dt.size() > 1) ||
                    b1 != b2 && dt[0] == lineseg_off;
            });
        });

        if (!init)
            switch(visit_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_area2(area_id, visit_ids.front()), 1;
            }
    }
    return 2;
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

void puz_state::make_move_area2(int i, int n)
{
    //auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    //for (int i = 0; i < 4; ++i) {
    //    auto p2 = p + offset[i];
    //    if (is_valid(p2))
    //        dots(p2) = {perm[i]};
    //}
    m_matches.erase(p);
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

bool puz_state::make_move_area(int i, int n)
{
    m_distance = 0;
    make_move_area2(p, n);
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
        auto& kv = *boost::min_element(m_matches, [](
            const pair<int, vector<int>>& kv1,
            const pair<int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : kv.second) {
            children.push_back(*this);
            if (!children.back().make_move_area(kv.first, n))
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
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            //auto it = m_game->m_pos2num.find(p);
            //out << char(it == m_game->m_pos2num.end() ? ' ' : it->second + '0')
            //    << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
        }
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vert-lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        out << endl;
    }
    return out;
}

}}

void solve_puz_CultureTrip()
{
    using namespace puzzles::CultureTrip;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CultureTrip.xml", "Puzzles/CultureTrip.txt", solution_format::GOAL_STATE_ONLY);
}
