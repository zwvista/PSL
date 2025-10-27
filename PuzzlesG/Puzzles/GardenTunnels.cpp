#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 4/Garden Tunnels

    Summary
    Whack a mole

    Description
    1. the board represents a few gardens where some moles are digging
       straight line tunnels.
    2. Each tunnel starts in the garden and ends in a different garden,
       and can pass through other gardens.
    3. The number in the garden tells you how many tunnels start/end in that
       garden.
    4. The entire board must be filled with tunnels.
*/

namespace puzzles::GardenTunnels{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_UNKNOWN = -1;
constexpr auto PUZ_START_END = '0';
constexpr auto PUZ_MIDDLE = '1';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<int> linesegs_all = {
    // ╵(up) ╶(right) ╷(down) ╴(left) ─ │ 
    1, 2, 4, 8, 10, 5,
};
// All line segments for a house
const vector<int> linesegs_all_start_end = {
    // ╵(up) ╶(right) ╷(down) ╴(left)
    1, 2, 4, 8,
};
const vector<int> linesegs_all_middle = {
    // ─ │ 
    10, 5,
};

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, int> m_pos2num;
    vector<pair<int, vector<Position>>> m_areas;
    map<Position, int> m_pos2area;
    map<pair<int, int>, vector<string>> m_config2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
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
            children.emplace_back(*this).make_move(p);
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
            if (char ch = str_v[c * 2 + 1]; ch != ' ')
                m_pos2num[p] = ch - '0';
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        int num = PUZ_UNKNOWN;
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                num = it->second;
        }
        m_areas.push_back({num, {smoves.begin(), smoves.end()}});

        int sz = smoves.size();
        auto& perms = m_config2perms[{num, sz}];
        if (!perms.empty()) continue;
        for (int i = 0; i <= sz; i++) {
            if (num != PUZ_UNKNOWN && num != i) continue;
            // 3. The number in the garden tells you how many tunnels start/end in that
            // garden.
            auto perm = string(i, PUZ_START_END) + string(sz - i, PUZ_MIDDLE);
            do
                perms.push_back(perm);
            while (boost::next_permutation(perm));
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
    bool operator<(const puz_state& x) const { return m_dots < x.m_dots; }
    bool make_move_hint(int i, int n);
    void make_move_hint2(int i, int n);
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

    const puz_game* m_game;
    vector<puz_dot> m_dots;
    map<int, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        auto& [num, rng] = g.m_areas[i];
        vector<int> perm_ids(g.m_config2perms.at({num, rng.size()}).size());
        boost::iota(perm_ids, 0);
        m_matches[i] = perm_ids;
    }

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

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& [num, rng] = m_game->m_areas[area_id];
        auto& perms = m_game->m_config2perms.at({num, rng.size()});
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            for (int k = 0; k < rng.size(); ++k) {
                char ch = perm[k];
                auto& p = rng[k];
                auto& linesegs_all2 =
                    ch == PUZ_START_END ? linesegs_all_middle : linesegs_all_start_end;
                if (boost::algorithm::all_of(dots(p), [&](int lineseg) {
                    return boost::algorithm::any_of_equal(linesegs_all2, lineseg);
                }))
                    return true;
            }
            return false;
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(area_id, perm_ids.front()), 1;
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

void puz_state::make_move_hint2(int i, int n)
{
    auto& [num, rng] = m_game->m_areas[i];
    auto& perm = m_game->m_config2perms.at({num, rng.size()})[n];
    for (int k = 0; k < rng.size(); ++k) {
        char ch = perm[k];
        auto& p = rng[k];
        auto& linesegs_all2 =
            ch == PUZ_START_END ? linesegs_all_middle : linesegs_all_start_end;
        for (int lineseg : linesegs_all2)
            boost::remove_erase(dots(p), lineseg);
    }
    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move_hint(int i, int n)
{
    m_distance = 0;
    make_move_hint2(i, n);
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

bool puz_state::check_loop() const
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (dt.size() == 1)
                rng.insert(p);
        }
    
    while (!rng.empty()) {
        auto p = *rng.begin(), p2 = p;
        set<int> visited_area_ids;
        int start_end_count = 0;
        for (int n = -1;;) {
            rng.erase(p2);
            visited_area_ids.insert(m_game->m_pos2area.at(p2));
            auto& lineseg = dots(p2)[0];
            if (boost::algorithm::any_of_equal(linesegs_all_start_end, lineseg))
                ++start_end_count;
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (!rng.contains(p2))
                break;
        }
        // 2. Each tunnel starts in the garden and ends in a different garden,
        // and can pass through other gardens.
        if (start_end_count == 2 && visited_area_ids.size() == 1)
            return false;
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
    if (!m_matches.empty()) {
        auto& [i, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const int, vector<int>>& kv1,
            const pair<const int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : perm_ids)
            if (!children.emplace_back(*this).make_move_hint(i, n))
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
        Position p(i / sidelen(), i% sidelen());
        for (int n = 0; n < dt.size(); ++n)
            if (!children.emplace_back(*this).make_move_dot(p, n))
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
            Position p(r, c);
            out << (m_game->m_horz_walls.contains(p) ?
                r < sidelen() && is_lineseg_on(dots(p)[0], 0) ? "-+-" : "---" :
                r < sidelen() && is_lineseg_on(dots(p)[0], 0) ? " I " : "   ");
        }
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2num.find(p); it != m_game->m_pos2num.end())
                out << it->second;
            else
                out << ' ';
            out << (is_lineseg_on(dots(p)[0], 0) ? "I " : "  ");
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ?
                c < sidelen() && is_lineseg_on(dots(p)[0], 3) ? '+' : '|' :
                c < sidelen() && is_lineseg_on(dots(p)[0], 3) ? '=' : ' ');
            if (c == sidelen()) break;
            out << (is_lineseg_on(dots(p)[0], 3) ? '=' : ' ');
            out << (is_lineseg_on(dots(p)[0], 3) || is_lineseg_on(dots(p)[0], 1) ? '=' :
                is_lineseg_on(dots(p)[0], 0) || is_lineseg_on(dots(p)[0], 2) ? 'I' : ' ');
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

void solve_puz_GardenTunnels()
{
    using namespace puzzles::GardenTunnels;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/GardenTunnels.xml", "Puzzles/GardenTunnels.txt", solution_format::GOAL_STATE_ONLY);
}
