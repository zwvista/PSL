#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 3/Straight and Turn

    Summary
    Straight and Turn

    Description
    1. Draw a path that crosses all gems and follows this rule:
    2. Crossing two adjacent gems:
    3. The line cannot cross two adjacent gems if they are of different color.
    4. The line is free to either go straight or turn when crossing two
       adjacent gems of the same color.
    5. Crossing a gem that is not adjacent to the last crossed:
    6. The line should go straight in the space between two gems of the same
       colour.
    7. The line should make a single 90 degree turn in the space between
       two gems of different colour.
*/

namespace puzzles::StraightAndTurn{

using boost::math::sign;

#define PUZ_BLACK_GEM        'B'
#define PUZ_WHITE_GEM        'W'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
// All line segments for a Black Pearl
// Lines passing through Black Pearls must do a 90 degree turn in them.
const vector<int> linesegs_all_black = {
    // └  ┌  ┐  ┘
    3, 6, 12, 9,
};
// All line segments for a White Pearl
// Lines passing through White Pearls must go straight through them.
const vector<int> linesegs_all_white = {
    // │  ─
    5, 10,
};

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

// first: the offset of the line segment
// second: an integer that depicts the line segment
typedef pair<Position, int> puz_lineseg_info;

// permutations of line segments for a black pearl and its two adjacent cells
// they must go straight in the next tile in both directions.
const puz_lineseg_info black_pearl_perms[][3] = {
    // │  ┌─ ─┐  │
    // └─ │   │ ─┘
    {{{0, 0}, 3}, {{-1, 0}, 5}, {{0, 1}, 10}},        // n & e
    {{{0, 0}, 6}, {{0, 1}, 10}, {{1, 0}, 5}},        // e & s
    {{{0, 0}, 12}, {{1, 0}, 5}, {{0, -1}, 10}},        // s & w
    {{{0, 0}, 9}, {{0, -1}, 10}, {{-1, 0}, 5}},    // w & n
};
// permutations of line segments for a white pearl and its two adjacent cells
// at least at one side of the White Pearl(or both), they must do a 90 degree turn.
const puz_lineseg_info white_pearl_perms[][3] = {
    // │ │ ┌ ┌ ┌ ┐ ┐ ┐
    // │ │ │ │ │ │ │ │
    // └ ┘ │ └ ┘ │ └ ┘
    {{{0, 0}, 5}, {{-1, 0}, 5}, {{1, 0}, 3}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 5}, {{1, 0}, 9}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 6}, {{1, 0}, 3}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 6}, {{1, 0}, 5}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 6}, {{1, 0}, 9}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 12}, {{1, 0}, 3}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 12}, {{1, 0}, 5}},        // n & s
    {{{0, 0}, 5}, {{-1, 0}, 12}, {{1, 0}, 9}},        // n & s
    // └─┘ ┌─┘ ──┘ └── ┌── └─┐ ┌─┐ ──┐
    {{{0, 0}, 10}, {{0, 1}, 9}, {{0, -1}, 3}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 9}, {{0, -1}, 6}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 9}, {{0, -1}, 10}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 10}, {{0, -1}, 3}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 10}, {{0, -1}, 6}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 12}, {{0, -1}, 3}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 12}, {{0, -1}, 6}},        // e & w
    {{{0, 0}, 10}, {{0, 1}, 12}, {{0, -1}, 10}},        // e & w
};

struct puz_link
{
    char m_ch;
    Position m_target, m_turn;
    int m_dir1 = -1, m_dir2 = -1;
};

struct puz_gem
{
    char m_ch;
    map<Position, puz_link> m_pos2link;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, puz_gem> m_pos2gem;

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
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_pos2gem[{r, c}].m_ch = ch;
    }
    for (auto&& [p1, gem1] : m_pos2gem)
        for (auto&& [p2, gem2] : m_pos2gem) {
            if (p1 == p2) continue;
            if (p1.first == p2.first || p1.second == p2.second) {
                if (gem1.m_ch != gem2.m_ch) continue;
                auto pd = p2 - p1;
                Position os(sign(pd.first), sign(pd.second));
                for (auto p3 = p1 + os; p3 != p2; p3 += os)
                    if (m_pos2gem.count(p3) != 0)
                        goto next;
                auto& o = gem1.m_pos2link[p2];
                o.m_ch = gem2.m_ch;
                o.m_target = p2;
                o.m_dir1 = boost::find(offset, os) - offset;
            next:;
            } else {
                if (gem1.m_ch == gem2.m_ch) continue;
                auto pd = p2 - p1;
                auto f = [&](const Position& pm, const Position& os1, const Position& os2) {
                    if (m_pos2gem.count(pm) != 0) return;
                    for (auto p3 = p1 + os1; p3 != pm; p3 += os1)
                        if (m_pos2gem.count(p3) != 0)
                            return;
                    for (auto p3 = pm + os2; p3 != p2; p3 += os2)
                        if (m_pos2gem.count(p3) != 0)
                            return;
                    auto& o = gem1.m_pos2link[p2];
                    o.m_ch = gem2.m_ch;
                    o.m_target = p2;
                    o.m_turn = pm;
                    o.m_dir1 = boost::find(offset, os1) - offset;
                    o.m_dir2 = boost::find(offset, os2) - offset;
                };
                Position pm1(p1.first, p2.second), pm2(p2.first, p1.second);
                Position os1(0, sign(pd.second)), os2(sign(pd.first), 0);
                f(pm1, os1, os2);
                f(pm2, os2, os1);
            }
        }
}

typedef vector<int> puz_dot;

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_dots, m_matches) < tie(x.m_dots, x.m_matches); 
    }
    bool make_move_pearl(const Position& p, int n);
    bool make_move_pearl2(const Position& p, int n);
    bool make_move_line(const Position& p, int n);
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
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<puz_dot> m_dots;
    // key: the position of the dot
    // value: the index of the permutation
    map<Position, vector<int>> m_matches;
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
            auto it = g.m_pos2gem.find(p);
            if (it == g.m_pos2gem.end())
                dt.push_back(lineseg_off);

            auto& linesegs_all2 = 
                it == g.m_pos2gem.end() ? linesegs_all :
                it->second.m_ch == PUZ_BLACK_GEM ? linesegs_all_black :
                linesegs_all_white;
            for (int lineseg : linesegs_all2)
                if ([&]{
                    for (int i = 0; i < 4; ++i)
                        // The line segment cannot lead to a position outside the board
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& kv : g.m_pos2gem) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(kv.second.m_ch == PUZ_BLACK_GEM ? 4 : 16);
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

        auto perms = m_game->m_pos2gem.at(p).m_ch == PUZ_BLACK_GEM ?
            black_pearl_perms : white_pearl_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto perm = perms[id];
            for (int i = 0; i < 3; ++i) {
                auto& info = perm[i];
                if (boost::algorithm::none_of_equal(dots(p + info.first), info.second))
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_pearl2(p, perm_ids.front()) ? 1 : 0;
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
                    if (m_finished.count({p, i}) == 0 && (
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
            auto& p = kv.first;
            int i = kv.second;
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

bool puz_state::make_move_pearl2(const Position& p, int n)
{
    auto perms = m_game->m_pos2gem.at(p).m_ch == PUZ_BLACK_GEM ?
        black_pearl_perms : white_pearl_perms;
    auto perm = perms[n];
    for (int i = 0; i < 3; ++i) {
        auto& info = perm[i];
        dots(p + info.first) = {info.second};
    }
    m_matches.erase(p);
    return check_loop();
}

bool puz_state::make_move_pearl(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move_pearl2(p, n))
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

bool puz_state::make_move_line(const Position& p, int n)
{
    m_distance = 0;
    auto& dt = dots(p);
    dt = {dt[n]};
    int m = check_dots(false);
    return m == 1 ? check_loop() : m == 2;
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
            int lineseg = dots(p2)[0];
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
            if (!children.back().make_move_pearl(kv.first, n))
                children.pop_back();
        }
    } else {
        int n = boost::min_element(m_dots, [](const puz_dot& dt1, const puz_dot& dt2) {
            auto f = [](int sz) {return sz == 1 ? 1000 : sz;};
            return f(dt1.size()) < f(dt2.size());
        }) - m_dots.begin();
        Position p(n / sidelen(), n % sidelen());
        auto& dt = dots(p);
        for (int i = 0; i < dt.size(); ++i) {
            children.push_back(*this);
            if (!children.back().make_move_line(p, i))
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
            auto& dt = dots(p);
            auto it = m_game->m_pos2gem.find(p);
            out << (it != m_game->m_pos2gem.end() ? it->second.m_ch : ' ')
                << (is_lineseg_on(dt[0], 1) ? '-' : ' ');
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

}

void solve_puz_StraightAndTurn()
{
    using namespace puzzles::StraightAndTurn;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/StraightAndTurn.xml", "Puzzles/StraightAndTurn.txt", solution_format::GOAL_STATE_ONLY);
}
