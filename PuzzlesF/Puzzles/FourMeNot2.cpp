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

namespace puzzles{ namespace FourMeNot2{

#define PUZ_EMPTY		'.'
#define PUZ_FIXED		'X'
#define PUZ_FLOWER		'F'
#define PUZ_BLOCK		'B'
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
			m_start.push_back(ch == ' ' ? PUZ_FLOWER : ch);
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

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_4flowers.size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	vector<set<Position>> m_4flowers;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	auto f = [&](set<Position>&& rng){
		if(boost::algorithm::all_of(rng, [&](const Position& p){
			char ch = this->cells(p);
			return ch == PUZ_FIXED || ch == PUZ_FLOWER;
		}))
			m_4flowers.push_back(std::move(rng));
	};

	for(int i = 1; i < sidelen() - 4; ++i)
		for(int j = 1; j < sidelen() - 1; ++j){
			f({{i, j}, {i + 1, j}, {i + 2, j}, {i + 3, j}});
			f({{j, i}, {j, i + 1}, {j, i + 2}, {j, i + 3}});
		}
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_state->sidelen(); }
	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
	int i = boost::find_if(s, [](char ch){
		return ch == PUZ_FIXED || ch == PUZ_FLOWER;
	}) - s.begin();
	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch == PUZ_FIXED || ch == PUZ_FLOWER){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move(const Position& p)
{
	cells(p) = PUZ_EMPTY;
	int cnt = m_4flowers.size();
	boost::remove_erase_if(m_4flowers, [&](const set<Position>& rng){
		return rng.count(p) != 0;
	});
	m_distance = cnt - m_4flowers.size();

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.size() == boost::count_if(*this, [](char ch){
		return ch == PUZ_FIXED || ch == PUZ_FLOWER;
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(int i = 0; i < length(); ++i)
		if((*this)[i] == PUZ_FLOWER){
			children.push_back(*this);
			if(!children.back().make_move({i / sidelen(), i % sidelen()}))
				children.pop_back();
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

void solve_puz_FourMeNot2()
{
	using namespace puzzles::FourMeNot2;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\FourMeNot.xml", "Puzzles\\FourMeNot2.txt", solution_format::GOAL_STATE_ONLY);
}