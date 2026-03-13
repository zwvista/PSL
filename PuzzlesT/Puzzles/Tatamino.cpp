#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Tatamino

    Summary
    Which is a little Tatami

    Description
    1. Plays like Fillomino, in which you have to guess areas on the board
       marked by their number.
    2. In Tatamino, however, you only have areas 1, 2 and 3 tiles big.
    3. Please remember two areas of the same number size can't be touching.
*/

namespace puzzles::Tatamino{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_move {
    char m_num;
    set<Position> m_area, m_neighbors;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p); }

    void make_move(const Position& p);
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    char m_num = PUZ_SPACE;
    bool m_is_goal;
};

void puz_state2::make_move(const Position &p)
{
    insert(p);
    if (m_num == PUZ_SPACE)
        m_num = m_game->cells(p);
    int sz = size();
    m_is_goal = (m_num == PUZ_SPACE || sz == m_num - '0') &&
    boost::algorithm::none_of(*this, [&](const Position& p2) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p3 = p2 + os;
            return !contains(p3) && m_game->cells(p3) - '0' == sz;
        });
    });
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    int sz = size();
    if (m_num == PUZ_SPACE && sz == 3 || sz == m_num - '0')
        return;
    for (auto& p : *this)
        for (auto& os : offset)
            if (auto p2 = p + os; !contains(p2))
                if (char ch = m_game->cells(p2);
                    ch == PUZ_SPACE || ch != PUZ_BOUNDARY &&
                    sz < ch - '0' && (m_num == PUZ_SPACE || ch == m_num))
                    children.emplace_back(*this).make_move(p2);
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        m_cells.push_back(PUZ_BOUNDARY);
        m_cells.append(strs[r - 1]);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r)
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p});
            for (auto& s : smoves)
                if (s.m_is_goal && boost::algorithm::none_of(m_moves, [&](const puz_move& move) {
                    return move.m_area == s;
                })) {
                    int n = m_moves.size();
                    auto& [num, area, neighbors] = m_moves.emplace_back();
                    area = s, num = s.size() + '0';
                    for (auto& p : s) {
                        m_pos2move_ids[p].push_back(n);
                        for (auto& os : offset)
                            if (auto p2 = p + os; !s.contains(p2))
                                neighbors.insert(p2);
                    }
                }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
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
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [num, area, neighbors] = m_game->m_moves[id];
            return boost::algorithm::any_of(area, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != num;
            }) || boost::algorithm::any_of(neighbors, [&](const Position& p2) {
                return cells(p2) == num;
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
    auto& [num, area, _1] = m_game->m_moves[n];
    for (auto& p : area) {
        cells(p) = num;
        ++m_distance, m_matches.erase(p);
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
    auto& [p, move_ids] = *boost::min_element(m_matches, [](
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
    auto f = [&](const Position& p1, const Position& p2) {
        return cells(p1) != cells(p2);
    };
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Tatamino()
{
    using namespace puzzles::Tatamino;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Tatamino.xml", "Puzzles/Tatamino.txt", solution_format::GOAL_STATE_ONLY);
}
