#include "stdafx.h"
#include "astar_solver.h"
#include "dijkstra_solver.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

namespace puzzles::fullsearch{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_BALL        '@'
#define PUZ_GOAL        'x'
#define PUZ_WALL        '#'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    Position m_start, m_goal;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_cells.append(cols(), PUZ_WALL);
    for (int r = 1; r < rows() - 1; ++r) {
        auto& str = strs[r - 1];
        m_cells.push_back(PUZ_WALL);
        for (int c = 1; c < cols() - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            switch(ch) {
            case PUZ_BALL: m_start = p; m_cells.push_back(PUZ_SPACE); break;
            case PUZ_GOAL: m_goal = p; m_cells.push_back(ch); break;
            default: m_cells.push_back(ch); break;
            }
        }
        m_cells.push_back(PUZ_WALL);
    }
    m_cells.append(cols(), PUZ_WALL);
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g)
        : m_game(&g), m_p(g.m_start), m_move() {}
    char cells(const Position& p) const { return m_game->m_cells[p.first * cols() + p.second]; }
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool operator<(const puz_state& x) const {
        return make_pair(m_p, m_move) < make_pair(x.m_p, x.m_move);
    }
    void make_move(int i);

    // solve_puzzle interface
    bool is_goal_state() const {return m_p == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_p, m_game->m_goal);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    Position m_p;
    char m_move;
};

void puz_state::make_move(int i)
{
    static char* moves = "urdl";
    m_p += offset[i];
    m_move = moves[i];
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++) {
        auto p = m_p + offset[i];
        char ch = cells(p);
        if (ch == PUZ_SPACE || ch == PUZ_GOAL) {
            children.push_back(*this);
            children.back().make_move(i);
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 1; r < rows() - 1; ++r) {
        for (int c = 1; c < cols() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (p == m_p ? PUZ_BALL : 
                ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_fullsearch()
{
    using namespace puzzles::fullsearch;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, true, false>>(
        "Puzzles/fullsearch.xml", "Puzzles/fullsearch_astar.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_dijkstra<puz_state, true, true, false>>(
        "Puzzles/fullsearch.xml", "Puzzles/fullsearch_dijkstra.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_bfs<puz_state, true, true, false>>(
        "Puzzles/fullsearch.xml", "Puzzles/fullsearch_bfs.txt");
}
