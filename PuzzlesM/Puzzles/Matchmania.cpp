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
    char m_bunny_name, m_food_name;
	Position m_origin;
	set<Position> m_food;
    int steps() const { return m_food.size() + 1; }
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    map<char, puz_bunny_info> m_name2info;
    set<Position> m_holes, m_food2;

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
            {
                auto& info = m_name2info[ch];
                info.m_bunny_name = ch;
                info.m_origin = p;
                break;
            }
            case 'c':
            {
                auto& info = m_name2info[toupper(ch)];
                info.m_food_name = ch;
                info.m_food.insert(p);
                break;
            }
            case 'm':
                m_food2.insert(p);
        		break;
            case PUZ_HOLE:
            	m_holes.insert(p);
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
    unsigned int get_heuristic() const { 
        return boost::accumulate(m_name2info, 0, [](int acc, const pair<const char, puz_bunny_info>& kv){
            return acc + kv.second.steps();
        }) + m_food2.size();
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const { if(m_move) out << *m_move; }
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<char, puz_bunny_info> m_name2info;
    set<Position> m_food2;
    unsigned int m_distance = 0;
    boost::optional<puz_step> m_move;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info) { make_move(info.m_origin); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    char ch = m_state->cells(*this);
    if(ch != m_info->m_bunny_name && ch != PUZ_SPACE) return;
    for(auto& os : offset){
        auto p = *this + os;
        ch = m_state->cells(p);
        if(ch == m_info->m_bunny_name || ch == PUZ_SPACE){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
, m_name2info(g.m_name2info), m_food2(g.m_food2)
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
