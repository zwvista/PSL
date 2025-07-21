#include "stdafx.h"
#include "astar_solver.h"
#include "dijkstra_solver.h"
#include "bfs_solver.h"
#include "idastar_solver.h"
#include "idastar_solver2.h"
#include "dfs_solver.h"
#include "solve_puzzle.h"

namespace puzzles::pathfind{

enum EDir {mvLeft, mvRight, mvUp, mvDown};

string_view moves = "lrud";

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    string m_id;
    Position m_size;
    Position m_start, m_goal;
    set<Position> m_horz_wall;
    set<Position> m_vert_wall;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_horz_wall(const Position& p) const {return m_horz_wall.contains(p);}
    bool is_vert_wall(const Position& p) const {return m_vert_wall.contains(p);}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2, strs[0].length() / 2)
{
    for (int r = 0; ; ++r) {
        string_view hstr = strs[2 * r];
        for (size_t i = 0; i < hstr.length(); ++i)
            if (hstr[i] == '-')
                m_horz_wall.insert(Position(r, i / 2));

        if (r == rows()) break;

        string_view vstr = strs[2 * r + 1];
        for (size_t i = 0; i < vstr.length(); ++i)
            switch(Position p(r, i / 2); vstr[i]) {
            case '|': m_vert_wall.insert(p); break;
            case '@': m_start = p; break;
            case '.': m_goal = p; break;
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_p(g.m_start), m_game(&g) {}
    bool operator<(const puz_state& x) const { return m_p < x.m_p; }
    bool operator==(const puz_state& x) const { return m_p == x.m_p; }
    bool make_move(EDir dir);

    // solve_puzzle interface
    bool is_goal_state() const {return m_p == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_p, m_game->m_goal);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    Position m_p;
    char m_move = 0;
};

bool puz_state::make_move(EDir dir)
{
    Position p = m_p + offset[dir];
    if (dir == mvLeft && m_game->is_vert_wall(m_p) ||
        dir == mvRight && m_game->is_vert_wall(p) ||
        dir == mvUp && m_game->is_horz_wall(m_p) ||
        dir == mvDown && m_game->is_horz_wall(p))
        return false;

    m_p = p;
    m_move = moves[dir];
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++)
        if (children.push_back(*this); !children.back().make_move(EDir(i)))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 0; ; ++r) {
        // draw horz wall
        for (int c = 0; c < m_game->cols(); ++c)
            out << (m_game->is_horz_wall(Position(r, c)) ? " -" : "  ");
        println(out);
        if (r == m_game->rows()) break;
        for (int c = 0; ; ++c) {
            Position pos(r, c);
            // draw vert wall
            out << (m_game->is_vert_wall(pos) ? "|" : " ");
            if (c == m_game->cols()) break;
            // draw balls and goals
            out << (pos == m_p ? '@' :
                pos == m_game->m_goal ? '.' : ' ');
        }
        println(out);
    }
    return out;
}

}

void solve_puz_pathfind()
{
    using namespace puzzles::pathfind;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_astar.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_dijkstra<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_dijkstra.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_bfs<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_bfs.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_idastar.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_idastar2<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_idastar2.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_dfs<puz_state>>(
        "Puzzles/pathfind.xml", "Puzzles/pathfind_dfs.txt");
}
