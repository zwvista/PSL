#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 7/Loop and Blocks

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

namespace puzzles{ namespace LoopAndBlocks{

#define PUZ_LINE_OFF        '0'
#define PUZ_LINE_ON            '1'

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
    {-1, 1},        // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},    // nw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, int> m_pos2num;
    map<int, vector<vector<int>>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            if (ch != ' ') {
                int n = ch - '0';
                m_num2perms[n];
                m_pos2num[{r, c}] = n;
            }
        }
    }

    for (auto& kv : m_num2perms) {
        int n = kv.first;
        // Find all line segment permutations in relation to a number
        auto& perms = kv.second;
        // pass/no pass permutations
        // PUZ_LINE_OFF means an adjacent cell is not passed by the line
        // PUZ_LINE_ON means an adjacent cell is passed by the line
        auto indicator = string(8 - n, PUZ_LINE_OFF) + string(n, PUZ_LINE_ON);
        do {
            vector<vector<int>> dir2linesegs(8);
            for (int i = 0; i < 8; ++i)
                // Find all line permutations from an adjacent cell
                if (indicator[i] == PUZ_LINE_OFF)
                    // This adjacent cell is not passed by the line
                    dir2linesegs[i] = {lineseg_off};
                else
                    for (int lineseg : linesegs_all)
                        if ([&]{
                            for (int j = 0; j < 4; ++j) {
                                if (!is_lineseg_on(lineseg, j))
                                    continue;
                                // compute the position that the line segment in an adjacent cell leads to
                                auto p = offset[i] + offset[2 * j];
                                // The line segment from an adjacent cell cannot lead to the number cell
                                // or any adjacent cell not covered by the line
                                if (p == Position(0, 0))
                                    return false;
                                int n = boost::find(offset, p) - offset;
                                if (n < 8 && indicator[n] == PUZ_LINE_OFF)
                                    return false;
                            }
                            return true;
                        }())
                            // This line segment permutation is possible for the adjacent cell
                            dir2linesegs[i].push_back(lineseg);

            // No line segment permutation from an adjacent cell means
            // this pass/no pass permutation is impossible
            if (boost::algorithm::any_of(dir2linesegs, [](const vector<int>& linesegs) {
                return linesegs.empty();
            }))
                continue;

            // Find all line permutations for this pass/no pass permutation
            vector<int> indexes(8);
            vector<int> perm(8);
            for (int i = 0; i < 8;) {
                for (int j = 0; j < 8; ++j)
                    perm[j] = dir2linesegs[j][indexes[j]];
                if ([&]{
                    for (int j = 0; j < 8; ++j) {
                        int lineseg = perm[j];
                        for (int k = 0; k < 4; ++k) {
                            if (!is_lineseg_on(lineseg, k))
                                continue;
                            auto p = offset[j] + offset[2 * k];
                            int n = boost::find(offset, p) - offset;
                            // If the line segment from an adjacent cell leads to another adjacent cell,
                            // the line segment from the latter should also lead to the former
                            if (n < 8 && !is_lineseg_on(perm[n], (k + 2) % 4))
                                return false;
                        }
                    }
                    return true;
                }())
                    perms.push_back(perm);
                for (i = 0; i < 8 && ++indexes[i] == dir2linesegs[i].size(); ++i)
                    indexes[i] = 0;
            }

        } while(boost::next_permutation(indicator));
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
    bool make_move_hint(const Position& p, int n);
    void make_move_hint2(const Position& p, int n);
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
    map<Position, vector<int>> m_matches;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count, {lineseg_off}), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (g.m_pos2num.count(p) != 0)
                continue;

            auto& dt = dots(p);
            for (int lineseg : linesegs_all)
                if ([&]{
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i * 2];
                        // The line segment cannot lead to a position
                        // outside the board or cover any number cell
                        if (!is_valid(p2) || g.m_pos2num.count(p2) != 0)
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& kv : g.m_pos2num) {
        auto& p = kv.first;
        m_finished.insert(p);
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_num2perms.at(kv.second).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            for (int i = 0; i < 8; ++i) {
                auto p2 = p + offset[i];
                int lineseg = perm[i];
                if (!is_valid(p2)) {
                    if (lineseg != lineseg_off)
                        return true;
                } else if (boost::algorithm::none_of_equal(dots(p2), lineseg))
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, perm_ids.front()), 1;
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
                auto p2 = p + offset[i * 2];
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

void puz_state::make_move_hint2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    for (int i = 0; i < 8; ++i) {
        auto p2 = p + offset[i];
        if (is_valid(p2))
            dots(p2) = {perm[i]};
    }
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
                    p2 += offset[(n = i) * 2];
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
        auto& kv = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : kv.second) {
            children.push_back(*this);
            if (!children.back().make_move_hint(kv.first, n))
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
            auto it = m_game->m_pos2num.find(p);
            out << char(it == m_game->m_pos2num.end() ? ' ' : it->second + '0')
                << (is_lineseg_on(dots(p)[0], 1) ? '-' : ' ');
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

void solve_puz_LoopAndBlocks()
{
    using namespace puzzles::LoopAndBlocks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LoopAndBlocks.xml", "Puzzles/LoopAndBlocks.txt", solution_format::GOAL_STATE_ONLY);
}
