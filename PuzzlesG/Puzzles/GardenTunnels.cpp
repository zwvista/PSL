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

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<int> linesegs_all = {
    // ╵(up) ╶(right) ╷(down) ╴(left) ─ │ 
    1, 2, 4, 8, 10, 5,
};
const vector<int> linesegs_all_start_end = {
    // ╵(up) ╶(right) ╷(down) ╴(left)
    1, 2, 4, 8,
};
const vector<int> linesegs_horz = {
    // ╶(right), ─, ╴(left)
    2, 10, 8
};
const vector<int> linesegs_vert = {
    // ╷(down), │ , ╵(up)
    4, 5, 1
};

constexpr auto offset = Position::Directions4;
constexpr auto offset2 = Position::WallsOffset4;

struct puz_move
{
    vector<Position> m_rng;
    bool m_is_vert;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, int> m_pos2num;
    // elem.first: area size
    // elem.second: positions
    vector<pair<int, vector<Position>>> m_areas;
    map<Position, int> m_pos2area;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : Position
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
        if (!walls.contains(p_wall))
            children.emplace_back(*this).make_move(p);
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
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        int num = PUZ_UNKNOWN;
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                num = it->second;
        }
        m_areas.push_back({num, {smoves.begin(), smoves.end()}});
    }

    // 2. Each tunnel starts in the garden and ends in a different garden,
    //    and can pass through other gardens.
    // 4. The entire board must be filled with tunnels.
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (Position p(r, c); int d : {1, 2}) {
                set area_ids{m_pos2area.at(p)};
                vector rng{p};
                bool is_vert = d == 2;
                auto& os = offset[d];
                for (auto p2 = p + os; is_valid(p2); p2 += os) {
                    area_ids.insert(m_pos2area.at(p2));
                    rng.push_back(p2);
                    if (area_ids.size() > 1) {
                        int n = m_moves.size();
                        m_moves.emplace_back(rng, is_vert);
                        for (auto& p3 : rng)
                            m_pos2move_ids[p3].push_back(n);
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
    bool operator<(const puz_state& x) const { return m_dots < x.m_dots; }
    bool make_move(int n);
    void make_move2(int n);
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
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
, m_matches(g.m_pos2move_ids)
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

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [rng, is_vert] = m_game->m_moves[id];
            auto& linesegs = !is_vert ? linesegs_horz : linesegs_vert;
            for (int i = 0, sz = rng.size(); i < sz; ++i)
                if (boost::algorithm::none_of_equal(dots(rng[i]), linesegs[i == 0 ? 0 : i == sz - 1 ? 2 : 1]))
                    return true;
            return false;
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
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
                if (!init && dt2.empty())
                    return 0;
            }
            m_finished.insert(kv);
        }
        m_distance += newly_finished.size();
    }
}

void puz_state::make_move2(int n)
{
    auto& [rng, is_vert] = m_game->m_moves[n];
    auto& linesegs = !is_vert ? linesegs_horz : linesegs_vert;
    for (int i = 0, sz = rng.size(); i < sz; ++i) {
        auto& p = rng[i];
        dots(p) = {linesegs[i == 0 ? 0 : i == sz - 1 ? 2 : 1]};
        ++m_distance, m_matches.erase(p);
    }
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
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
    // 3. The number in the garden tells you how many tunnels start/end in that
    //    garden.
    auto f = [&](int lineseg) {
        return boost::algorithm::any_of_equal(linesegs_all_start_end, lineseg);
    };
    for (auto& [num, rng] : m_game->m_areas) {
        if (num == PUZ_UNKNOWN) continue;
        int max_possible = 0;
        int min_guaranteed = 0;

        for (const auto& p : rng) {
            const auto& dt = dots(p);
            bool can_be_start_end = boost::algorithm::any_of(dt, f);
            bool must_be_start_end = boost::algorithm::all_of(dt, f);

            if (can_be_start_end) max_possible++;
            if (must_be_start_end) min_guaranteed++;
        }

        // Prune if we can't possibly reach the target
        if (max_possible < num) return false;
        // Prune if we have already exceeded the target
        if (min_guaranteed > num) return false;
        // Final check
        if (is_goal_state() && min_guaranteed != num) return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
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
