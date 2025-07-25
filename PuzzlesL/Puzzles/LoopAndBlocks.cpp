#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Loop and Blocks

    Summary
    Don't block me now

    Description
    1. Draw a loop that passes through each clear tile.
    2. The loop must be a single one and can't intersect itself.
    3. A number in a cell shows how many cell must be shaded around its
       four sides.
    4. Not all cells that must be shaded are given with a hint. Two shaded
       cells can't touch orthogonally.
    5. The loop must pass over every cell that hasn't got a number or has
       not been shaded.
*/

namespace puzzles::LoopAndBlocks{

constexpr auto PUZ_NOT_SHADED = ' ';
constexpr auto PUZ_SHADED = 'X';

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

struct puz_area
{
    int m_num;
    vector<Position> m_range;
    int size() const { return m_range.size(); }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, puz_area> m_pos2area;
    map<pair<int, int>, vector<string>> m_info2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_pos2area[{r, c}].m_num = ch - '0';
    }

    for (auto& [p, o] : m_pos2area) {
        int n = o.m_num;
        for (auto& os : offset)
            if (auto p2 = p + os; is_valid(p2))
                o.m_range.push_back(p2);
        int sz = o.size();
        // Find all shaded permutations in relation to a number-size pair
        auto& perms = m_info2perms[{n, sz}];
        if (!perms.empty()) continue;
        // shaded/not shaded permutations
        auto perm = string(sz - n, PUZ_NOT_SHADED) + string(n, PUZ_SHADED);
        do
            perms.push_back(perm);
        while(boost::next_permutation(perm));
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
    bool make_move_hint(const Position& p, int n);
    bool make_move_hint2(const Position& p, int n);
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
    set<Position> m_shaded;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count, {lineseg_off}), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (g.m_pos2area.contains(p))
                continue;

            auto& dt = dots(p);
            for (int lineseg : linesegs_all)
                if ([&]{
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i];
                        // A line segment cannot go beyond the boundaries of the board
                        // or cover any number cell
                        if (!is_valid(p2) || g.m_pos2area.contains(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& [p, o] : g.m_pos2area) {
        for (int i = 0; i < 4; ++i)
            m_finished.emplace(p, i);
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_info2perms.at({o.m_num, o.size()}).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& o = m_game->m_pos2area.at(p);
        auto& perms = m_game->m_info2perms.at({o.m_num, o.size()});

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            for (int k = 0; k < perm.size(); ++k)
                if (perm[k] == PUZ_SHADED) {
                    auto& p2 = o.m_range[k];
                    for (auto& os : offset)
                        if (auto p3 = p2 + os; m_shaded.contains(p3))
                            return true;
                }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, perm_ids[0]) ? 1 : 0;
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

                if (dt.size() == 1 && [&] {
                    for (int i = 0; i < 4; ++i)
                        if (!m_finished.contains({p, i}))
                            return true;
                    return false;
                }() && dt[0] == lineseg_off && !m_shaded.contains(p)) {
                    m_shaded.insert(p);
                    for (int j = 0; j < 4; ++j)
                        if (auto p2 = p + offset[j]; is_valid(p2))
                            if (m_shaded.contains(p2))
                                return 0;
                }
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

bool puz_state::make_move_hint2(const Position& p, int n)
{
    auto& o = m_game->m_pos2area.at(p);
    auto& perm = m_game->m_info2perms.at({o.m_num, o.size()})[n];
    for (int i = 0; i < perm.size(); ++i) {
        auto p2 = o.m_range[i];
        if (perm[i] == PUZ_SHADED) {
            dots(p2) = {lineseg_off};
            m_shaded.insert(p2);
            for (int j = 0; j < 4; ++j)
                if (auto p3 = p2 + offset[j]; is_valid(p3))
                    boost::remove_erase_if(dots(p3), [&](int lineseg3) {
                        return is_lineseg_on(lineseg3, (j + 2) % 4);
                    });
        } else
            boost::remove_erase(dots(p2), lineseg_off);
    }
    ++m_distance;
    m_matches.erase(p);
    return true;
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

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    make_move_hint2(p, n);
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
        auto& [p, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : perm_ids)
            if (children.push_back(*this); !children.back().make_move_hint(p, n))
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
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto it = m_game->m_pos2area.find(p);
            out << char(it != m_game->m_pos2area.end() ? it->second.m_num + '0' :
                m_shaded.contains(p) ? PUZ_SHADED : '.')
                << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_LoopAndBlocks()
{
    using namespace puzzles::LoopAndBlocks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LoopAndBlocks.xml", "Puzzles/LoopAndBlocks.txt", solution_format::GOAL_STATE_ONLY);
}
