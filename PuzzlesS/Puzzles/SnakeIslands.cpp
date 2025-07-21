#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 3/Snake Islands

    Summary
    Snakes and Nurikabes

    Description
    1. A mix between Nurikabe and Snake, with some rules from one game and
       some from the other.
    2. Nurikabe Rules: Each number on the grid indicates a garden, occupying
       as many tiles as the number itself.
    3. Gardens can have any form, extending horizontally and vertically but
       can't extend diagonally.
    4. The gardens are separated by a single, continuous wall. This means all
       wall tiles on the board must be connected horizontally or vertically. There can't be isolated walls
    5. Additionally, not all the gardens in the puzzle may be numbered at the
       start. There could be some hidden gardens.
    6. The wall can't form 2x2 squares.
    7. Snake Rules: Two circles are the head and tail of the snake
       (it is irrelevant which is which).
    8. A snake can't touch its body orthogonally, but it CAN touch itself
       diagonally. However a snake head can touch its tail.
    9. Snakes cannot cross each other.
*/

namespace puzzles::SnakeIslands{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_ONE = '1';
constexpr auto PUZ_SNAKE = 'S';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};
constexpr Position offset2[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

struct puz_garden
{
    // character that represents the garden. 'a', 'b' and so on
    char m_name;
    // number of the tiles occupied by the garden
    int m_num;
    // all permutations (forms) of the garden
    vector<set<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_garden> m_pos2garden;
    string m_cells;
    set<Position> m_snakes;

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
                return !contains(p3) && ch3 != PUZ_SPACE && ch3 != PUZ_BOUNDARY && ch3 != PUZ_SNAKE;
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
        for (int c = 1; c < m_sidelen - 1; ++c)
            switch (Position p(r, c); char ch = str[c - 1]) {
            case PUZ_SPACE:
            case PUZ_SNAKE:
                m_cells.push_back(ch);
                if (ch == PUZ_SNAKE)
                    m_snakes.insert(p);
                break;
            default:
                int n2 = ch - '0';
                auto& garden = m_pos2garden[p];
                garden.m_num = n2;
                m_cells.push_back(garden.m_name = n2 == 1 ? PUZ_ONE : ch_g++);
            }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, garden] : m_pos2garden) {
        auto& [_1, num, perms] = garden;
        if (num == 1)
            perms = {{p}};
        else {
            puz_state2 sstart(this, num, p);
            list<list<puz_state2>> spaths;
            // Gardens can have any form.
            if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                // save all goal states as permutations
                // A goal state is a garden formed from the number
                for (auto& spath : spaths)
                    perms.push_back(spath.back());
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
    bool make_move2(Position p, int n);
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
    // key: the position of the hint
    // value: index into the permutations (forms) of the garden
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    for (auto& [p, garden] : g.m_pos2garden) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(garden.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    // key: a space tile
    // value.elem: position of the number from which a garden
    //             containing that space tile can be formed
    map<Position, set<Position>> space2hints;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SPACE || ch == PUZ_EMPTY)
                space2hints[p];
        }

    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_pos2garden.at(p).m_perms;
        // remove any path if it contains a tile which belongs to another garden
        boost::remove_erase_if(perm_ids, [&](int id) {
            return boost::algorithm::any_of(perms[id], [&](const Position& p2) {
                char ch = cells(p2);
                return p != p2 && ch != PUZ_SPACE && ch != PUZ_EMPTY;
            });
        });
        for (int id : perm_ids)
            for (auto& p2 : perms[id])
                if (p2 != p)
                    space2hints.at(p2).insert(p);

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    bool changed = false;
    for (auto& [p, h] : space2hints) {
        if (!h.empty()) continue;
        // Cells that cannot be reached by any garden can be nothing but a wall
        char& ch = cells(p);
        if (ch == PUZ_EMPTY)
            return false;
        ch = PUZ_SNAKE;
        changed = true;
    }
    if (changed) {
        if (!check_2x2()) return 0;
        for (auto& [p, h] : space2hints) {
            if (h.size() != 1) continue;
            char ch = cells(p);
            if (ch == PUZ_SPACE) continue;
            // Cells that can be reached by only one garden
            auto& p2 = *h.begin();
            auto& perms = m_game->m_pos2garden.at(p2).m_perms;
            // remove any permutation that does not contain the empty tile
            boost::remove_erase_if(m_matches.at(p2), [&](int id) {
                return boost::algorithm::none_of_equal(perms[id], p);
            });
        }
        if (!init) {
            for (auto& [p, perm_ids] : m_matches)
                if (perm_ids.size() == 1)
                    return make_move2(p, perm_ids.front()) ? 1 : 0;
            if (!is_interconnected())
                return 0;
        }
    }
    return 2;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

// All wall tiles on the board must be connected horizontally or vertically.
bool puz_state::is_interconnected() const
{
    set<Position> a;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SPACE || ch == PUZ_SNAKE)
                a.insert(p);
        }

    auto smoves = puz_move_generator<puz_state3>::gen_moves(a);
    return smoves.size() == a.size();
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
                case PUZ_SNAKE: rng1.push_back(p2); break;
                case PUZ_SPACE: rng2.push_back(p2); break;
                }
            if (rng1.size() == 4) return false;
            if (rng1.size() == 3 && rng2.size() == 1)
                cells(rng2[0]) = PUZ_EMPTY;
        }
    return true;
}

bool puz_state::make_move2(Position p, int n)
{
    auto& garden = m_game->m_pos2garden.at(p);
    auto& perm = garden.m_perms[n];

    for (auto& p2 : perm)
        cells(p2) = garden.m_name;
    for (auto& p2 : perm)
        // Gardens are separated by a wall.
        for (auto& os : offset)
            switch(char& ch2 = cells(p2 + os)) {
            case PUZ_SPACE: ch2 = PUZ_SNAKE; break;
            case PUZ_EMPTY: return false;
            }
    ++m_distance;
    m_matches.erase(p);

    return check_2x2() && is_interconnected();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](auto& kv1, auto& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SNAKE)
                out << PUZ_SNAKE << ' ';
            else {
                auto it = m_game->m_pos2garden.find(p);
                if (it == m_game->m_pos2garden.end())
                    out << ". ";
                else
                    out << format("{:<2}", it->second.m_num);
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_SnakeIslands()
{
    using namespace puzzles::SnakeIslands;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SnakeIslands.xml", "Puzzles/SnakeIslands.txt", solution_format::GOAL_STATE_ONLY);
}
