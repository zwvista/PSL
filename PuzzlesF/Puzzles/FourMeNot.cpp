#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 9/Four-Me-Not

	Summary
	It seems we do a lot of gardening in this game! 

	Description
	1. In Four-Me-Not (or Forbidden Four) you need to create a continuous
	   flower bed without putting four flowers in a row.
	2. More exactly, you have to join the existing flowers by adding more of
	   them, creating a single path of flowers touching horizontally or
	   vertically.
	3. At the same time, you can't line up horizontally or vertically more
	   than 3 flowers (thus Forbidden Four).
	4. Some tiles are marked as squares and are just fixed blocks.
*/

namespace puzzles{ namespace FourMeNot{

#define PUZ_EMPTY		'.'
#define PUZ_FIXED		'F'
#define PUZ_ADDED		'f'
#define PUZ_BLOCK		'B'
#define PUZ_BOUNDARY	'+'

bool is_flower(char ch) { return ch == PUZ_FIXED || ch == PUZ_ADDED; }

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
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
{
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			m_start.push_back(ch == ' ' ? PUZ_EMPTY : ch);
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);
	void find_paths();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_path_count - 1; }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	int m_path_count = 0;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	find_paths();
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& a);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>* m_area;
};

puz_state2::puz_state2(const set<Position>& a)
: m_area(&a)
{
	make_move(*a.begin());
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_area->count(p2) != 0){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

void puz_state::find_paths()
{
	set<Position> a;
	for(int i = 0; i < length(); ++i)
		if(is_flower(at(i)))
			a.insert({i / sidelen(), i % sidelen()});

	m_path_count = 0;
	while(!a.empty()){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves(a, smoves);
		for(auto& p : smoves)
			a.erase(p);
		++m_path_count;
	}
}

bool puz_state::make_move(const Position& p)
{
	int cnt = m_path_count;
	cells(p) = PUZ_ADDED;
	find_paths();
	m_distance = cnt - m_path_count;
	
	vector<int> counts;
	for(auto& os : offset){
		int n = 0;
		for(auto p2 = p + os; is_flower(cells(p2)); p2 += os)
			++n;
		counts.push_back(n);
	}
	return counts[0] + counts[2] < 3 && counts[1] + counts[3] < 3;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(int i = 0; i < length(); ++i){
		if((*this)[i] != PUZ_EMPTY)
			continue;
		Position p(i / sidelen(), i % sidelen());
		if([&]{
			for(auto& os : offset)
				if(is_flower(cells(p + os)))
					return true;
			return false;
		}()){
			children.push_back(*this);
			if(!children.back().make_move(p))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			char ch = cells({r, c});
			out << ch;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_FourMeNot()
{
	using namespace puzzles::FourMeNot;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\FourMeNot.xml", "Puzzles\\FourMeNot.txt", solution_format::GOAL_STATE_ONLY);
}