#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/The Grey Labyrinth

    Summary
    Maze Curator

    Description
    1. Find the walls that divide the board in a Labyrinth.
    2. The Labyrinth must have these rules:
    3. Walls can't touch each other orthogonally.
    4. From any location, there must only be one route to the treasure.
    5. You must follow the arrows, where present.
*/

namespace puzzles::TheGreyLabyrinth{

/*
    Facts
    6. A cell has a symbol in it. It can be a wall, the treasure, or it can be
       an arrow pointing to one of its four neighbors.
    7. A cell that an arrow points to cannot be a wall. It can only be an arrow cell
       that does not point to that arrow.
    8. A cell neighboring to two arrows, neither of which points to it, must be a wall.
    9. An arrow cell neighboring to an arrow that doesn't point to it must point
       to that arrow.
    10.An arrow cell neighboring to the treasure must point to the treasure.
    11.A cell neighbring to the treasure and an arrow not pointing to it must be a wall.
    12.Any 2x2 area can only have one or two walls. It cannot have 0 walls, that is,
       it can have two or three arrows.
*/

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_TRESURE_CHAR = 'T';
constexpr auto PUZ_WALL_CHAR = 'W';
constexpr auto PUZ_TRESURE = 4;
constexpr auto PUZ_WALL = 5;

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::Square2x2Offset;

const string_view dirs = "^>v<";

using puz_cell = vector<int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_cell> m_cells;
    Position m_treasure;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            switch (Position p(r, c); char ch = str[c]) {
            case PUZ_TRESURE_CHAR:
                m_treasure = p;
                m_cells.push_back({PUZ_TRESURE});
                break;
            case PUZ_SPACE:
                m_cells.push_back([&]{
                    vector<int> v = {PUZ_WALL};
                    for (int i = 0; i < 4; ++i)
                        if (auto p2 = p + offset[i]; is_valid(p2))
                            v.push_back(i);
                    return v;
                }());
                break;
            default:
                m_cells.push_back({static_cast<int>(dirs.find(ch))});
                break;
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
    const puz_cell& cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    puz_cell& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);
    bool is_interconnected() const;
    void check_arrows();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count_if(m_cells, [&](const puz_cell& cell) {
            return cell.size() == 1;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_cell> m_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    check_arrows();
}

void puz_state::check_arrows()
{
    for (;;) {
        bool finished = true;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                auto& cl = cells(p);
                int sz = cl.size();
                if (sz == 1) continue;
                for (int i = 0; i < 4; ++i) {
                    auto p2 = p + offset[i];
                    if (!is_valid(p2)) continue;
                    auto& cl2 = cells(p2);
                    if (cl2.size() != 1) continue;
                    switch (int n = cl2[0]) {
                        case PUZ_WALL:
                            boost::remove_erase_if(cl, [&](int n2) {
                                return n2 == PUZ_WALL || n2 == i;
                            });
                            break;
                        case PUZ_TRESURE:
                            boost::remove_erase_if(cl, [&](int n2) {
                                return n2 != PUZ_WALL && n2 != i;
                            });
                            break;
                        default:
                            if (auto p3 = p2 + offset[n]; p3 == p)
                                boost::remove_erase_if(cl, [&](int n2) {
                                    return n2 == PUZ_WALL || n2 == i;
                                });
                            else
                                boost::remove_erase_if(cl, [&](int n2) {
                                    return n2 != PUZ_WALL && n2 != i;
                                });
                            break;
                    }
                }
                if (sz != cl.size())
                    finished = false;
            }
        for (int r = 0; r < sidelen() - 1; ++r)
            for (int c = 0; c < sidelen() - 1; ++c) {
                Position p(r, c);
                vector<Position> rng;
                for (auto& os : offset2) {
                    auto p2 = p + os;
                    if (auto& cl = cells(p2); boost::algorithm::any_of_equal(cl, PUZ_WALL))
                        rng.push_back(p2);
                }
                if (rng.size() == 1)
                    if (auto& cl = cells(rng[0]); cl.size() > 1)
                        cl = {PUZ_WALL}, finished = false;
            }
        if (finished) break;
    }
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
//    for (auto& p : m_state->m_pos2prev.at(*this))
//        children.emplace_back(*this).make_move(p);
}

// 4. From any location, there must only be one route to the treasure.
bool puz_state::is_interconnected() const
{
//    auto smoves = puz_move_generator<puz_state2>::gen_moves(
//        {this, m_game->m_treasure});
//    return smoves.size() == boost::count_if(m_cells, [&](char ch) {
//        return ch != PUZ_WALL;
//    });
    return true;
}

bool puz_state::make_move(const Position& p, char ch)
{
    m_distance = 1;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
//    auto& [p, next] = *boost::max_element(m_pos2next, [](
//        const pair<const Position, vector<Position>>& kv1,
//        const pair<const Position, vector<Position>>& kv2) {
//        return kv1.second.size() < kv2.second.size();
//    });
//    auto f = [&](const Position& p2, char ch) {
//        if (cells(p2) == PUZ_SPACE)
//            if (!children.emplace_back(*this).make_move(p2, ch))
//                children.pop_back();
//    };
//    f(p, PUZ_WALL);
//    // 4. From any location, there must only be one route to the treasure.
//    if (next.size() > 2) {
//        for (auto& p2 : next)
//            f(p2, PUZ_WALL);
//    } else {
//        f(p, PUZ_EMPTY);
//    }
}

ostream& puz_state::dump(ostream& out) const
{
//    for (int r = 0; r < sidelen(); ++r) {
//        for (int c = 0; c < sidelen(); ++c)
//            out << cells({r, c}) << ' ';
//        println(out);
//    }
    return out;
}

}

void solve_puz_TheGreyLabyrinth()
{
    using namespace puzzles::TheGreyLabyrinth;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheGreyLabyrinth.xml", "Puzzles/TheGreyLabyrinth.txt", solution_format::GOAL_STATE_ONLY);
}
