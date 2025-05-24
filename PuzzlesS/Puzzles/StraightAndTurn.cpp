#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 7/Straight and Turn

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

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_link
{
    char m_ch;
    Position m_target, m_turn{-1, -1};
    // the direction from this position to the target(or the turning point) and its reverse direction
    int m_dir11 = -1, m_dir12 = -1;
    // the direction from the turning point to the target and its reverse direction
    int m_dir21 = -1, m_dir22 = -1;
};

struct puz_gem
{
    char m_ch;
    vector<puz_link> m_links;
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
    for (auto& kv1 : m_pos2gem) {
        auto& p1 = kv1.first;
        auto& gem1 = kv1.second;
        for (auto& kv2 : m_pos2gem) {
            auto& p2 = kv2.first;
            auto& gem2 = kv2.second;
            if (p1 == p2) continue;
            if (p1.first == p2.first || p1.second == p2.second) {
                if (gem1.m_ch != gem2.m_ch) continue;
                auto pd = p2 - p1;
                [&]{
                    Position os(sign(pd.first), sign(pd.second));
                    for (auto p3 = p1 + os; p3 != p2; p3 += os)
                        if (m_pos2gem.contains(p3))
                            return;
                    auto& o = gem1.m_links.emplace_back();
                    o.m_ch = gem2.m_ch;
                    o.m_target = p2;
                    o.m_dir11 = boost::find(offset, os) - offset, o.m_dir12 = (o.m_dir11 + 2) % 4;
                }();
            } else {
                if (gem1.m_ch == gem2.m_ch) continue;
                auto pd = p2 - p1;
                auto f = [&](const Position& pm, const Position& os1, const Position& os2) {
                    if (m_pos2gem.contains(pm)) return;
                    for (auto p3 = p1 + os1; p3 != pm; p3 += os1)
                        if (m_pos2gem.contains(p3))
                            return;
                    for (auto p3 = pm + os2; p3 != p2; p3 += os2)
                        if (m_pos2gem.contains(p3))
                            return;
                    auto& o = gem1.m_links.emplace_back();
                    o.m_ch = gem2.m_ch;
                    o.m_target = p2;
                    o.m_turn = pm;
                    o.m_dir11 = boost::find(offset, os1) - offset, o.m_dir12 = (o.m_dir11 + 2) % 4;
                    o.m_dir21 = boost::find(offset, os2) - offset, o.m_dir22 = (o.m_dir21 + 2) % 4;
                };
                Position pm1(p1.first, p2.second), pm2(p2.first, p1.second);
                Position os1(0, sign(pd.second)), os2(sign(pd.first), 0);
                f(pm1, os1, os2);
                f(pm2, os2, os1);
            }
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    int& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_dots, m_matches) < tie(x.m_dots, x.m_matches); 
    }
    bool make_move(const Position& p, int n);
    void make_move2(Position p, int n);
    int find_matches(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_matches, 0, [&](int acc, const pair<const Position, vector<int>>& kv){
            return acc + (dots(kv.first) == lineseg_off ? 2 : 1);
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<int> m_dots;
    // key: the position of the gem
    // value: the index of the permutation
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_dots(g.m_dot_count), m_game(&g)
{
    for (auto&& [p, gem] : g.m_pos2gem) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(gem.m_links.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto f = [&](const Position& p, const Position& os, const Position& p2) {
        for (auto p3 = p + os; p3 != p2; p3 += os)
            if (dots(p3) != lineseg_off)
                return true;
        return false;
    };
    
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto perms = m_game->m_pos2gem.at(p).m_links;
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& o = perms[id];
            return !m_matches.contains(o.m_target) ||
                (o.m_dir21 == -1 ? f(p, offset[o.m_dir11], o.m_target) :
                dots(o.m_turn) != lineseg_off || f(p, offset[o.m_dir11], o.m_turn) || f(o.m_turn, offset[o.m_dir21], o.m_target));
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids[0]), 1;
            }
    }
    return check_loop() ? 2 : 0;
}

void puz_state::make_move2(Position p, int n)
{
    auto f = [&](const Position& p, int dir1, int dir2, const Position& p2) {
        auto& os = offset[dir1];
        for (auto p3 = p + os; p3 != p2; p3 += os)
            dots(p3) = (1 << dir1) | (1 << dir2);
    };

    auto& o = m_game->m_pos2gem.at(p).m_links[n];
    bool finished = dots(p) != lineseg_off;
    bool finished2 = dots(o.m_target) != lineseg_off;
    dots(p) |= 1 << o.m_dir11;
    if (o.m_dir21 == -1) {
        dots(o.m_target) |= 1 << o.m_dir12;
        f(p, o.m_dir11, o.m_dir12, o.m_target);
    } else {
        dots(o.m_turn) = (1 << o.m_dir12) | (1 << o.m_dir21);
        f(p, o.m_dir11, o.m_dir12, o.m_turn);
        dots(o.m_target) |= 1 << o.m_dir22;
        f(o.m_turn, o.m_dir21, o.m_dir22, o.m_target);
    }
    
    if (finished)
        m_matches.erase(p);
    else
        boost::remove_erase(m_matches.at(p), n);
    if (finished2)
        m_matches.erase(o.m_target);
    else
        boost::remove_erase_if(m_matches.at(o.m_target), [&](int n2){
            return m_game->m_pos2gem.at(o.m_target).m_links[n2].m_target == p;
        });
    m_distance += 2;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

bool puz_state::check_loop() const
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (dots(p) != lineseg_off)
                rng.insert(p);
        }

    bool has_branch = false;
    while (!rng.empty()) {
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            int lineseg = dots(p2);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            int dt = dots(p);
            auto it = m_game->m_pos2gem.find(p);
            out << (it != m_game->m_pos2gem.end() ? it->second.m_ch : ' ')
                << (is_lineseg_on(dt, 1) ? '-' : ' ');
        }
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vert-lines
            out << (is_lineseg_on(dots({r, c}), 2) ? "| " : "  ");
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
