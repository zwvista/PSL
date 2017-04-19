#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Matchmania
*/

namespace puzzles{ namespace Matchmania{

#define PUZ_SPACE        ' '
#define PUZ_HOLE         'O'
#define PUZ_STONE        'S'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_bunny_info
{
	Position m_origin;
	vector<Position> m_food;
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    map<char, puz_bunny_info> m_name2info;
    vector<Position> m_holes;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
	, m_size(strs.size() + 2, strs[0].size() + 2)
{
    m_start.append(cols(), PUZ_STONE);
    for(int r = 1; r < rows() - 1; ++r){
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_STONE);
        for(int c = 1; c < cols() - 1; ++c){
        	Position p(r, c);
        	char ch = str[c - 1];
        	m_start.push_back(ch);
        	switch(ch){
            case 'C':
            	m_name2info[ch].m_origin = p;
            	break;
            case 'c':
            	m_name2info[toupper(ch)].m_food.push_back(p);
        		break;
            case PUZ_HOLE:
            	m_holes.push_back(p);
        		break;
            }
        }
        m_start.push_back(PUZ_STONE);
    }
    m_start.append(cols(), PUZ_STONE);
}

struct puz_step : pair<Position, Position>
{
    using pair::pair;
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: %1% -> %2%\n") % mi.first % mi.second;
    return out;
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return 1; }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const { if(m_move) out << *m_move; }
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    unsigned int m_distance = 0;
    boost::optional<puz_step> m_move;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
}

void puz_state::gen_children(list<puz_state>& children) const
{
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    return out;
}

}}

void solve_puz_Matchmania()
{
    using namespace puzzles::Matchmania;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Matchmania.xml", "Puzzles/Matchmania.txt");
}
