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

struct puz_snake
{
    char m_num;
    vector<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_hints, m_cells;

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
    void make_move(const Position& p, bool at_front) { push_back(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    char m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    auto f = [&](const Position& p, bool at_front) {
        for (int i = 0; i < 4; ++i)
            if (auto p2 = p + offset[i];
                m_game->cells(p2) == PUZ_SPACE && !is_self(p2) &&
                boost::algorithm::none_of(offset, [&](const Position& os) {
                    auto p3 = p2 + os;
                    return p3 != p && is_self(p3);
                    })) {
            children.push_back(*this);
            children.back().make_move(p2, at_front);
        }
    };
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
            hints(p) = str[c * 2 - 2], cells(p) = str[c * 2 - 1];
        }
    }

}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
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

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the hint
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, area_ids] : m_matches) {
        //boost::remove_erase_if(area_ids, [&](int id) {
        //    auto& [rng, _2, rng2D] = m_game->m_areas[id];
        //    return !boost::algorithm::all_of(rng2D, [&](const set<Position>& rng2) {
        //        return boost::algorithm::all_of(rng2, [&](const Position& p2) {
        //            return cells(p2) == PUZ_SPACE ||
        //                boost::algorithm::any_of_equal(rng, p2);
        //        });
        //    });
        //});

        if (!init)
            switch(area_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    //auto& [rng, names, rng2D] = m_game->m_areas[n];
    //for (int i = 0; i < names.size(); ++i) {
    //    auto& rng2 = rng2D[i];
    //    char ch2 = names[i];
    //    for (auto& p2 : rng2)
    //        cells(p2) = ch2, ++m_distance, m_matches.erase(p2);
    //}
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
    auto& [p, area_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : area_ids)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
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
