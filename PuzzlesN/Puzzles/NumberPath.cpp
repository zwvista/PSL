#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 15/Number Path

    Summary
    Tangled, Scrambled Path

    Description
    1. Connect the top left corner (1) to the bottom right corner (N), including
       all the numbers between 1 and N, only once.
*/

namespace puzzles{ namespace NumberPath{

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    vector<int> m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size())
{
    for(int r = 0; r < m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c < m_sidelen; ++c){
            Position p(r, c);
            int n = atoi(str.substr(c * 2, 2).c_str());
            m_start.push_back(n);
        }
    }
}

struct puz_state : vector<Position>
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    int cells(const Position& p) const { return m_game->cells(p); }
    int count() const { return m_game->m_start.back(); }
    bool make_move(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return count() - size(); }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<int> m_nums;
};

puz_state::puz_state(const puz_game& g)
    : vector<Position>{{}}, m_nums{g.m_start[0]}, m_game(&g)
{
}


bool puz_state::make_move(const Position& p)
{
    int n = cells(p);
    if(n > count() || boost::algorithm::any_of_equal(m_nums, n)) return false;
    push_back(p);
    return !is_goal_state() || p == Position(sidelen() - 1, sidelen() - 1);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for(auto& os : offset){
        auto p = back() + os;
        if(!m_game->is_valid(p)) continue;
        children.push_back(*this);
        if(!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c)
            out << format("%3d") % cells({r, c});
        out << endl;
    }
    return out;
}

}}

void solve_puz_NumberPath()
{
    using namespace puzzles::NumberPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\NumberPath.xml", "Puzzles\\NumberPath.txt", solution_format::GOAL_STATE_ONLY);
}
