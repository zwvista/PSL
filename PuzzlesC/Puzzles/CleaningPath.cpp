#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 1/Cleaning Path

    Summary
    Puzzles for Roombas

    Description
    1. You are a Roomba! And this is office floor you have to clean tonight.
    2. The floor is divided in rooms. You can enter (and exit) the room only once.
    3. Follow a path that allows you to clean all the tiles on the floor.
*/

namespace puzzles::CleaningPath{

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
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
    Position m_size;
    int m_dot_count;
    set<Position> m_horz_walls, m_vert_walls;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
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
    , m_size(strs.size() / 2, strs[0].length() / 2)
    , m_dot_count(rows() * cols())
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < cols(); ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == rows()) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == cols()) break;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
        m_areas.emplace_back(smoves.begin(), smoves.end());
    }
}

using puz_dot = vector<int>;

struct puz_state : vector<puz_dot>
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    const puz_dot& dots(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
    puz_dot& dots(const Position& p) { return (*this)[p.first * cols() + p.second]; }
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
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            int area_id = m_game->m_pos2area.at(p);
            bool has_single_cell = m_game->m_areas[area_id].size() == 1;
            for (int lineseg : linesegs_all)
                if ([&]{
                    set<int> area_ids;
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i];
                        // A line segment cannot go beyond the boundaries of the board
                        if (!is_valid(p2))
                            return false;
                        area_ids.insert(m_game->m_pos2area.at(p2));
                    }
                    // 2. You can enter (and exit) the room only once.
                    return has_single_cell || area_ids.contains(area_id);
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
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (dt.size() == 1)
                rng.insert(p);
        }

    // 2. You can enter (and exit) the room only once.
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
    Position p(i / cols(), i % cols());
    for (int n = 0; n < dt.size(); ++n) {
        children.push_back(*this);
        if (!children.back().make_move_dot(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; ; ++c) {
            out << ' ';
            if (c == cols()) break;
            out << (m_game->m_horz_walls.contains({r, c}) ? "---" : "   ");
        }
        println(out);
        if (r == rows()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            out << (is_lineseg_on(dots(p)[0], 0) ? " I " : "   ");
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            out << (is_lineseg_on(dots(p)[0], 3) ? '=' : ' ');
            out << ' ';
            out << (is_lineseg_on(dots(p)[0], 1) ? '=' : ' ');
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            out << (is_lineseg_on(dots(p)[0], 2) ? " I " : "   ");
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CleaningPath()
{
    using namespace puzzles::CleaningPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CleaningPath.xml", "Puzzles/CleaningPath.txt", solution_format::GOAL_STATE_ONLY);
}
