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

namespace puzzles::CastlePatrol{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_EMPTY2 = '=';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_WALL2 = 'X';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

// first: empty or wall area
// second: size of the area
using puz_hint = pair<char, int>;

struct puz_move
{
    // key: position of the number (hint)
    Position m_start;
    // empty or wall area
    char m_ch;
    set<Position> m_rng, m_neighbors;
    bool operator<(const puz_move& x) const {
        return tie(m_start, m_ch) < tie(x.m_start, x.m_ch);
    }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_hint> m_pos2hint, m_pos2hintBig;
    map<Position, vector<puz_move>> m_pos2moves;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p, const puz_hint* hint)
        : m_game(game), m_hint(hint) { make_move(p); }
    void make_move(const Position& p) { insert(p); }

    bool is_goal_state() const { return size() == m_hint->second; }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const puz_hint* m_hint;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    if (is_goal_state())
        return;
    for (auto& p : *this)
        for (auto& os : offset) {
            // Areas extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile can be occupied by the area
            // if it is a space tile and has not been occupied by the area and
            if (ch2 == PUZ_SPACE && !contains(p2) && boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                // no adjacent tile to it is of the same type as the area, because
                // 3. Areas of the same type cannot share an edge.
                return !contains(p3) && ch3 == m_hint->first;
            }))
                children.emplace_back(*this).make_move(p2);
        }
}

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
                (num > 10 ? m_pos2hintBig : m_pos2hint)[p] = {ch2, num};
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, hint] : m_pos2hint) {
        // Areas can have any form.
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, &hint});
        for (auto& s : smoves)
            // save all goal states as permutations
            // A goal state is a area formed from the number
            if (auto& moves = m_pos2moves[p]; s.is_goal_state()) {
                auto& [start, ch, rng, neighbors] = moves.emplace_back();
                start = p, ch = hint.first, rng = s;
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os;
                            !s.contains(p3) && cells(p3) == PUZ_SPACE)
                            neighbors.insert(p3);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches, m_movesBig) < tie(x.m_cells, x.m_matches, x.m_movesBig);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    void make_move_big();
    bool check_spaces(set<Position>& spaces) const;
    bool check_hints(bool check_empty, bool check_wall) const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count_if(m_cells, [&](char ch) {
            return ch == PUZ_SPACE || ch == PUZ_EMPTY2 || ch == PUZ_WALL2;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number (hint)
    // value : the index of the move
    map<Position, vector<int>> m_matches;
    vector<puz_move> m_movesBig;
    unsigned int m_distance = 0;
    bool m_is_phase_big = false;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    for (auto& [p, moves] : g.m_pos2moves) {
        auto& v = m_matches[p];
        v.resize(moves.size());
        boost::iota(v, 0);
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        auto& moves = m_is_phase_big ? m_movesBig : m_game->m_pos2moves.at(p);
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [start, ch, rng, neighbors] = moves[id];
            char ch12 = ch == PUZ_EMPTY ? PUZ_EMPTY2 : PUZ_WALL2;
            char ch21 = ch == PUZ_EMPTY ? PUZ_WALL : PUZ_EMPTY;
            char ch22 = ch == PUZ_EMPTY ? PUZ_WALL2 : PUZ_EMPTY2;
            return !boost::algorithm::all_of(rng, [&](const Position& p2) {
                char ch3 = cells(p2);
                return p2 == start || ch3 == PUZ_SPACE || ch3 == ch12;
            }) || !boost::algorithm::all_of(neighbors, [&](const Position& p2) {
                // no adjacent tile to it is of the same type as the area, because
                // 3. Areas of the same type cannot share an edge.
                char ch3 = cells(p2);
                return ch3 == PUZ_SPACE || ch3 == ch21 || ch3 == ch22;
            });
        });
        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids[0]), 1;
            }
    }
    return check_hints(true, false) && check_hints(false, true) && check_hints(true, true) ? 2 : 0;
}

struct puz_state4 : Position
{
    puz_state4(const set<Position>* area)
    : m_area(area) { make_move(*area->begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state4>& children) const;

    const set<Position>* m_area;
};

void puz_state4::gen_children(list<puz_state4>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os;
            m_area->contains(p2))
            children.emplace_back(*this).make_move(p2);
}

bool puz_state::check_hints(bool check_empty, bool check_wall) const
{
    set<Position> area;
    map<Position, int> pos2hint;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c)
            switch(Position p(r, c); char ch = cells(p)) {
            case PUZ_SPACE:
            case PUZ_EMPTY2:
            case PUZ_WALL2:
                if (!check_empty && ch == PUZ_EMPTY2 || !check_wall && ch == PUZ_WALL2)
                    break;
                area.insert(p);
                break;
            default:
                if (!check_empty && ch == PUZ_EMPTY || !check_wall && ch == PUZ_WALL)
                    break;
                if (m_is_phase_big) {
                    if (m_matches.contains(p))
                        area.insert(p), pos2hint[p] = m_game->m_pos2hintBig.at(p).second;
                } else {
                    if (m_matches.contains(p))
                        area.insert(p), pos2hint[p] = m_game->m_pos2hint.at(p).second;
                    else if (m_game->m_pos2hintBig.contains(p))
                        area.insert(p), pos2hint[p] = m_game->m_pos2hintBig.at(p).second;
                }
            }
    while (!area.empty()) {
        auto smoves = puz_move_generator<puz_state4>::gen_moves({&area});
        int num = boost::accumulate(smoves, 0, [&](int acc, const Position& p) {
            auto it = pos2hint.find(p);
            return acc + (it == pos2hint.end() ? 0 : it->second);
        });
        if (check_empty && check_wall && smoves.size() != num || smoves.size() < num)
            return false;
        for (auto& p : smoves)
            area.erase(p);
    }
    return true;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& moves = m_is_phase_big ? m_movesBig : m_game->m_pos2moves.at(p);
    auto& [_1, ch, rng, neighbors] = moves[n];
    for (auto& p2 : rng) {
        cells(p2) = ch, ++m_distance;
        if (m_is_phase_big)
            m_matches.erase(p2);
    }
    char ch2 = ch == PUZ_EMPTY ? PUZ_WALL2 : PUZ_EMPTY2;
    for (auto& p2 : neighbors)
        if (char& ch3 = cells(p2); ch3 == PUZ_SPACE)
            ch3 = ch2;
    if (!m_is_phase_big)
        m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

struct puz_state5 : set<Position>
{
    puz_state5(const puz_state* state, const Position& p, const puz_hint& hint);
    void make_move(const Position& p);

    void gen_children(list<puz_state5>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_state* m_state;
    char m_ch, m_ch2;
    int m_num;
    bool m_is_goal;
};

puz_state5::puz_state5(const puz_state* state, const Position& p, const puz_hint& hint)
: m_state(state)
, m_ch(hint.first)
, m_ch2(hint.first == PUZ_EMPTY ? PUZ_EMPTY2 : PUZ_WALL2)
, m_num(hint.second)
{
    make_move(p);
}

void puz_state5::make_move(const Position &p)
{
    insert(p);
    m_is_goal = size() == m_num && boost::algorithm::none_of(*this, [&](const Position& p2) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p3 = p2 + os;
            return !contains(p3) && m_state->cells(p3) == m_ch2;
        });
    });
}

void puz_state5::gen_children(list<puz_state5>& children) const {
    if (size() == m_num)
        return;
    for (auto& p : *this)
        for (auto& os : offset) {
            // Areas extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_state->cells(p2);
            // An adjacent tile can be occupied by the area
            // if it is a space tile and has not been occupied by the area and
            if ((ch2 == PUZ_SPACE || ch2 == m_ch2) && !contains(p2) && boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_state->cells(p3);
                // no adjacent tile to it is of the same type as the area, because
                // 3. Areas of the same type cannot share an edge.
                return !contains(p3) && ch3 == m_ch;
            }))
                children.emplace_back(*this).make_move(p2);
        }
}

void puz_state::make_move_big()
{
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c)
            switch(Position p(r, c); char ch = cells(p)) {
            case PUZ_SPACE:
            case PUZ_EMPTY2:
            case PUZ_WALL2:
                m_matches[p];
            }
    for (auto& [p, hint] : m_game->m_pos2hintBig) {
        // Areas can have any form.
        auto smoves = puz_move_generator<puz_state5>::gen_moves({this, p, hint});
        for (auto& s : smoves)
            // save all goal states as permutations
            // A goal state is a area formed from the number
            if (s.m_is_goal) {
                int n = m_movesBig.size();
                auto& [start, ch, rng, neighbors] = m_movesBig.emplace_back();
                start = p, ch = hint.first, rng = s;
                for (auto& p2 : s) {
                    m_matches[p2].push_back(n);
                    for (auto& os : offset)
                        if (auto p3 = p2 + os;
                            !s.contains(p3) && cells(p3) == PUZ_SPACE)
                            neighbors.insert(p3);
                }
            }
    }
    m_is_phase_big = true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, move_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : move_ids)
            if (!children.emplace_back(*this).make_move(p, n))
                children.pop_back();
    } else if (!m_is_phase_big) {
        children.emplace_back(*this).make_move_big();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (auto it = m_game->m_pos2hint.find(p); it != m_game->m_pos2hint.end())
                out << format("{:2}", it->second.second);
            else if (auto it = m_game->m_pos2hintBig.find(p); it != m_game->m_pos2hintBig.end())
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

void solve_puz_CastlePatrol()
{
    using namespace puzzles::CastlePatrol;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CastlePatrol.xml", "Puzzles/CastlePatrol.txt", solution_format::GOAL_STATE_ONLY);
}
