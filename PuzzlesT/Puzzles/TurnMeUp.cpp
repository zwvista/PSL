#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
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

using puz_move = vector<Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2ch;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_id;

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
    if (is_goal_state())
        return;
    auto& p = back();
    for (int i = 0; i < 4; ++i) {
        if (m_last_dir == (i + 2) % 4) continue;
        if (m_last_dir != -1 && m_last_dir != i && cannot_turn()) continue;
        if (auto p2 = p + offset[i];
            m_game->is_valid(p2) && boost::algorithm::none_of_equal(*this, p2))
            if (char ch2 = m_game->cells(p2); ch2 == PUZ_SPACE ||
                p2 > m_p && is_goal_number(ch2,
                    m_turn_count + (m_last_dir == -1 || m_last_dir == i ? 0 : 1)))
                children.emplace_back(*this).make_move(i, p2);
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
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, ch});
        for (auto& s : smoves)
            if (s.is_goal_state()) {
                int n = m_moves.size();
                m_moves.push_back(s);
                m_pos2move_id[p].push_back(n);
                for (auto& p2 : s)
                    m_pos2move_id[p2].push_back(n);
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
    bool make_move(int n);
    void make_move2(int n);
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
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_dots(g.m_sidelen * g.m_sidelen)
, m_matches(g.m_pos2move_id)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& move = m_game->m_moves[id];
            return !boost::algorithm::all_of(move, [&](const Position& p2) {
                return dots(p2) == lineseg_off;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& move = m_game->m_moves[n];
    for (int i = 0, sz = move.size(); i < sz; ++i) {
        auto& p2 = move[i];
        auto f = [&](const Position& p3) {
            int dir = boost::find(offset, p3 - p2) - offset.begin();
            dots(p2) |= 1 << dir;
            dots(p3) |= 1 << (dir + 2) % 4;
            m_matches.erase(p2), ++m_distance;
        };
        if (i < sz - 1)
            f(move[i + 1]);
        if (i > 0)
            f(move[i - 1]);
    }
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
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
