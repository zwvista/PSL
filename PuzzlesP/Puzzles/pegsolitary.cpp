#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace pegsolitary{

#define PUZ_PEG        '$'
#define PUZ_HOLE    ' '
#define PUZ_NONE    '#'

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
    {-1, -1},
    {1, 1},
    {-1, 1},
    {1, -1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_start = string(rows() * cols(), ' ');
    fill(m_start.begin(), m_start.begin() + cols(), PUZ_NONE);
    fill(m_start.rbegin(), m_start.rbegin() + cols(), PUZ_NONE);

    int n = cols();
    for(int r = 1; r < rows() - 1; ++r, n += cols()){
        const string& str = strs[r - 1];
        m_start[n] = m_start[n + cols() - 1] = PUZ_NONE;
        for(int c = 1; c < cols() - 1; ++c)
            m_start[n + c] = str[c - 1];
    }
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

struct puz_state : public string
{
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    void make_move(const Position& p1, const Position& p2, const Position& p3) {
        cells(p1) = cells(p2) = PUZ_HOLE;
        cells(p3) = PUZ_PEG;
        m_move = puz_step(p1, p3);
    }

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 1;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        int n = 0;
        for(size_t i = 0; i < length(); i++)
            if(at(i) == PUZ_PEG)
                ++n;
        return n;
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
    for(int r = 1; r < rows() - 1; ++r)
        for(int c = 1; c < cols() - 1; ++c){
            Position p1(r, c);
            if(cells(p1) != PUZ_PEG) continue;
            for(int i = 0; i < 8; ++i){
                Position p2 = p1 + offset[i];
                Position p3 = p2 + offset[i];
                if(cells(p2) == PUZ_PEG && cells(p3) == PUZ_HOLE){
                        children.push_back(*this);
                        children.back().make_move(p1, p2, p3);
                }
            }
        }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for(int r = 1; r < rows() -1 ; ++r) {
        for(int c = 0; c < cols() - 1; ++c)
            out << cells({r, c}) << " ";
        out << endl;
    }
    return out;
}

}}

void solve_puz_pegsolitary()
{
    using namespace puzzles::pegsolitary;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/pegsolitary.xml", "Puzzles/pegsolitary.txt");
}