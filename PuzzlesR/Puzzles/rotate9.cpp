#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::rotate9{

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start, m_goal;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2, strs[0].length())
{
    m_start = accumulate(strs.begin(), strs.begin() + rows(), string());
    m_goal = accumulate(strs.begin() + rows(), strs.end(), string());
}

struct puz_move
{
    bool m_is_row;
    int m_rc;
    int m_n;
    puz_move(bool is_row, int rc, int n)
        : m_is_row(is_row), m_rc(rc), m_n(n) {}

};

ostream & operator<<(ostream &out, const puz_move &mi)
{
    out << (mi.m_is_row ? "rotate row " : "rotate col ")
        << mi.m_rc + 1 << " ";
    if (mi.m_n == 1)
        out << "once";
    else if (mi.m_n == 2)
        out << "twice";
    else
        out << mi.m_n << " times";
    println(out);
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_start), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(int r, int c) const {return m_cells[r * cols() + c];}
    char& cells(int r, int c) {return m_cells[r * cols() + c];}
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    void rotate_row(int r, int n) {
        vector<int> v(cols());
        for (int c = 0; c < cols(); ++c)
            v[c] = cells(r, c);
        rotate(v.begin(), v.end() - n, v.end());
        for (int c = 0; c < cols(); ++c)
            cells(r, c) = v[c];
        m_move = puz_move(true, r, n);
    }
    void rotate_col(int c, int n) {
        vector<int> v(rows());
        for (int r = 0; r < rows(); ++r)
            v[r] = cells(r, c);
        rotate(v.begin(), v.end() - n, v.end());
        for (int r = 0; r < rows(); ++r)
            cells(r, c) = v[r];
        m_move = puz_move(false, c, n);
    }

    // solve_puzzle interface
    bool is_goal_state() const {return m_cells == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    boost::optional<puz_move> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    // rotate rows
    for (int r = 0; r < rows(); ++r)
        for (int n = 1; n < cols(); ++n) {
            children.push_back(*this);
            children.back().rotate_row(r, n);
        }
    // rotate cols
    for (int c = 0; c < cols(); ++c)
        for (int n = 1; n < rows(); ++n) {
            children.push_back(*this);
            children.back().rotate_col(c, n);
        }
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int md = 0;
    for (size_t i = 0; i < m_cells.size(); ++i) {
        int j = m_game->m_goal.find(m_cells[i]);
        md += myabs(i / cols() - j / cols()) + myabs(i % cols() - j % cols());
    }
    return md;
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells(r, c) << " ";
        println(out);
    }
    return out;
}

}

void solve_puz_rotate9()
{
    using namespace puzzles::rotate9;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/rotate9.xml", "Puzzles/rotate9.txt");
}
