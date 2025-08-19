#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Water pouring puzzle

    Summary
    Water pouring puzzle
      
    Description
*/

namespace puzzles::WaterSort{

constexpr auto PUZ_SPACE = ' ';

using puz_tube = vector<string>;

struct puz_game
{
    string m_id;
    Position m_size;
    vector<puz_tube> m_tubes;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

struct puz_move : pair<int, int> { using pair::pair; };

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size(), strs[0].length()))
    , m_tubes(cols())
{
    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c)
            if (char ch = str[c]; ch == PUZ_SPACE)
                ;
            else if (m_tubes[c].empty() || m_tubes[c].back().back() != ch)
                m_tubes[c].push_back(string(1, ch));
            else
                m_tubes[c].back().push_back(ch);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const { 
        return pair{m_quantities, m_move} < pair{x.m_quantities, x.m_move};
    }
    void make_move(int i, int j);

    //solve_puzzle interface
    bool is_goal_state() const {
        return true;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return is_goal_state() ? 0 : 1; }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_quantities;
    boost::optional<puz_move> m_move;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g)
{
}

void puz_state::make_move(int i, int j)
{
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < m_quantities.size(); i++)
        for (int j = 0; j < m_quantities.size(); j++) {
            if (i == j) continue;
            children.push_back(*this);
            children.back().make_move(i, j);
        }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move) {
    }
    println(out);
    return out;
}

}

void solve_puz_WaterSort()
{
    using namespace puzzles::WaterSort;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WaterSort.xml", "Puzzles/WaterSort.txt");
}
