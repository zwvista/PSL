#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "idastar_solver2.h"
#include "solve_puzzle.h"

namespace puzzles::_8puzzle{

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

typedef unordered_map<char, pair<vector<int>, vector<int> > > group_map;

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start, m_goal;
    Position m_space;
    mutable group_map m_groups;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    void compute_heuristic();
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2, strs[0].length())
{
    m_start = accumulate(strs.begin(), strs.begin() + rows(), string());
    m_goal = accumulate(strs.begin() + rows(), strs.end(), string());
    int n = m_start.find(' ');
    m_space = Position(n / cols(), n % cols());
    compute_heuristic();
}

void puz_game::compute_heuristic()
{
    for (size_t i = 0; i < m_goal.size(); ++i)
        m_groups[m_goal[i]].first.push_back(i);
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g), m_space(g.m_space), m_move(0) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    void make_move(const Position& p, char dir) {
        cells(m_space) = cells(p);
        cells(m_space = p) = ' ';
        m_move = dir;
    }

    //solve_puzzle interface
    bool is_goal_state() const {return *this == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    Position m_space;
    char m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    const string_view dirs = "wens";
    for (int i = 0; i < 4; ++i) {
        Position p = m_space + offset[i];
        if (is_valid(p)) {
            children.push_back(*this);
            children.back().make_move(p, dirs[i]);
        }
    }
}

//unsigned int puz_state::get_heuristic() const
//{
//    unsigned int md = 0;
//    for (size_t i = 0; i < size(); ++i) {
//        size_t j = m_game->m_goal.find(operator[](i));
//        md += myabs(j / cols() - i / cols()) + myabs(j % cols() - i % cols());
//    }
//    return md;
//}

// This heuristic function may overestimate the cost,
// meaning that it may not be admissible,
// so an optimal solution may not be found.
unsigned int puz_state::get_heuristic() const
{
    unsigned int md = 0;

    group_map& g = m_game->m_groups;
    for (size_t i = 0; i < size(); ++i)
        g[at(i)].second.push_back(i);
    for (group_map::iterator i = g.begin(); i != g.end(); ++i) {
        vector<int>& v0 = i->second.first;
        vector<int>& v1 = i->second.second;
        for (size_t j = 0; j < v0.size(); ++j)
            md += myabs(v0[j] / cols() - v1[j] / cols()) + myabs(v0[j] % cols() - v1[j] % cols());
        v1.clear();
    }

    return md;
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_8puzzle()
{
    using namespace puzzles::_8puzzle;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/8puzzle.xml", "Puzzles/8puzzle.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/8puzzle.xml", "Puzzles/8puzzle.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
