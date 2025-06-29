#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 1/Culture Trip

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

namespace puzzles::CultureTrip{

constexpr auto PUZ_MUSEUM = 'M';
constexpr auto PUZ_MONUMENT = 'T';
constexpr auto PUZ_SPACE = ' ';

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

constexpr Position offset2[] = {
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
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
        m_areas.emplace_back(smoves.begin(), smoves.end());
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
    bool make_move_area(int i, char ch2);
    void make_move_area2(int i, char ch2);
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
    map<int, vector<char>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count, {lineseg_off}), m_game(&g)
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
                        // A line segment cannot go beyond the boundaries of the board
                        if (!is_valid(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (int i = 0; i < g.m_areas.size(); ++i)
        m_matches[i] = {PUZ_MUSEUM, PUZ_MONUMENT};

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, visit_chars] : m_matches) {
        auto& area = m_game->m_areas[area_id];

        boost::remove_erase_if(visit_chars, [&](char ch2) {
            return boost::algorithm::any_of(area, [&](const Position& p) {
                char ch = m_game->cells(p);
                if (ch == PUZ_SPACE) return false;
                auto& dt = dots(p);
                return ch == ch2 && dt[0] == lineseg_off && dt.size() == 1 ||
                    ch != ch2 && dt[0] != lineseg_off;
            });
        });

        if (!init)
            switch(visit_chars.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_area2(area_id, visit_chars.front()), 1;
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

void puz_state::make_move_area2(int i, char ch2)
{
    auto& area = m_game->m_areas[i];
    for (auto& p : area) {
        char ch = m_game->cells(p);
        if (ch == PUZ_SPACE) continue;
        auto& dt = dots(p);
        if (ch == ch2)
            boost::remove_erase(dt, lineseg_off);
        else
            dt = {lineseg_off};
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

    // 3. All neighborhoods must be visited exactly and only once.
    // 4. You have to set foot in a neighborhood only once and can't come back after
    // you leave it.
    map<int, int> area2num;
    for (auto& p : rng) {
        int area_id = m_game->m_pos2area.at(p);
        int lineseg = dots(p)[0];
        for (int i = 0; i < 4; ++i)
            if (is_lineseg_on(lineseg, i)) {
                int area_id2 = m_game->m_pos2area.at(p + offset[i]);
                if (area_id != area_id2)
                    area2num[area_id]++;
            }
    }
    if (is_goal_state() && area2num.size() != m_game->m_areas.size())
        return false;
    for (auto& [id, num] : area2num)
        if (is_goal_state() && num != 2 || num > 2)
            return false;

    bool has_branch = false;
    while (!rng.empty()) {
        auto p = *rng.begin(), p2 = p;
        set<int> area_ids;
        char first_visit_char = PUZ_SPACE, last_visit_char = PUZ_SPACE;
        for (int n = -1;;) {
            rng.erase(p2);
            auto& lineseg = dots(p2)[0];
            char ch = m_game->cells(p2);
            if (ch != PUZ_SPACE) {
                int area_id = m_game->m_pos2area.at(p2);
                if (!area_ids.contains(area_id)) {
                    area_ids.insert(area_id);
                    if (first_visit_char == PUZ_SPACE)
                        first_visit_char = ch;
                    if (last_visit_char == ch)
                        return false;
                    last_visit_char = ch;
                }
            }
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // and we are supposed to have exhausted the line segments
                return !has_branch && rng.empty() && last_visit_char != first_visit_char;
            if (!rng.contains(p2)) {
                has_branch = true;
                break;
            }
        }
    }
    return true;
}

bool puz_state::make_move_area(int i, char ch2)
{
    m_distance = 0;
    make_move_area2(i, ch2);
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
        auto& [area_id, visit_chars] = *boost::min_element(m_matches, [](
            const pair<int, vector<char>>& kv1,
            const pair<int, vector<char>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int ch2 : visit_chars) {
            children.push_back(*this);
            if (!children.back().make_move_area(area_id, ch2))
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

void solve_puz_CultureTrip()
{
    using namespace puzzles::CultureTrip;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CultureTrip.xml", "Puzzles/CultureTrip.txt", solution_format::GOAL_STATE_ONLY);
}
