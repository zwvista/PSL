#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 2/Turn me up

    Summary
    How many turns

    Description
    1. Connect the circles between them, in pairs.
    2. The number on the circle tells you how many turns the connection
       does between circles.
    3. Two circles without numbers can have any number of turns.
    4. All tiles on the board must be used and all circles must be connected.
*/

namespace puzzles::TurnMeUp{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_QM = '?';

constexpr array<Position, 4> offset = Position::Directions4;

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2ch;
    string m_cells;
    map<Position, vector<vector<Position>>> m_pos2perms;
    map<Position, vector<pair<Position, int>>> m_pos2perminfo;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game* game, const Position& p, char ch)
        : m_game(game), m_p(p), m_char(ch) { make_move(-1, p); }
    bool cannot_turn() const {
        return m_char != PUZ_QM && m_char - '0' == m_turn_count;
    }
    bool is_goal_number(char ch2, int turn_count) const {
        return m_char == PUZ_QM && ch2 == PUZ_QM ||
            (m_char == PUZ_QM || ch2 == PUZ_QM) &&
            (m_char != PUZ_QM ? m_char : ch2) - '0' == turn_count ||
            m_char == ch2 && m_char - '0' == turn_count;
    }

    bool is_goal_state() const {
        if (size() == 1) return false;
        char ch2 = m_game->cells(back());
        return ch2 != PUZ_SPACE && (m_char == PUZ_QM || ch2 == PUZ_QM || m_char == ch2);
    }
    void make_move(int i, Position p2);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    Position m_p;
    char m_char;
    int m_last_dir = -1;
    int m_turn_count = 0;
};

void puz_state2::make_move(int i, Position p2)
{
    if (m_last_dir != -1 && m_last_dir != i) ++m_turn_count;
    m_last_dir = i;
    push_back(p2);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& p = back();
    for (int i = 0; i < 4; ++i) {
        if (m_last_dir == (i + 2) % 4) continue;
        if (m_last_dir != -1 && m_last_dir != i && cannot_turn()) continue;
        if (auto p2 = p + offset[i];
            m_game->is_valid(p2) && boost::algorithm::none_of_equal(*this, p2))
            if (char ch2 = m_game->cells(p2); ch2 == PUZ_SPACE ||
                p2 > m_p && is_goal_number(ch2,
                    m_turn_count + (m_last_dir == -1 || m_last_dir == i ? 0 : 1))) {
                children.emplace_back(*this).make_move(i, p2);
            }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_pos2ch[{r, c}] = ch;
    }

    for (auto& [p, ch] : m_pos2ch) {
        puz_state2 sstart(this, p, ch);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_bfs<puz_state2, true, false, false>::find_solution(sstart, spaths); found)
            // save all goal states as permutations
            // A goal state is a line starting from N to N
            for (auto& perms = m_pos2perms[p]; auto& spath : spaths) {
                auto& perm = spath.back();
                for (int n = perms.size(); auto& p2 : perm)
                    m_pos2perminfo[p2].emplace_back(p, n);
                perms.push_back(perm);
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

    const puz_game* m_game;
    vector<int> m_dots;
    map<Position, vector<pair<Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_dots(g.m_sidelen * g.m_sidelen)
, m_matches(g.m_pos2perminfo)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, infos] : m_matches) {
        boost::remove_erase_if(infos, [&](const pair<Position, int>& info) {
            auto& perm = m_game->m_pos2perms.at(info.first)[info.second];
            return !boost::algorithm::all_of(perm, [&](const Position& p2) {
                return m_matches.contains(p2);
            });
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
        auto f = [&](const Position& p3) {
            int dir = boost::find(offset, p3 - p2) - offset.begin();
            dots(p2) |= 1 << dir;
            dots(p3) |= 1 << (dir + 2) % 4;
            m_matches.erase(p2), ++m_distance;
        };
        if (i < sz - 1)
            f(perm[i + 1]);
        if (i > 0)
            f(perm[i - 1]);
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
        const pair<const Position, vector<pair<Position, int>>>& kv1,
        const pair<const Position, vector<pair<Position, int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& info : infos)
        if (!children.emplace_back(*this).make_move(info.first, info.second))
            children.pop_back();
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

void solve_puz_TurnMeUp()
{
    using namespace puzzles::TurnMeUp;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TurnMeUp.xml", "Puzzles/TurnMeUp.txt", solution_format::GOAL_STATE_ONLY);
}
