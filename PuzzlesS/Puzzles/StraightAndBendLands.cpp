#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/Straight and Bend Lands

    Summary
    Our land of curves is better than your straight one!

    Description
    1. This odd nation is divided into two types of regions. One where roads
       always turn on villages, and one where roads always go straight!
    2. Draw a loop that goes through villages (houses), but avoid trees.
    3. While passing on villages, the road might turn or not, but if it turns
       then the road will turn on all villages in that region.
    4. Conversely if it goes straight, all villages of that region will have
       the road go straight through them.
*/

namespace puzzles::StraightAndBendLands{

constexpr auto PUZ_HOUSE = 'O';
constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_STRAIGHT = 0;
constexpr auto PUZ_TURN = 1;

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
const vector<int> linesegs_all_straight = {
    // ─  │ 
    10, 5
};
const vector<int> linesegs_all_turn = {
    // ┐  ┌  ┘  └
    12, 6, 9, 3,
};

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_area
{
    vector<Position> m_rng;
    vector<Position> m_houses;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    set<Position> m_horz_walls, m_vert_walls;
    vector<puz_area> m_areas;
    set<Position> m_trees;
    map<Position, int> m_pos2area;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
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
        if (!walls.contains(p_wall)) {
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
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            rng.insert(p);
            m_cells.push_back(str_v[c * 2 + 1]);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& [rng2, houses] = m_areas.emplace_back();
        rng2 = {smoves.begin(), smoves.end()};
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            switch (cells(p)) {
            case PUZ_TREE: m_trees.insert(p); break;
            case PUZ_HOUSE: houses.push_back(p); break;
            }
        }
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
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
    bool make_move_area(int i, int n);
    void make_move_area2(int i, int n);
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

    const puz_game* m_game = nullptr;
    vector<puz_dot> m_dots;
    map<int, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (g.m_trees.contains(p)) {
                dt = {lineseg_off};
                continue;
            }
            for (int lineseg : linesegs_all)
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

    for (int i = 0; i < g.m_areas.size(); ++i)
        m_matches[i] = {PUZ_STRAIGHT, PUZ_TURN};

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, path_types] : m_matches) {
        auto& [rng, houses] = m_game->m_areas[area_id];

        boost::remove_erase_if(path_types, [&](int n) {
            auto& linesegs_all2 =
                n == PUZ_STRAIGHT ? linesegs_all_straight : linesegs_all_turn;
            return boost::algorithm::any_of(houses, [&](const Position& p) {
                return boost::algorithm::none_of(dots(p), [&](int lineseg) {
                    return boost::algorithm::any_of_equal(linesegs_all2, lineseg);
                });
            });
        });

        if (!init)
            switch(path_types.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_area2(area_id, path_types.front()), 1;
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
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(kv);
        }
        m_distance += newly_finished.size();
    }
}

void puz_state::make_move_area2(int i, int n)
{
    auto& [rng, houses] = m_game->m_areas[i];
    auto& linesegs_all2 =
        n == PUZ_STRAIGHT ? linesegs_all_straight : linesegs_all_turn;
    for (auto& p : houses) {
        auto& dt = dots(p);
        boost::remove_erase_if(dt, [&](int lineseg) {
            return boost::algorithm::none_of_equal(linesegs_all2, lineseg);
        });
    }
    ++m_distance;
    m_matches.erase(i);
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

bool puz_state::make_move_area(int i, int n)
{
    m_distance = 0;
    make_move_area2(i, n);
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
        auto& [area_id, path_types] = *boost::min_element(m_matches, [](
            const pair<int, vector<int>>& kv1,
            const pair<int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : path_types)
            if (children.push_back(*this); !children.back().make_move_area(area_id, n))
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
            if (children.push_back(*this); !children.back().make_move_dot(p, n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; ; ++c) {
            out << ' ';
            if (c == sidelen()) break;
            out << (m_game->m_horz_walls.contains({r, c}) ? "---" : "   ");
        }
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << (is_lineseg_on(dots(p)[0], 0) ? " I " : "   ");
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << (is_lineseg_on(dots(p)[0], 3) ? '=' : ' ');
            out << m_game->cells(p);
            out << (is_lineseg_on(dots(p)[0], 1) ? '=' : ' ');
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << (is_lineseg_on(dots(p)[0], 2) ? " I " : "   ");
        }
        println(out);
    }
    return out;
}

}

void solve_puz_StraightAndBendLands()
{
    using namespace puzzles::StraightAndBendLands;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/StraightAndBendLands.xml", "Puzzles/StraightAndBendLands.txt", solution_format::GOAL_STATE_ONLY);
}
