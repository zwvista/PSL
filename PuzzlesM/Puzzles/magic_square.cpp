#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace magic_square{

struct puz_game
{
	string m_id;
	Position m_size;
	vector<int> m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int sidelen() const {return m_size.first;}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_size(strs.size(), strs.size())
	, m_start(sidelen() * sidelen())
{
	for(int r = 0, n = 0; r < sidelen(); ++r) {
		auto& str = strs[r];
		for(int c = 0; c < sidelen(); ++c)
			m_start[n++] = atoi(str.substr(c * 2, 2).c_str());
	}
}

struct puz_step
{
	Position m_p1, m_p2;
	puz_step(const Position& p1, const Position& p2)
		: m_p1(p1 + Position(1, 1)), m_p2(p2 + Position(1, 1)) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
	out << format("move: %1% <=> %2%\n") % mi.m_p1 % mi.m_p2;
	return out;
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g)
		: vector<int>(g.m_start), m_game(&g) {}
	int sidelen() const {return m_game->sidelen();}
	int cells(int r, int c) const {return (*this)[r * sidelen() + c];}
	int& cells(int r, int c) {return (*this)[r * sidelen() + c];}
	void make_move(int r1, int c1, int r2, int c2){
		std::swap(cells(r1, c1), cells(r2, c2));
		m_move = puz_step(Position(r1, c1), Position(r2, c2));
	}

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const;
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
	int rcmax = sidelen() - 1;
	for(int r1 = 0; r1 < rcmax; ++r1)
		for(int c1 = 0; c1 < rcmax; ++c1)
			for(int r2 = r1; r2 < rcmax; ++ r2)
				for(int c2 = r1 == r2 ? c1 + 1 : 0; c2 < rcmax; ++c2){
					children.push_back(*this);
					children.back().make_move(r1, c1, r2, c2);
				}
}

unsigned int puz_state::get_heuristic() const
{
	unsigned int d = 0;
	int rcmax = sidelen() - 1;
	for(int r = 0; r < rcmax; ++r){
		int sum = 0;
		for(int c = 0; c < rcmax; ++c)
			sum += cells(r, c);
		if(sum != cells(r, rcmax))
			d++;
	}
	for(int c = 0; c < rcmax; ++c){
		int sum = 0;
		for(int r = 0; r < rcmax; ++r)
			sum += cells(r, c);
		if(sum != cells(rcmax, c))
			d++;
	}
	{
		int sum = 0;
		for(int i = 0; i < rcmax; ++i)
			sum += cells(i, i);
		if(sum != cells(rcmax, rcmax))
			d++;
	}
	return (d + 3) / 4;
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << format("%2d") % cells(r, c);
		out << endl;
	}
	return out;
}

}}

void solve_puz_magic_square()
{
	using namespace puzzles::magic_square;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
		"Puzzles\\magic_square.xml", "Puzzles\\magic_square.txt");
}