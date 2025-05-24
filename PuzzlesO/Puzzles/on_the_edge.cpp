#include "stdafx.h"
#include "astar_solver.h"
#include "dfs_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

namespace puzzles::on_the_edge{

#define PUZ_HOLE        '#'
#define PUZ_BLACK        '2'
#define PUZ_WHITE        ' '
#define PUZ_BLOCK        '@'
#define PUZ_GOAL        '.'

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    string m_id;
    Position m_size;
    Position m_block, m_goal;
    string m_cells;
    map<Position, Position> m_teleporters;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0, n = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c, ++n)
            switch(char& ch = m_cells[n]) {
            case PUZ_BLOCK:
                ch = PUZ_WHITE;
                m_block = Position(r, c);
                break;
            case PUZ_GOAL:
                ch = PUZ_WHITE;
                m_goal = Position(r, c);
                break;
            }
    for (auto v : level.children())
        if (string(v.name()) == "teleporter") {
            Position p1, p2;
            parse_position(v.attribute("position1").value(), p1);
            parse_position(v.attribute("position2").value(), p2);
            m_teleporters[p1] = p2;
            m_teleporters[p2] = p1;
        }
}

struct puz_state_base
{
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }

    const puz_game* m_game = nullptr;
    Position m_block;
};

struct puz_state : puz_state_base
{
    puz_state() {}
    puz_state(const puz_game& g) : m_cells(g.m_cells), m_move(0) {
        m_game = &g, m_block = g.m_block;
    }
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells ||
            m_cells == x.m_cells && m_block < x.m_block;
    }
    bool make_move(int i);

    //solve_puzzle interface
    bool is_goal_state() const {
        return m_block == m_game->m_goal && get_heuristic() == 1;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        unsigned int n = 0;
        for (char ch : m_cells)
            n += ch == PUZ_WHITE ? 1 : ch == PUZ_BLACK ? 2 : 0;
        return n;
    }
    unsigned int get_spaces() const {
        return boost::count_if(m_cells, arg1 == PUZ_WHITE || arg1 == PUZ_BLACK);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    string m_cells;
    char m_move;
};

struct puz_state2 : puz_state_base
{
    puz_state2(const puz_state& s) : m_cells(&s.m_cells) {
        m_game = s.m_game, m_block = s.m_block;
    }
    char cells(const Position& p) const {return (*m_cells)[p.first * cols() + p.second];}
    bool operator<(const puz_state2& x) const {
        return m_block < x.m_block;
    }
    void gen_children(list<puz_state2>& children) const;

    const string* m_cells;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    map<Position, Position>::const_iterator it = m_game->m_teleporters.find(m_block);
    if (it != m_game->m_teleporters.end() && cells(it->second) != PUZ_HOLE) {
        children.push_back(*this);
        children.back().m_block = it->second;
    }
    for (int i = 0; i < 4; ++i) {
        Position p = m_block + offset[i];
        if (is_valid(p) && cells(p) != PUZ_HOLE) {
            children.push_back(*this);
            children.back().m_block = p;
        }
    }
}

bool puz_state::make_move(int i)
{
    const string_view dirs = "lrud";

    if (m_block != m_game->m_goal)
        cells(m_block) = cells(m_block) == PUZ_BLACK ? PUZ_WHITE : PUZ_HOLE;

    if (!is_valid(m_block += offset[i]) ||
        cells(m_block) == PUZ_HOLE) return false;

    map<Position, Position>::const_iterator it = m_game->m_teleporters.find(m_block);
    if (it != m_game->m_teleporters.end()) {
        cells(m_block) = PUZ_HOLE;
        m_block = it->second;
    }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    if (get_spaces() != smoves.size()) return false;

    m_move = dirs[i];
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i) {
        children.push_back(*this);
        if (!children.back().make_move(i))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            out << (p == m_block ? PUZ_BLOCK :
            p == m_game->m_goal ? PUZ_GOAL :
            cells(p)) << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_on_the_edge()
{
    using namespace puzzles::on_the_edge;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/on_the_edge.xml", "Puzzles/on_the_edge.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_dfs<puz_state>>(
        "Puzzles/on_the_edge.xml", "Puzzles/on_the_edge_dfs.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
