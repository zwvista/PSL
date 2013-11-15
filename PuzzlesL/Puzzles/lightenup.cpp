#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 2/Lighten Up

	Summary
	Place lightbulbs to light up all the room squares

	Description
	1. What you see from above is a room and the marked squares are walls.
	2. The goal is to put lightbulbs in the room so that all the blank(non-wall)
	   squares are lit, following these rules.
	3. Lightbulbs light all free, unblocked squares horizontally and vertically.
	4. A lightbulb can't light another lightbulb.
	5. Walls block light. Also walls with a number tell you how many lightbulbs
	   are adjacent to it, horizontally and vertically.
	6. Walls without a number can have any number of lightbulbs. However,
	   lightbulbs don't need to be adjacent to a wall.
*/

namespace puzzles{ namespace lightenup{

#define PUZ_WALL		'W'
#define PUZ_BULB		'B'
#define PUZ_SPACE		' '
#define PUZ_LIT			'.'

Position offset[] = {
	{-1, 0},	// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},	// w
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2bulb;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			switch(char ch = str[c]){
			case PUZ_SPACE:
			case PUZ_WALL:
				m_start.push_back(ch);
				break;
			default:
				m_start.push_back(PUZ_WALL);
				m_pos2bulb[p] = ch - '0';
				break;
			}
		}
	}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start)
, m_game(&g)
{
}

bool puz_state::make_move(const Position& p)
{
	return true;
}

void puz_state::gen_children(list<puz_state> &children) const
{
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cell(Position(r, c)) << " ";
		out << endl;
	}
	return out;
}

}}

void solve_puz_lightenup()
{
	using namespace puzzles::lightenup;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\lightenup.xml", "Puzzles\\lightenup.txt", solution_format::GOAL_STATE_ONLY);
}
