#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

#define PUZ_ON		'1'
#define PUZ_OFF		'0'

namespace puzzles{ namespace lightsout_int{

const Position offset[] = {
	{0, 0},
	{0, -1},
	{0, 1},
	{-1, 0},
	{1, 0},
};

struct puz_game
{
	string m_id;
	Position m_size;
	unsigned int m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int rows() const {return m_size.first;}
	int cols() const {return m_size.second;}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_size(strs.size(), strs[0].length())
	, m_start(0)
{
	for(int r = 0, n = 0; r < rows(); ++r){
		auto& str = strs[r];
		for(int c = 0; c < cols(); ++c, ++n)
			if(str[c] == PUZ_ON)
				m_start |= 1 << n;
	}
}

struct puz_step
{
	Position m_p;
	puz_step(const Position& p)
		: m_p(p + Position(1, 1)) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
	out << "click " << mi.m_p << endl;;
	return out;
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g)
		: m_data(g.m_start), m_game(&g) {}
	int rows() const {return m_game->rows();}
	int cols() const {return m_game->cols();}
	char cells(int r, int c) const {return m_data & (1 << (r * cols() + c)) ? PUZ_ON : PUZ_OFF;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
	}
	inline void click(const Position& p) {
		for(int i = 0; i < 5; ++i){
			Position p2 = p + offset[i];
			if(is_valid(p2))
				m_data ^= 1 << (p2.first * cols() + p2.second);
		}
		m_move = puz_step(p);
	}
	inline bool operator<(const puz_state& x) const {
		return m_data < x.m_data;
	}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		// not an admissible heuristic
		int n = 0;
		for(int i = 0; i < rows() * cols(); ++i)
			if(m_data & (1 << i))
				n++;
		return n;
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {if(m_move) out << *m_move;}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	unsigned int m_data;
	const puz_game* m_game;
	boost::optional<puz_step> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
	for(int r = 0; r < rows(); ++r)
		for(int c = 0; c < cols(); ++c){
			children.push_back(*this);
			children.back().click(Position(r, c));
		}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 0; r < rows(); ++r) {
		for(int c = 0; c < cols(); ++c)
			out << cells(r, c) << " ";
		out << endl;
	}
	return out;
}

}}

void solve_puz_lightsout_int()
{
	using namespace puzzles::lightsout_int;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\lightsout-int.xml", "Puzzles\\lightsout-int.txt");
}