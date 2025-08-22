#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

namespace puzzles::sokoban{

constexpr auto PUZ_MAN = '@';
constexpr auto PUZ_MAN_GOAL = '+';
constexpr auto PUZ_FLOOR = ' ';
constexpr auto PUZ_GOAL = '.';
constexpr auto PUZ_BOX = '$';
constexpr auto PUZ_BOX_GOAL = '*';

constexpr array<Position, 4> offset = Position::Directions4;

using group_map = unordered_map<char, vector<int> >;

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    Position m_man;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
    size_t i = m_cells.find(PUZ_MAN);
    m_man = Position(i / cols(), i % cols());
    m_cells[i] = ' ';
}

struct puz_state_base
{
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}

    const puz_game* m_game;
    Position m_man;
    string m_move;
};

struct puz_state2;

struct puz_state : puz_state_base
{
    puz_state(const puz_game& g) : m_cells(g.m_cells) {
        m_game = &g, m_man = g.m_man;
    }
    puz_state(const puz_state2& x2);
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_man < x.m_man ||
            m_cells == x.m_cells && m_man == x.m_man && m_move < x.m_move;
    }
    void make_move(const Position& p1, const Position& p2, char dir) {
        cells(p1) = cells(p1) == PUZ_BOX ? PUZ_FLOOR : PUZ_GOAL;
        cells(p2) = cells(p2) == PUZ_FLOOR ? PUZ_BOX : PUZ_BOX_GOAL;
        m_man = p1;
        m_move += dir;
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(!m_move.empty()) out << m_move;}
    ostream& dump(ostream& out) const;

    string m_cells;
};

struct puz_state2 : puz_state_base
{
    puz_state2(const puz_state& s) : m_cells(&s.m_cells) {
        m_game = s.m_game, m_man = s.m_man;
    }
    char cells(const Position& p) const {return (*m_cells)[p.first * cols() + p.second];}
    bool operator<(const puz_state2& x) const {
        return m_man < x.m_man;
    }
    void make_move(const Position& p, char dir) {
        m_man = p, m_move += dir;
    }
    void gen_children(list<puz_state2>& children) const;

    const string* m_cells;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    static string_view dirs = "urdl";
    for (int i = 0; i < 4; ++i) {
        Position p = m_man + offset[i];
        char ch = cells(p);
        if (ch == PUZ_FLOOR || ch == PUZ_GOAL) {
            children.push_back(*this);
            children.back().make_move(p, dirs[i]);
        }
    }
}

puz_state::puz_state(const puz_state2& x2)
    : m_cells(*x2.m_cells)
{
    m_game = x2.m_game, m_man = x2.m_man, m_move = x2.m_move;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    static string_view dirs = "URDL";
    auto smoves = puz_move_generator<puz_state2>::gen_moves(*this);
    for (const puz_state2& s : smoves)
        for (int i = 0; i < 4; ++i) {
            Position p1 = s.m_man + offset[i];
            char ch = cells(p1);
            if (ch != PUZ_BOX && ch != PUZ_BOX_GOAL) continue;
            Position p2 = p1 + offset[i];
            ch = cells(p2);
            if (ch != PUZ_FLOOR && ch != PUZ_GOAL) continue;
            children.push_back(s);
            children.back().make_move(p1, p2, dirs[i]);
        }
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int md = 0;
    int i = -1, j = -1;
    for (;;) {
        i = m_cells.find(PUZ_BOX, i + 1);
        if (i == -1) break;
        j = m_cells.find(PUZ_GOAL, j + 1);
        md += myabs(i / cols() - j / cols()) + myabs(i % cols() - j % cols());
    }
    return md;
}

ostream& puz_state::dump(ostream& out) const
{
    if (!m_move.empty())
        out << "move: " << m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            char ch = cells({r, c});
            out << (p == m_man ? 
                ch == PUZ_FLOOR ? PUZ_MAN : PUZ_MAN_GOAL : ch);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_sokoban()
{
    using namespace puzzles::sokoban;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/sokoban.xml", "Puzzles/sokoban.txt");
}
