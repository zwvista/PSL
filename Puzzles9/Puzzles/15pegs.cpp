// Description: Traditional 5-on-a-side Triangle Peg Solitaire.
// 15 peg holes are in a triangular hex grid as follows: 
//
//        0 
//       / \
//      1---2
//     / \ / \ 
//    3---4---5
//   / \ / \ / \
//  6---7---8---9 
// / \ / \ / \ / \
//10--11--12--13--14
//
// Initial State: All 15 holes have pegs except for one central vacant hole (position 4).

// Operators: Peg removal via linear jump.  
// A peg which jumps over an adjacent peg to an empty peg hole immediately 
// beyond in the same direction results in the removal of the jumped peg.  
// For example, in the initial state, the peg at 13 could jump the peg at 8
// on its way to vacant position 4.  This results in the removal of the peg at 8. 
// Afterwards, there is a peg at 4, and positions 8 and 13 are vacant. 
//
// Goal: Exactly one peg remains after all others have been removed.

#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::_15pegs{

#define PUZ_PEG        '1'
#define PUZ_SPACE    '0'

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
    {-1, -1},
    {1, 1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_start = boost::accumulate(strs, string());
}

struct puz_step
{
    Position m_p1, m_p2;
    puz_step(const Position& p1, const Position& p2)
        : m_p1(p1), m_p2(p2) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: %1% => %2%\n") % mi.m_p1 % mi.m_p2;
    return out;
}

class puz_state : public string
{
public:
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second <= p.first;
    }
    void make_move(const Position& p1, const Position& p2, const Position& p3) {
        cells(p1) = cells(p2) = PUZ_SPACE;
        cells(p3) = PUZ_PEG;
        m_move = puz_step(p1, p3);
    }

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 1;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count_if(*this, arg1 == PUZ_PEG);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    boost::optional<puz_step> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c <= r; ++c) {
            Position p1(r, c);
            if (cells(p1) == PUZ_SPACE) continue;
            for (int i = 0; i < 6; ++i) {
                Position p2 = p1 + offset[i];
                Position p3 = p2 + offset[i];
                if (is_valid(p2) && cells(p2) == PUZ_PEG &&
                    is_valid(p3) && cells(p3) == PUZ_SPACE) {
                        children.push_back(*this);
                        children.back().make_move(p1, p2, p3);
                }
            }
        }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int i = 0; i < cols()- r - 1; ++i)
            out << " ";
        for (int c = 0; c <= r; ++c)
            out << cells({r, c}) << " ";
        out << endl;
    }
    return out;
}

}

void solve_puz_15pegs()
{
    using namespace puzzles::_15pegs;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/15pegs.xml", "Puzzles/15pegs.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/15pegs.xml", "Puzzles/15pegs.txt");
}
