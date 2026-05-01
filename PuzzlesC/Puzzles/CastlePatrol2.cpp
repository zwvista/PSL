#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 4/Castle Patrol

    Summary
    Don't fall down the wall

    Description
    1. Divide the grid into walls and empty areas. Every area contains one number.
    2. The number indicates the size of the area. Numbers in wall tiles are part
       of wall areas; numbers in empty tiles are part of empty areas.
    3. Areas of the same type cannot share an edge.
*/

namespace puzzles::CastlePatrol2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_EMPTY2 = '=';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_WALL2 = 'X';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

// first: empty or wall area
// second: size of the area
using puz_hint_info = pair<char, int>;

struct puz_move
{
    // key: position of the number (hint)
    Position m_start;
    // empty or wall area
    char m_ch;
    set<Position> m_rng, m_neighbors;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_hint_info> m_pos2hint_info;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch1 = str[c * 2 - 2], ch2 = str[c * 2 - 1];
            m_cells.push_back(ch2);
            if (ch1 != ' ') {
                Position p(r, c);
                int num = isdigit(ch1) ? ch1 - '0' : ch1 - 'A' + 10;
                m_pos2hint_info[p] = {ch2, num};
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_pos2hints) < tie(x.m_cells, x.m_pos2hints);
    }
    bool make_move(const puz_move& move);
    void gen_maps();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_pos2hints.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, set<Position>> m_pos2hints;
    map<Position, int> m_hint2size;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    gen_maps();
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p_hint, const puz_hint_info& hint_info);
    void make_move(const Position& p, int i) {
        static_cast<Position&>(*this) = p, m_index = i;
    }

    bool is_goal_state() const { return m_index == m_num; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    Position m_p_hint;
    char m_ch, m_ch2;
    int m_num, m_index;
};

puz_state2::puz_state2(const puz_state* s, const Position& p_hint, const puz_hint_info& hint_info)
: m_state(s)
, m_p_hint(p_hint)
, m_ch(hint_info.first)
, m_ch2(m_ch == PUZ_EMPTY ? PUZ_EMPTY2 : PUZ_WALL2)
, m_num(hint_info.second)
{
    make_move(p_hint, 1);
}

void puz_state2::gen_children(list<puz_state2>& children) const {
    if (is_goal_state())
        return;
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (char ch2 = m_state->cells(p2); (ch2 == PUZ_SPACE || ch2 == m_ch2) &&
            boost::algorithm::none_of(offset, [&](const Position& os2) {
            auto p3 = p2 + os2;
            return p3 != m_p_hint && m_state->cells(p3) == m_ch;
        }))
            children.emplace_back(*this).make_move(p2, m_index + 1);
    }
}

void puz_state::gen_maps()
{
    for (auto& [_1, hints] : m_pos2hints)
        hints.clear();
    m_hint2size.clear();
    for (auto& [p_hint, hint_info] : m_game->m_pos2hint_info) {
        if (m_finished.contains(p_hint)) continue;
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p_hint, hint_info});
        m_hint2size[p_hint] = smoves.size();
        for (auto& s : smoves)
            if (char ch = cells(s); ch == PUZ_SPACE || ch == s.m_ch2)
                m_pos2hints[s].insert(p_hint);
    }
}

bool puz_state::make_move(const puz_move& move)
{
    m_distance = 0;
    auto& [p_hint, ch1, rng, neighbors] = move;
    m_finished.insert(p_hint);
    char ch2 = ch1 == PUZ_EMPTY ? PUZ_EMPTY2 : PUZ_WALL2;
    for (auto& p : rng)
        if (char& ch = cells(p); ch == PUZ_SPACE || ch == ch2)
            ch = ch1, m_distance++, m_pos2hints.erase(p);
    char ch3 = ch1 == PUZ_EMPTY ? PUZ_WALL2 : PUZ_EMPTY2;
    for (auto& p : neighbors)
        if (char& ch = cells(p); ch == PUZ_SPACE)
            ch = ch3;
    gen_maps();
    return boost::algorithm::none_of(m_pos2hints, [&](const pair<const Position, set<Position>>& kv) {
        return kv.second.empty();
    });
}

struct puz_state3 : set<Position>
{
    puz_state3(const puz_state* s, const Position& p_hint, const puz_hint_info& hint_info, const vector<Position>& rng2);
    void make_move(const Position& p) { insert(p); }

    bool is_goal_state() const {
        return size() == m_num &&
        boost::algorithm::all_of(m_rng2, [&](const Position& p) {
            return contains(p);
        });
    }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
    vector<Position> m_rng2;
    char m_ch, m_ch2;
    int m_num;
};

puz_state3::puz_state3(const puz_state* s, const Position& p_hint, const puz_hint_info& hint_info, const vector<Position>& rng2)
: m_state(s), m_rng2(rng2)
, m_ch(hint_info.first)
, m_ch2(m_ch == PUZ_EMPTY ? PUZ_EMPTY2 : PUZ_WALL2)
, m_num(hint_info.second)
{
    make_move(p_hint);
}

void puz_state3::gen_children(list<puz_state3>& children) const {
    int sz = size();
    if (sz == m_num)
        return;
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            if (char ch2 = m_state->cells(p2);
                (ch2 == PUZ_SPACE || ch2 == m_ch2) && !contains(p2) &&
                boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_state->cells(p3);
                return !contains(p3) && (ch3 == m_ch || sz == m_num - 1 && ch3 == m_ch2);
            }))
                children.emplace_back(*this).make_move(p2);
        }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, hints] = *boost::min_element(m_pos2hints, [&](
        const pair<const Position, set<Position>>& kv1,
        const pair<const Position, set<Position>>& kv2) {
        auto f = [&](const set<Position>& hints) {
            int sz_all = boost::accumulate(hints, 0, [&](int acc, const Position& p_hint) {
                return acc + m_hint2size.at(p_hint);
            });
            return pair(hints.size(), sz_all);
        };
        return f(kv1.second) < f(kv2.second);
    });
    vector<puz_move> moves;
    for (auto& p_hint : hints) {
        auto& hint_info = m_game->m_pos2hint_info.at(p_hint);
        vector<Position> rng2;
        if (hints.size() > 1)
            rng2 = {p};
        else
            for (auto& [p2, hints2] : m_pos2hints)
                if (hints2.size() == 1 && *hints2.begin() == p_hint)
                    rng2.push_back(p2);
        auto smoves = puz_move_generator<puz_state3>::gen_moves({this, p_hint, hint_info, rng2});
        for (auto& s : smoves) {
            if (!s.is_goal_state()) continue;
            set<Position> neighbors;
            for (auto& p2 : s)
                for (auto& os : offset)
                    if (auto p3 = p2 + os; !s.contains(p3))
                        if (char ch = cells(p3); ch == PUZ_SPACE)
                            neighbors.insert(p3);
            moves.emplace_back(p_hint, s.m_ch, s, neighbors);
        }
    }
    for (auto& move : moves)
        if (!children.emplace_back(*this).make_move(move))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (auto it = m_game->m_pos2hint_info.find(p); it != m_game->m_pos2hint_info.end())
                out << format("{:2}", it->second.second);
            else
                out << "  ";
            out << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CastlePatrol2()
{
    using namespace puzzles::CastlePatrol2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CastlePatrol.xml", "Puzzles/CastlePatrol2.txt", solution_format::GOAL_STATE_ONLY);
}
