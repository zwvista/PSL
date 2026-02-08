#include "stdafx.h"
#include "astar_solver.h"
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

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TRESURE = 'T';
constexpr auto PUZ_WALL = 'W';

constexpr array<Position, 4> offset = Position::Directions4;

const string_view dirs = "^>v<";

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    Position m_treasure;
    map<Position, vector<Position>> m_pos2next, m_pos2prev;
    map<Position, Position> m_arrow2pos;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            switch (Position p(r, c); char ch = cells(p)) {
            case PUZ_TRESURE:
                m_treasure = p;
                m_pos2next[p].push_back(p);
                break;
            case PUZ_SPACE:
                break;
            default:
                {
                    auto& os = offset[dirs.find(ch)];
                    auto p2 = p + os;
                    m_arrow2pos[p] = p2;
                    m_pos2next[p].push_back(p2);
                }
                break;
            }
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (auto& next = m_pos2next[p]; next.empty())
                for (auto& os : offset) {
                    auto p2 = p + os;
                    auto it = m_arrow2pos.find(p2);
                    if (bool is_arrow = it != m_arrow2pos.end();
                        is_arrow && it->second != p || !is_arrow && is_valid(p2))
                        next.push_back(p2);
                }
        }
    for (auto& [p, next] : m_pos2next)
        for (auto& p2 : next)
            m_pos2prev[p2].push_back(p);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<Position>> m_pos2next, m_pos2prev;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_pos2next(g.m_pos2next)
, m_pos2prev(g.m_pos2prev)
{
    for (auto& [p, next] : m_pos2next)
        if (cells(p) != PUZ_SPACE)
            m_pos2next.erase(p);
}

bool puz_state::make_move(const Position& p, char ch)
{
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, next] = *boost::max_element(m_pos2next, [](
        const pair<const Position, vector<Position>>& kv1,
        const pair<const Position, vector<Position>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    if (!children.emplace_back(*this).make_move(p, PUZ_WALL))
        children.pop_back();
    if (next.size() > 2) {
        for (auto& p2 : next)
            if (!children.emplace_back(*this).make_move(p2, PUZ_WALL))
                children.pop_back();
    } else {
        if (!children.emplace_back(*this).make_move(p, PUZ_EMPTY))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_TheGreyLabyrinth()
{
    using namespace puzzles::TheGreyLabyrinth;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheGreyLabyrinth.xml", "Puzzles/TheGreyLabyrinth.txt", solution_format::GOAL_STATE_ONLY);
}
