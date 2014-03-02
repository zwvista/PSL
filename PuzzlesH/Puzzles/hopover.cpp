#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace hopover{

#define PUZ_WALL	'#'
#define PUZ_RED		'>'
#define PUZ_BLUE	'<'
#define PUZ_SPACE	' '

struct puz_game
{
	string m_id;
	string m_start, m_goal;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_start(strs[0])
	, m_goal(strs[1])
{
}

struct puz_step : pair<int, int> {
	puz_step(int n1, int n2) : pair<int, int>(n1, n2) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
	out << format("move: %1% -> %2%\n") % mi.first % mi.second;
	return out;
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g)
		: string(g.m_start), m_game(&g) {}

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		int n = 0;
		for(size_t i = 1; i < length() - 1; ++i)
			n += myabs(at(i) - m_game->m_goal.at(i));
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
	for(size_t i = 1; i < length() - 1; ++i){
		char n = at(i);
		if(n == PUZ_RED || n == PUZ_BLUE){
			bool can_move = false;
			int delta = n == PUZ_RED ? 1 : -1;
			int j = i + delta;
			char n2 = at(j);
			if(n2 == PUZ_SPACE)
				can_move = true;
			else if(n2 != PUZ_WALL && n2 != n){
				j += delta;
				n2 = at(j);
				if(n2 == PUZ_SPACE)
					can_move = true;
			}
			if(can_move){
				puz_state state = *this;
				::swap(state[i], state[j]);
				state.m_move = puz_step(i, j);
				children.push_back(state);
			}
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(size_t j = 1; j < size() - 1; ++j)
		out << at(j) << " ";
	out << endl;
	return out;
}

}}

void solve_puz_hopover()
{
	using namespace puzzles::hopover;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\hopover.xml", "Puzzles\\hopover.txt");
}