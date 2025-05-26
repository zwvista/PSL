// http://acm.pku.edu.cn/JudgeOnline/problem?id=2286

#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::rotationgame{

struct puz_game
{
    string m_id;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_start(boost::erase_all_copy(strs[0], " "))
{
}

struct puz_state : string
{
    puz_state(const puz_game& g)
        : string(g.m_start), m_move(0) {}
    void make_rotation(int n, bool reverse, char m);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    char m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    static const char* moves = "AFBEHCGD";
    for (int i = 0; i < 8; ++i) {
        children.push_back(*this);
        children.back().make_rotation(i / 2, i % 2 == 0, moves[i]);
    }
}

void puz_state::make_rotation(int n, bool reverse, char m)
{
    //        0     1
    //        2     3
    //  4  5  6  7  8  9 10
    //       11    12  
    // 13 14 15 16 17 18 19
    //       20    21
    //       22    23
    static int offset[4][7] = {
        {0, 2, 6, 11, 15, 20, 22},
        {1, 3, 8, 12, 17, 21, 23},
        {4, 5, 6, 7, 8, 9, 10},
        {13, 14, 15, 16, 17, 18, 19},
    };
    static vector<char> v(7);
    for (int i = 0; i < 7; ++i)
        v[i] = (*this)[offset[n][i]];
    rotate(v.begin(), reverse ? v.begin() + 1 : v.end() - 1, v.end());
    for (int i = 0; i < 7; ++i)
        (*this)[offset[n][i]] = v[i];
    m_move = m;
}

unsigned int puz_state::get_heuristic() const
{
    static int offset[8] = {6, 7, 8, 11, 12, 15, 16, 17};
    static vector<char> v(8);
    for (int i = 0; i < 8; ++i)
        v[i] = (*this)[offset[i]];
    map<int, char> groups;
    for (char ch = '1'; ch <= '3'; ++ch)
        groups[boost::count(v, ch)] = ch;
    char chMax = groups.rbegin()->second;
    unsigned int md = boost::accumulate(v, 0, arg1 + phx::bind(&myabs, chMax - arg2));
    return md;
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (size_t i = 0; i < length(); ++i)
        print("{} ", at(i));
    println();
    return out;
}

}

void solve_puz_rotationgame()
{
    using namespace puzzles::rotationgame;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/rotationgame.xml", "Puzzles/rotationgame.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
