#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 1/Trace Numbers

    Summary
    Trace Numbers

    Description
    1. On the board there are a few number sets. Those numbers are
       sequences all starting from 1 up to a number N.
    2. You should draw as many lines into the grid as number sets:
       a line starts with the number 1, goes through the numbers in
       order up to the highest, where it ends.
    3. In doing this, you have to pass through all tiles on the board.
       Lines cannot cross.
*/

namespace puzzles::TraceNumbers{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_FROM = 1;
constexpr auto PUZ_TO = 2;

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, vector<Position>> m_ch2rng;
    string m_start;
    char m_max_ch;
    map<Position, vector<vector<Position>>> m_pos2perms;
    map<pair<Position, int>, vector<pair<Position, int>>> m_posinfo2perminfo;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p, char ch)
        : m_game(&game), m_char(ch) { make_move(p); }

    bool is_goal_state() const { return m_game->cells(back()) == m_char + 1; }
    void make_move(const Position& p) { push_back(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    char m_char;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& p = back(); auto& os : offset)
        if (auto p2 = p + os; 
            m_game->is_valid(p2) && boost::algorithm::none_of_equal(*this, p2))
            if (char ch2 = m_game->cells(p2); ch2 == PUZ_SPACE || ch2 == m_char + 1) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_ch2rng[ch].emplace_back(r, c);
    }
    m_max_ch = m_ch2rng.rbegin()->first;

    for (auto& [ch, rng] : m_ch2rng) {
        if (ch == m_max_ch) break;
        for (auto& p : rng) {
            puz_state2 sstart(*this, p, ch);
            list<list<puz_state2>> spaths;
            puz_solver_bfs<puz_state2, true, false, false>::find_solution(sstart, spaths);
            // save all goal states as permutations
            // A goal state is a line starting from N to N+1
            auto& perms = m_pos2perms[p];
            for (auto& spath : spaths)
                perms.push_back(spath.back());
        }
    }

    for (auto& [p, perms] : m_pos2perms)
        for (int i = 0; i < perms.size(); ++i) {
            auto& perm = perms[i];
            for (int j = 0, sz = perm.size(); j < sz; ++j) {
                auto& p2 = perm[j];
                if (j < sz - 1)
                    m_posinfo2perminfo[{p2, PUZ_FROM}].emplace_back(p, i);
                if (j > 0)
                    m_posinfo2perminfo[{p2, PUZ_TO}].emplace_back(p, i);
            }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    int dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    int& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<int> m_dots;
    map<pair<Position, int>, vector<pair<Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_dots(g.m_sidelen * g.m_sidelen)
, m_matches(g.m_posinfo2perminfo)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, infos] : m_matches) {
        boost::remove_erase_if(infos, [&](const pair<Position, int>& info) {
            auto& perm = m_game->m_pos2perms.at(info.first)[info.second];
            for (int i = 0, sz = perm.size(); i < sz; ++i) {
                auto& p2 = perm[i];
                int dt = dots(p2);
                if (i < sz - 1 && !m_matches.contains({p2, PUZ_FROM}) ||
                    i > 0 && !m_matches.contains({p2, PUZ_TO}))
                    return true;
            }
            return false;
        });

        if (!init)
            switch(infos.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(infos[0].first, infos[0].second), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_pos2perms.at(p)[n];
    for (int i = 0, sz = perm.size(); i < sz; ++i) {
        auto& p2 = perm[i];
        auto f = [&](const Position& p3, int fromto) {
            int dir = boost::find(offset, p3 - p2) - offset;
            dots(p2) |= 1 << dir;
            dots(p3) |= 1 << (dir + 2) % 4;
            m_matches.erase({p2, fromto}), ++m_distance;
        };
        if (i < sz - 1)
            f(perm[i + 1], PUZ_FROM);
        if (i > 0)
            f(perm[i - 1], PUZ_TO);
    }
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, infos] = *boost::min_element(m_matches, [](
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv1,
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& info : infos) {
        children.push_back(*this);
        if (!children.back().make_move(info.first, info.second))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << m_game->cells(p)
                << (is_lineseg_on(dots(p), 1) ? '-' : ' ');
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c}), 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_TraceNumbers()
{
    using namespace puzzles::TraceNumbers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TraceNumbers.xml", "Puzzles/TraceNumbers.txt", solution_format::GOAL_STATE_ONLY);
}
