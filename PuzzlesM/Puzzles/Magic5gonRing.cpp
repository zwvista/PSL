#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
     Project Euler Problem 68
     Consider the following "magic" 3-gon ring, filled with the numbers 1 to 6, and each line adding to nine.
         4
          \
           3
          / \
         1 - 2 - 6
        /
       5
     Working clockwise, and starting from the group of three with the numerically lowest external node (4,3,2 in this example), each solution can be described uniquely. For example, the above solution can be described by the set: 4,3,2; 6,2,1; 5,1,3.
 
     It is possible to complete the ring with four different totals: 9, 10, 11, and 12. There are eight solutions in total.
 
     Total    Solution Set
     9    4,2,3; 5,3,1; 6,1,2
     9    4,3,2; 6,2,1; 5,1,3
     10    2,3,5; 4,5,1; 6,1,3
     10    2,5,3; 6,3,1; 4,1,5
     11    1,4,6; 3,6,2; 5,2,4
     11    1,6,4; 5,4,2; 3,2,6
     12    1,5,6; 2,6,4; 3,4,5
     12    1,6,5; 3,5,4; 2,4,6
     By concatenating each group it is possible to form 9-digit strings; the maximum string for a 3-gon ring is 432621513.
 
     Using the numbers 1 to 10, and depending on arrangements, it is possible to form 16- and 17-digit strings. What is the maximum 16-digit string for a "magic" 5-gon ring?
*/

namespace puzzles{ namespace Magic5gonRing{

struct puz_game
{
    string m_id;
    int m_gon, m_numbers, m_total;
    vector<vector<int>> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_gon(level.attribute("gon").as_int())
, m_numbers(level.attribute("numbers").as_int())
, m_total(level.attribute("total").as_int())
{
    for(int i = 1; i <= m_numbers; ++i)
        for(int j = 1; j <= m_numbers; ++j){
            if (j == i) continue;
            int k = m_total - i - j;
            if (k <= 0 || k > m_numbers || k == i || k == j) continue;
            m_perms.push_back({i, j, k});
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i);

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_gon - m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<vector<int>> m_matches;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
}

bool puz_state::make_move(int i)
{
    m_matches.push_back(m_game->m_perms[i]);
    set<int> s;
    for(auto& m : m_matches)
        for(int i : m)
            s.insert(i);
    int sz1 = m_matches.size(), sz2 = s.size();
    bool b = is_goal_state();
    if(!(b && sz2 == m_game->m_numbers || sz2 == 2 * sz1 + 1))
        return false;
    return sz1 == 1 || (*next(m_matches.rbegin()))[2] == m_matches.back()[1] &&
        m_matches.front()[0] < m_matches.back()[0] &&
        (!b || b && m_matches.back()[2] == m_matches.front()[1]);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for(int i = 0; i < m_game->m_perms.size(); ++i){
        children.push_back(*this);
        if(!children.back().make_move(i))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    out << m_game->m_total << ": ";
    for(int i = 0; i < m_matches.size(); ++i){
        auto& m = m_matches[i];
        for(int j = 0; j < m.size(); ++j){
            out << m[j];
            if (j < m.size() - 1)
                out << ",";
        }
        if (i < m_matches.size() - 1)
            out << "; ";
    }
    out << endl;
    return out;
}

}}

void solve_puz_Magic5gonRing()
{
    using namespace puzzles::Magic5gonRing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, true, false>>(
        "Puzzles/Magic5gonRing.xml", "Puzzles/Magic5gonRing.txt", solution_format::GOAL_STATE_ONLY);
}
