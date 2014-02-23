#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Battle Ships

	Summary
	Play solo Battleships, with the help of the numbers on the border.

	Description
	1. Standard rules of Battleships apply, but you are guessing the other
	   player ships disposition, by using the numbers on the borders.
	2. Each number tells you how many ship or ship pieces you're seeing in
	   that row or column.
	3. Standard rules apply: a ship or piece of ship can't touch another,
	   not even diagonally.
	4. In each puzzle there are
	   1 Aircraft Carrier (4 squares)
	   2 Destroyers (3 squares)
	   3 Submarines (2 squares)
	   4 Patrol boats (1 square)

	Variant
	5. Some puzzle can also have a:
	   1 Supertanker (5 squares)
*/

namespace puzzles{ namespace BattleShips{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_TOP			'^'
#define PUZ_BOTTOM		'v'
#define PUZ_LEFT		'<'
#define PUZ_RIGHT		'>'
#define PUZ_MIDDLE		'+'
#define PUZ_BOAT		'o'

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	bool m_has_supertank;
	int m_piece_total_count;
	vector<int> m_ships;
	map<Position, int> m_pos2piece;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size() - 1)
	, m_has_supertank(attrs.get<int>("SuperTank", 0) == 1)
{
	m_ships = { 1, 1, 1, 1, 2, 2, 2, 3, 3, 4 };
	if(m_has_supertank)
		m_ships.push_back(5);
	m_piece_total_count = boost::accumulate(m_ships, 0);

	for(int r = 0; r <= m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c <= m_sidelen; c++){
			Position p(r, c);
			switch(char ch = str[c]){
			case PUZ_SPACE:
			case PUZ_EMPTY:
				if(r != m_sidelen || c != m_sidelen)
					m_start.push_back(ch);
				break;
			case PUZ_BOAT:
			case PUZ_TOP:
			case PUZ_BOTTOM:
			case PUZ_LEFT:
			case PUZ_RIGHT:
			case PUZ_MIDDLE:
				m_start.push_back(PUZ_SPACE);
				m_pos2piece[p] = ch;
				break;
			default:
				if(c == m_sidelen)
					m_piece_counts_rows.push_back(ch - '0');
				else
					m_piece_counts_cols.push_back(ch - '0');
				break;
			};
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
	char cells(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { 
		return 1;
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
};

puz_state::puz_state(const puz_game& g)
{
}

bool puz_state::make_move(const Position& p)
{
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			char ch = cells(p);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_BattleShips()
{
	using namespace puzzles::BattleShips;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\BattleShips.xml", "Puzzles\\BattleShips.txt", solution_format::GOAL_STATE_ONLY);
}
