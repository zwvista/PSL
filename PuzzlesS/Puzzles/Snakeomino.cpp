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

namespace puzzles::Snakeomino{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_END = 'O';
constexpr auto PUZ_NOT_END = 'X';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_move
{
    char m_num;
    vector<Position> m_snake;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_hints, m_cells;
    map<Position, char> m_pos2num;
    map<Position, vector<puz_move>> m_pos2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    char hints(const Position& p) const { return m_hints[p.first * m_sidelen + p.second]; }
    char& hints(const Position& p) { return m_hints[p.first * m_sidelen + p.second]; }
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p, char num)
        : m_game(&game), m_num(num) { make_move(p, false); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of_equal(*this, p);
    }

    bool is_goal_state() const { return size() == m_num - '0'; }
    void make_move(const Position& p, bool at_front) {
        at_front ? (void)insert(begin(), p) : push_back(p);
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    char m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto f = [&](const Position& p, bool at_front) {
        for (auto& os : offset) {
            auto p2 = p + os;
            if (char ch = m_game->cells(p2);
                !is_self(p2) && (ch == PUZ_SPACE || ch == m_num) &&
                boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                return p3 != p && is_self(p3);
            })) {
                children.push_back(*this);
                children.back().make_move(p2, at_front);
            }
        }
    };
    // 3. A cell with a circle must be at one of the ends of a snake. A snake may contain one
    // circled cell, two circled cells, or no circled cells at all.
    // 6. A cell with a cross cannot be an end of a snake.
    char ch_f = m_game->hints(front()), ch_b = m_game->hints(back());
    if (size() > 1 && ch_f != PUZ_END && ch_b != PUZ_NOT_END)
        f(front(), true);
    if (size() == 1 || ch_f != PUZ_NOT_END && ch_b != PUZ_END)
        f(back(), false);
}

struct puz_state3 : vector<Position>
{
    puz_state3(const puz_game& game, const Position& p)
        : m_game(&game) { make_move(PUZ_SPACE, p, false); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of_equal(*this, p);
    }

    void make_move(char num, const Position& p, bool at_front) {
        if (m_num == PUZ_SPACE)
            m_num = num;
        at_front ? (void)insert(begin(), p) : push_back(p);
    }
    void gen_children(list<puz_state3>& children) const;

    const puz_game* m_game = nullptr;
    char m_num = PUZ_SPACE;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    if (m_num != PUZ_SPACE && size() == m_num - '0')
        return;

    auto f = [&](const Position& p, bool at_front) {
        for (auto& os : offset) {
            auto p2 = p + os;
            if (char ch = m_game->cells(p2);
                !is_self(p2) && (ch == PUZ_SPACE || ch == m_num) &&
                boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                return p3 != p && is_self(p3);
            })) {
                children.push_back(*this);
                children.back().make_move(ch, p2, at_front);
            }
        }
    };
    // 3. A cell with a circle must be at one of the ends of a snake. A snake may contain one
    // circled cell, two circled cells, or no circled cells at all.
    // 6. A cell with a cross cannot be an end of a snake.
    char ch_f = m_game->hints(front()), ch_b = m_game->hints(back());
    if (size() > 1 && ch_f != PUZ_END && ch_b != PUZ_NOT_END)
        f(front(), true);
    if (size() == 1 || ch_f != PUZ_NOT_END && ch_b != PUZ_END)
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
            char ch_h = hints(p) = str[c * 2 - 2];
            char ch_c = cells(p) = str[c * 2 - 1];
            if (ch_h != PUZ_SPACE || ch_c != PUZ_SPACE)
                m_pos2num[p] = cells(p) = ch_c;
        }
    }

    for (auto& [p, num] : m_pos2num) {
        auto& moves = m_pos2moves[p];
        auto f = [&](const vector<Position>& v, char num2) {
            return boost::algorithm::none_of(moves, [&](const puz_move& move) {
                return move.m_snake == v;
            }) && boost::algorithm::all_of(v, [&](const Position& p2) {
                return boost::algorithm::all_of(offset, [&](const Position& os) {
                    auto p3 = p2 + os;
                    return boost::algorithm::any_of_equal(v, p3) ||
                        cells(p3) != num2;
                });
            });
        };
        if (num != PUZ_SPACE) {
            puz_state2 sstart(*this, p, num);
            list<list<puz_state2>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                // save all goal states as permutations
                // A goal state is a snake formed from the hint
                for (auto& spath : spaths) {
                    auto& s = spath.back();
                    auto v = s.front() < s.back() ? s : vector<Position>{s.rbegin(), s.rend()};
                    if (f(v, num))
                        moves.emplace_back(num, v);
                }
        } else {
            auto smoves = puz_move_generator<puz_state3>::gen_moves({*this, p});
            for (auto& s : smoves) {
                if (s.size() == 1) continue;
                auto v = s.front() < s.back() ? s : vector<Position>{s.rbegin(), s.rend()};
                if (char num = v.size() + '0'; f(v, num))
                    moves.emplace_back(num, v);
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
    bool make_move_hint(const Position& p, int n);
    void make_move_hint2(const Position& p, int n);
    int find_matches(bool init);
    bool make_move_hidden(char num, const vector<Position>& snake);
    set<Position> get_spaces() const;
    bool check_spaces(const set<Position> spaces) const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the hint
    // value.elem: the index of the move
    map<Position, vector<int>> m_matches;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
{
    for (auto& [p, moves] : g.m_pos2moves) {
        auto& v = m_matches[p];
        v.resize(moves.size());
        boost::iota(v, 0);
    }
    find_matches(true);
}

set<Position> puz_state::get_spaces() const
{
    set<Position> spaces;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c)
            if (Position p(r, c); cells(p) == PUZ_SPACE)
                spaces.insert(p);
    return spaces;
}

bool puz_state::check_spaces(const set<Position> spaces) const
{
    return boost::algorithm::all_of(spaces, [&](const Position& p) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            return cells(p + os) == PUZ_SPACE;
        });
    });
}

int puz_state::find_matches(bool init)
{
    auto spaces = get_spaces();
    for (auto& [p, move_ids] : m_matches) {
        auto& moves = m_game->m_pos2moves.at(p);
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [num, snake] = moves[id];
            return boost::algorithm::any_of(snake, [&](const Position& p2) {
                if (m_finished.contains(p2))
                    return true;
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != num;
            });
        });

        for (auto& [num, snake] : moves)
            for (auto& p2 : snake)
                spaces.erase(p2);

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, move_ids[0]), 1;
            }
    }
    return check_spaces(spaces) ? 2 : 0;
}

void puz_state::make_move_hint2(const Position& p, int n)
{
    auto& [num, snake] = m_game->m_pos2moves.at(p)[n];
    for (auto& p2 : snake) {
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = num, ++m_distance;
        ++m_distance;
        m_matches.erase(p2);
        m_finished.insert(p2);
    }
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    make_move_hint2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

struct puz_state4 : Position
{
    puz_state4(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state4>& children) const;

    const puz_state* m_state;
};

void puz_state4::gen_children(list<puz_state4>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; m_state->cells(p2) == PUZ_SPACE) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

struct puz_state5 : vector<Position>
{
    puz_state5(const puz_state* s, const Position& p, char num)
        : m_state(s), m_num(num) { make_move(p, false); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of_equal(*this, p);
    }

    bool is_goal_state() const { return size() == m_num - '0'; }
    void make_move(const Position& p, bool at_front) {
        at_front ? (void)insert(begin(), p) : push_back(p);
    }
    void gen_children(list<puz_state5>& children) const;
    unsigned int get_distance(const puz_state5& child) const { return 1; }

    const puz_state* m_state;
    char m_num;
};

void puz_state5::gen_children(list<puz_state5>& children) const {
    auto f = [&](const Position& p, bool at_front) {
        for (auto& os : offset) {
            auto p2 = p + os;
            if (char ch = m_state->cells(p2);
                !is_self(p2) && ch == PUZ_SPACE &&
                boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                return p3 != p && is_self(p3) || m_state->cells(p3) == m_num;
            })) {
                children.push_back(*this);
                children.back().make_move(p2, at_front);
            }
        }
    };
    if (size() > 1)
        f(front(), true);
    f(back(), false);
}

bool puz_state::make_move_hidden(char num, const vector<Position>& snake)
{
    for (auto& p : snake)
        cells(p) = num, ++m_distance;
    auto spaces = get_spaces();
    return check_spaces(spaces);
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
            if (children.push_back(*this); !children.back().make_move_hint(p, n))
                children.pop_back();
    } else {
        int i = m_cells.find(PUZ_SPACE);
        Position p(i / sidelen(), i % sidelen());
        auto smoves = puz_move_generator<puz_state4>::gen_moves({this, p});
        char num_max = smoves.size() + '0';
        for (char num = '2'; num <= num_max; ++num) {
            if (boost::algorithm::any_of(offset, [&](const Position& os) {
                return cells(p + os) == num;
            }))
                continue;
            vector<vector<Position>> snakes;
            puz_state5 sstart(this, p, num);
            list<list<puz_state5>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state5, false, false>::find_solution(sstart, spaths); found) {
                for (auto& spath : spaths) {
                    auto& s = spath.back();
                    auto v = s.front() < s.back() ? s : vector<Position>{s.rbegin(), s.rend()};
                    if (boost::algorithm::none_of_equal(snakes, v))
                        snakes.push_back(v);
                }
                for (auto& snake : snakes)
                    if (children.push_back(*this); !children.back().make_move_hidden(num, snake))
                        children.pop_back();
            }
        }
    }
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

void solve_puz_Snakeomino()
{
    using namespace puzzles::Snakeomino;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Snakeomino.xml", "Puzzles/Snakeomino.txt", solution_format::GOAL_STATE_ONLY);
}
