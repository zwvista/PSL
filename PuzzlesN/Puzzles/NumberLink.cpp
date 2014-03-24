#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/NumberLink

	Summary
	Connect the same numbers without the crossing paths

	Description
	1. Connect the couples of equal numbers (i.e. 2 with 2, 3 with 3 etc)
	   with a continuous line.
	2. The line can only go horizontally or vertically and can't cross
	   itself or other lines.
	3. Lines must originate on a number and must end in the other equal
	   number.
	4. At the end of the puzzle, you must have covered ALL the squares with
	   lines and no line can cover a 2*2 area (like a 180 degree turn).
	5. In other words you can't turn right and immediately right again. The
	   same happens on the left, obviously. Be careful not to miss this rule.

	Variant
	6. In some levels there will be a note that tells you don't need to cover
	   all the squares.
	7. In some levels you will have more than a couple of the same number.
	   In these cases, you must connect ALL the same numbers together.
*/

namespace puzzles{ namespace NumberLink{

#define PUZ_SPACE		' '
#define PUZ_NUMBER		'N'
#define PUZ_BOUNDARY	'+'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<char, vector<Position>> m_ch2path;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(ch);
			else{
				m_start.push_back(PUZ_NUMBER);
				m_ch2path[ch].push_back({r, c});
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, char ch);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return 1; }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
}

bool puz_state::make_move(const Position& p, char ch)
{
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c)
			out << cells({r, c}) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_NumberLink()
{
	using namespace puzzles::NumberLink;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\NumberLink.xml", "Puzzles\\NumberLink.txt", solution_format::GOAL_STATE_ONLY);
}