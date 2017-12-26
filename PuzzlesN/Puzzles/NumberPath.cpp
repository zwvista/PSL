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

    puz_game(const vector<string>& strs, const xml_node& level);
    int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            int n = stoi(str.substr(c * 2, 2));
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
};

puz_state::puz_state(const puz_game& g)
    : vector<Position>{{}}, m_game(&g)
{
}

bool puz_state::make_move(const Position& p)
{
    int n = cells(p);
    if (n > count() || boost::algorithm::any_of(*this, [&](auto& p2) {
        return cells(p2) == n;
    }))
        return false;
    push_back(p);
    return manhattan_distance(p, {sidelen() - 1, sidelen() - 1}) <= count() - size();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (auto& os : offset) {
        auto p = back() + os;
        if (!m_game->is_valid(p)) continue;
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_lines, vert_lines;
    for (int i = 0; i < size() - 1; ++i) {
        auto &p1 = (*this)[i], &p2 = (*this)[i + 1];
        switch(boost::range::find(offset, p2 - p1) - offset) {
        case 0: vert_lines.insert(p2);  break;
        case 1: horz_lines.insert(p1);  break;
        case 2: vert_lines.insert(p1);  break;
        case 3: horz_lines.insert(p2);  break;
        }
    }

    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << format("%2d") % cells(p)
                << (horz_lines.count(p) != 0 ? '-' : ' ');
        }
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vert-lines
            out << (vert_lines.count({r, c}) != 0 ? " | " : "   ");
        out << endl;
    }
    return out;
}

}}

void solve_puz_NumberPath()
{
    using namespace puzzles::NumberPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberPath.xml", "Puzzles/NumberPath.txt", solution_format::GOAL_STATE_ONLY);
}
