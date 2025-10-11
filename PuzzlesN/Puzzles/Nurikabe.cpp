#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 1/Nurikabe

    Summary
    Draw a continuous wall that divides gardens as big as the numbers

    Description
    1. Each number on the grid indicates a garden, occupying as many tiles
       as the number itself.
    2. Gardens can have any form, extending horizontally and vertically, but
       can't extend diagonally.
    3. The garden is separated by a single continuous wall. This means all
       wall tiles on the board must be connected horizontally or vertically.
       There can't be isolated walls.
    4. You must find and mark the wall following these rules:
    5. All the gardens in the puzzle are numbered at the start, there are no
       hidden gardens.
    6. A wall can't go over numbered squares.
    7. The wall can't form 2*2 squares.
*/

namespace puzzles::Nurikabe{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_GARDEN = '.';
constexpr auto PUZ_ONE = '1';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_WALL_MOVE_ID = -1;

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::Square2x2Offset;

struct puz_garden
{
    // character that represents the garden. 'a', 'b' and so on
    char m_name;
    // number of the tiles occupied by the garden
    int m_num;
};

struct puz_move
{
    char m_name;
    Position m_p_hint;
    set<Position> m_area;
    set<Position> m_walls;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_garden> m_pos2garden;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, int num, const Position& p)
        : m_game(game), m_num(num) {make_move(p);}

    bool is_goal_state() const { return size() == m_num; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // Gardens extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile can be occupied by the garden
            // if it is a space tile and has not been occupied by the garden and
            if (ch2 == PUZ_SPACE && !contains(p2) && boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                // no adjacent tile to it belongs to another garden, because
                // Gardens are separated by a wall. They cannot touch each other orthogonally.
                return !contains(p3) && ch3 != PUZ_SPACE && ch3 != PUZ_BOUNDARY;
            })) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_g = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE) {
                m_cells.push_back(PUZ_SPACE);
                m_pos2move_ids[p].push_back(PUZ_WALL_MOVE_ID);
            } else {
                auto& [name, num] = m_pos2garden[p];
                num = ch - '0';
                m_cells.push_back(name = num == 1 ? PUZ_ONE : ch_g++);
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, garden] : m_pos2garden) {
        auto& [name, num] = garden;
        puz_state2 sstart(this, num, p);
        list<list<puz_state2>> spaths;
        // Gardens can have any form.
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            // save all goal states as permutations
            // A goal state is a garden formed from the number
            for (auto& spath : spaths) {
                auto& s = spath.back();
                int n = m_moves.size();
                auto& [name2, p_hint, area, walls] = m_moves.emplace_back();
                name2 = name, p_hint = p, area = s;
                // Gardens are separated by a wall.
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os; !s.contains(p3))
                            if (char ch3 = cells(p3); ch3 == PUZ_SPACE)
                                walls.insert(p3);
                for (auto& p2 : s)
                    m_pos2move_ids[p2].push_back(n);
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_interconnected() const;
    bool check_2x2();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            if (id == PUZ_WALL_MOVE_ID)
                return cells(p) != PUZ_SPACE;

            auto& [_1, p_hint, area, walls] = m_game->m_moves[id];
            return !boost::algorithm::all_of(area, [&](const Position& p2) {
                char ch2 = cells(p2);
                return p_hint == p2 || ch2 == PUZ_SPACE || ch2 == PUZ_GARDEN;
            }) || !boost::algorithm::all_of(walls, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_BOUNDARY || ch2 == PUZ_WALL;
            });
        });
        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids.front()), 1;
            }
    }
    return check_2x2() && is_interconnected() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        switch (auto p2 = *this + os;  m_state->cells(p2)) {
        case PUZ_WALL:
        case PUZ_SPACE:
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

// 5. All wall tiles on the board must be connected horizontally or vertically.
bool puz_state::is_interconnected() const
{
    int i = m_cells.find(PUZ_WALL);
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return cells(p) == PUZ_WALL;
    }) == boost::count(m_cells, PUZ_WALL);
}

// The wall can't form 2*2 squares.
bool puz_state::check_2x2()
{
    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c) {
            Position p(r, c);
            vector<Position> rng1, rng2;
            for (auto& os : offset2)
                switch(auto p2 = p + os; cells(p2)) {
                case PUZ_WALL: rng1.push_back(p2); break;
                case PUZ_SPACE: rng2.push_back(p2); break;
                }
            if (rng1.size() == 4) return false;
            if (rng1.size() == 3 && rng2.size() == 1)
                cells(rng2[0]) = PUZ_GARDEN;
        }
    return true;
}

void puz_state::make_move2(const Position& p, int n)
{
    if (n == PUZ_WALL_MOVE_ID)
        cells(p) = PUZ_WALL, ++m_distance, m_matches.erase(p);
    else {
        auto& [name, _1, area, walls] = m_game->m_moves[n];
        for (auto& p2 : area)
            cells(p2) = name, ++m_distance, m_matches.erase(p2);
        for (auto& p2 : walls)
            if (char& ch2 = cells(p2); ch2 == PUZ_SPACE)
                ch2 = PUZ_WALL, ++m_distance, m_matches.erase(p2);
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
    auto& [p, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_WALL)
                out << PUZ_WALL << ' ';
            else if (auto it = m_game->m_pos2garden.find(p);
                it == m_game->m_pos2garden.end())
                out << ". ";
            else
                out << format("{:<2}", it->second.m_num);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Nurikabe()
{
    using namespace puzzles::Nurikabe;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Nurikabe.xml", "Puzzles/Nurikabe.txt", solution_format::GOAL_STATE_ONLY);
}
