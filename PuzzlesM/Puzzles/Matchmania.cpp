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
#define PUZ_FOOD2        'm'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_bunny_info
{
    char m_bunny_name, m_food_name;
	Position m_bunny;
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
                info.m_bunny = p;
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
    bool make_move(const Position& p1, const Position& p2);

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
    char m_curr_bunny = 0;
    unsigned int m_distance = 0;
    boost::optional<puz_step> m_move;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info) { make_move(info.m_bunny); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    char ch = m_state->cells(*this);
    if(ch == m_info->m_food_name || ch == PUZ_FOOD2) return;
    for(auto& os : offset){
        auto p = *this + os;
        ch = m_state->cells(p);
        if(ch == m_info->m_bunny_name || ch == PUZ_SPACE ||
           ch == m_info->m_food_name || ch == PUZ_FOOD2){
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
    
bool puz_state::make_move(const Position& p1, const Position& p2)
{
    char &ch1 = cells(p1), &ch2 = cells(p2);
    if(ch2 == PUZ_HOLE){
        m_name2info.erase(ch1);
        ch1 = PUZ_SPACE;
        m_curr_bunny = 0;
    }
    else{
        m_name2info[ch2].m_food.erase(p2);
        m_curr_bunny = ch2 = exchange(ch1, PUZ_SPACE);
    }
    m_move = puz_step(p1, p2);
    m_distance = 1;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if(m_curr_bunny == 0)
        for(auto& kv : m_name2info){
            auto& info = kv.second;
            list<puz_state2> smoves;
            puz_move_generator<puz_state2>::gen_moves({*this, info}, smoves);
            smoves.remove_if([&](const Position& p){
                return info.m_food.count(p) == 0;
            });
            for(auto& p : smoves){
                children.push_back(*this);
                children.back().make_move(info.m_bunny, p);
            }
        }
    else{
        auto& info = m_name2info.at(m_curr_bunny);
        auto& p1 = info.m_bunny;
        for(auto& os : offset){
            auto p2 = p1 + os;
            char ch = cells(p2);
            if(ch == info.m_food_name || ch == PUZ_FOOD2){
                children.push_back(*this);
                children.back().make_move(p1, p2);
            }
            else if(ch == PUZ_HOLE){
                children.push_back(*this);
                children.back().make_move(p1, p2);
            }
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for(int r = 1; r < rows() - 1; ++r){
        for(int c = 1; c < cols() - 1; ++c)
            out << cells(Position(r, c));
        out << endl;
    }
    return out;
}

}}

void solve_puz_Matchmania()
{
    using namespace puzzles::Matchmania;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Matchmania.xml", "Puzzles/Matchmania.txt");
}
