#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
#include <boost/math/special_functions/sign.hpp>

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Path on the Mountains

    Summary
    Turn on the peak, turn on the plain

    Description
    1. Fill the board with a loop that passes through all tiles.
    2. The path should make 90 degrees turns on the spots.
    3. Between spots, the path makes one more 90 degrees turn.
    4. So the path alternates turning on spots and outside them.
*/

namespace puzzles::PathOnTheMountains{

using boost::math::sign;

constexpr auto PUZ_SPOT = 'O';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
// All line segments for a spot
// 2. The path should make 90 degrees turns on the spots.
const vector<int> linesegs_all_spot = {
    // └  ┌  ┐  ┘
    3, 6, 12, 9,
};

const vector<vector<vector<int>>> linesegs_all_path = {
    {
        {9, 9, 12, 12, 6, 6, 12, 12},
        {6, 6, 6, 6, 9, 9, 9, 9},
        {3, 9, 3, 9, 3, 6, 3, 6},
    },
    {
        {3, 3, 6, 6, 6, 6, 12, 12},
        {12, 12, 12, 12, 3, 3, 3, 3},
        {3, 9, 3, 9, 9, 12, 9, 12},
    },
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_path
{
    Position m_p_start, m_p_end;
    vector<Position> m_rng;
    vector<int> m_line;
};

struct puz_game
{
    string m_id;
    Position m_size;
    int m_dot_count;
    set<Position> m_spots;
    vector<puz_path> m_paths;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
    , m_dot_count(rows() * cols())
{
    for (int r = 0; r < rows(); ++r) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); ++c)
            if (char ch = str[c];  ch != ' ')
                m_spots.emplace(r, c);
    }
    Position os0;
    for (auto& p1 : m_spots)
        for (auto& p2 : m_spots) {
            auto& [r1, c1] = p1;
            auto& [r2, c2] = p2;
            // only make a pairing with a tile in the bottom left or bottom right
            if (r2 <= r1 || c2 == c1) continue;
            Position os1(1, 0);
            Position os2(0, sign(c2 - c1));
            auto& linesegs_path = linesegs_all_path[c2 < c1 ? 0 : 1];
            for (int k = 0; k < 8; ++k) {
                // bottom left, k = 0,1,2,3: ┌── p3(r1, c2)
                // ┐  ┘ -> ─ -> ┌ -> │ ->  └  ┘
                // [9, 12] -> 10 -> 6 -> 5 -> [3, 9]
                // bottom left, k = 4,5,6,7: ──┘ p3(r2, c1)
                // ┌  ┐ -> │ -> ┘ -> ─ ->  └  ┌
                // [6, 12] -> 5 -> 9 -> 10 -> [3, 6]
                // bottom right, k = 0,1,2,3: ──┐ p3(r1, c2)
                // └  ┌ -> ─ -> ┐ -> │ ->  └  ┘
                // [3, 6] -> 10 -> 12 -> 5 -> [3, 9]
                // bottom right, k = 4,5,6,7: └── p3(r2, c1) 
                // ┌  ┐ -> │ -> └ -> ─ ->  ┐  ┘
                // [6, 12] -> 5 -> 3 -> 10 -> [9, 12]
                Position p3(k < 4 ? r1 : r2, k < 4 ? c2 : c1);
                vector<Position> rng;
                vector<int> line;
                for (auto p = p1;;
                    p += (p == p3 ? k < 4 ? os1 : os2 : p.first == p3.first ? os2 : os1)) {
                    rng.push_back(p);
                    line.push_back(
                        p == p1 ? linesegs_path[0][k] :
                        p == p3 ? linesegs_path[1][k] :
                        p == p2 ? linesegs_path[2][k] :
                        p.first == p3.first ? 10 : 5
                    );
                    if (p == p2) break;
                    if (p != p1 && m_spots.contains(p)) goto next_k;
                }
                m_paths.emplace_back(p1, p2, rng, line);
            next_k:;
            }
        }
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
        return tie(m_dots, m_matches) < tie(x.m_dots, x.m_matches); 
    }
    bool make_move(int n);
    bool make_move2(int n);
    int find_matches(bool init);
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
    vector<puz_dot> m_dots;
    // key: the position of the dot
    // value: the index of the permutation
    map<Position, vector<int>> m_matches;
    map<Position, int> m_pos2count;
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
            auto& linesegs_all2 =
                g.m_spots.contains(p) ? linesegs_all_spot : linesegs_all;
            for (int lineseg : linesegs_all2)
                if ([&]{
                    for (int i = 0; i < 4; ++i)
                        // A line segment cannot go beyond the boundaries of the board
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (int i = 0; i < g.m_paths.size(); ++i) {
        auto& [p1, p2, rng, line] = g.m_paths[i];
        m_matches[p1].push_back(i), m_matches[p2].push_back(i);
    }
    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, path_ids] : m_matches) {
        boost::remove_erase_if(path_ids, [&](int id) {
            auto& [p1, p2, rng, line] = m_game->m_paths[id];
            for (int i = 0; i < rng.size(); ++i)
                if (boost::algorithm::none_of_equal(dots(rng[i]), line[i]))
                    return true;
            return false;
        });

        if (!init)
            switch(path_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(path_ids.front()) ? 1 : 0;
            }
    }
    return 2;
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
                        newly_finished.insert({p, i});
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

bool puz_state::make_move2(int n)
{
    auto& [p1, p2, rng, line] = m_game->m_paths[n];
    for (int i = 0; i < rng.size(); ++i)
        dots(rng[i]) = {line[i]};

    auto f = [&](int n2) {
        for (auto& [p3, path_ids] : m_matches)
            boost::remove_erase(path_ids, n2);
    };

    f(n);
    for (auto& p3 : {p1, p2})
        if (++m_pos2count[p3] == 2) {
            for (int n2 : m_matches.at(p3))
                f(n2);
            m_matches.erase(p3);
        }

    return check_loop();
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    if (!make_move2(n))
        return false;
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
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (dt.size() == 1)
                rng.insert(p);
        }

    bool has_branch = false;
    while (!rng.empty()) {
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            int lineseg = dots(p2)[0];
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

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, path_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

    for (int n : path_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            out << (m_game->m_spots.contains(p) ? PUZ_SPOT : ' ')
                << (is_lineseg_on(dt[0], 1) ? '-' : ' ');
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

void solve_puz_PathOnTheMountains()
{
    using namespace puzzles::PathOnTheMountains;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PathOnTheMountains.xml", "Puzzles/PathOnTheMountains.txt", solution_format::GOAL_STATE_ONLY);
}
