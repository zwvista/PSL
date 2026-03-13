#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 4/Snake-omino

    Summary
    Snakes on a Plain

    Description
    1. Find Snakes by numbering them:
    2. A snake is a one-cell-wide path at least two cells long. A snake cannot touch itself,
       not even diagonally.
    3. A cell with a circle must be at one of the ends of a snake. A snake may contain one
       circled cell, two circled cells, or no circled cells at all.
    4. A cell with a number must be part of a snake with a length of exactly that number of cells.
    5. Two snakes of the same length must not be orthogonally adjacent.
    6. A cell with a cross cannot be an end of a snake.
    7. every cell in the board is part of a snake.
*/

namespace puzzles::Snakeomino2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_END = 'O';
constexpr auto PUZ_NOT_END = 'X';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_move
{
    char m_num;
    vector<Position> m_snake;
    set<Position> m_neighbors;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_hints, m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char hints(const Position& p) const { return m_hints[p.first * m_sidelen + p.second]; }
    char& hints(const Position& p) { return m_hints[p.first * m_sidelen + p.second]; }
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p, false); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of_equal(*this, p);
    }

    void make_move(const Position& p, bool at_front);
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    char m_num = PUZ_SPACE;
    bool m_is_goal;
};

void puz_state2::make_move(const Position& p, bool at_front)
{
    at_front ? (void)insert(begin(), p) : push_back(p);
    if (m_num == PUZ_SPACE)
        m_num = m_game->cells(p);
    int sz = size();
    m_is_goal = sz > 1 && (m_num == PUZ_SPACE || sz == m_num - '0') &&
    boost::algorithm::none_of(*this, [&](const Position& p2) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p3 = p2 + os;
            return boost::algorithm::none_of_equal(*this, p3) &&
            m_game->cells(p3) - '0' == sz;
        });
    });
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    int sz = size();
    if (m_num == PUZ_SPACE && sz == 9 || sz > 1 && sz == m_num - '0')
        return;
    auto f = [&](const Position& p, bool at_front) {
        for (auto& os : offset) {
            auto p2 = p + os;
            if (char ch = m_game->cells(p2);
                !is_self(p2) && (m_num == PUZ_SPACE && ch != PUZ_BOUNDARY ||
                ch == PUZ_SPACE || ch == m_num) &&
                // 2. A snake is a one-cell-wide path at least two cells long. A snake cannot touch itself,
                //    not even diagonally.
                boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                return p3 != p && is_self(p3);
            }))
                children.emplace_back(*this).make_move(p2, at_front);
        }
    };
    // 3. A cell with a circle must be at one of the ends of a snake. A snake may contain one
    // circled cell, two circled cells, or no circled cells at all.
    // 6. A cell with a cross cannot be an end of a snake.
    char ch_f = m_game->hints(front()), ch_b = m_game->hints(back());
    if (sz > 1 && ch_f != PUZ_END && ch_b != PUZ_NOT_END)
        f(front(), true);
    if (sz == 1 || ch_f != PUZ_NOT_END && ch_b != PUZ_END)
        f(back(), false);
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
, m_hints(m_sidelen * m_sidelen, PUZ_SPACE)
, m_cells(m_sidelen* m_sidelen, PUZ_SPACE)
{
    for (int i = 0; i < m_sidelen; ++i)
        hints({i, 0}) = hints({i, m_sidelen - 1}) =
        hints({0, i}) = hints({m_sidelen - 1, i}) =
        cells({i, 0}) = cells({i, m_sidelen - 1}) =
        cells({0, i}) = cells({m_sidelen - 1, i}) = PUZ_BOUNDARY;
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            cells(p) = str[c * 2 - 2];
            hints(p) = str[c * 2 - 1];
        }
    }
    for (int r = 1; r < m_sidelen - 1; ++r)
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p});
            for (auto& s : smoves)
                if (s.m_is_goal && boost::algorithm::none_of(m_moves, [&](const puz_move& move) {
                    return move.m_snake == s;
                })) {
                    int n = m_moves.size();
                    auto& [num, snake, neighbors] = m_moves.emplace_back();
                    snake = s, num = s.size() + '0';
                    for (auto& p : s) {
                        m_pos2move_ids[p].push_back(n);
                        for (auto& os : offset)
                            if (auto p2 = p + os; cells(p2) != PUZ_BOUNDARY &&
                                boost::algorithm::none_of_equal(s, p2))
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
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
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
    string m_cells;
    // key: the position of the hint
    // value.elem: the index of the move
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
            auto& [num, snake, neighbors] = m_game->m_moves[id];
            return boost::algorithm::any_of(snake, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != num;
            }) || boost::algorithm::any_of(neighbors, [&](const Position& p2) {
                // 5. Two snakes of the same length must not be orthogonally adjacent.
                return cells(p2) == num;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [num, snake, _1] = m_game->m_moves[n];
    for (auto& p : snake) {
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

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

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

void solve_puz_Snakeomino2()
{
    using namespace puzzles::Snakeomino2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Snakeomino.xml", "Puzzles/Snakeomino2.txt", solution_format::GOAL_STATE_ONLY);
}
