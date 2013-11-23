#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Abc

	Summary
	Fill the board with ABC

	Description
	1. The goal is to put the letter A B and C in the board.
	2. Each letter appear once in every row and column.
	3. The letters on the borders tell you what letter you see from there.
	4. Since most puzzles can contain empty spaces, the hint on the board
	   doesn't always match the tile next to it.
	5. Bigger puzzles can contain the letter 'D'. In these cases, the name
	   of the puzzle is 'Abcd'. Further on, you might also encounter 'E',
	   'F' etc.
*/

namespace puzzles{ namespace abc{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	char m_letter_max;
	vector<vector<Position>> m_area_pos;
	vector<string> m_combs;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area_pos(m_sidelen * 2)
{
	m_start = boost::accumulate(strs, string());
	m_letter_max = *boost::max_element(m_start);
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area_pos[r].push_back(p);
			m_area_pos[m_sidelen + c].push_back(p);
		}
	}

	string comb(m_sidelen, PUZ_EMPTY);
	auto f = [&](int border, int start, int end, int step){
		for(int i = start; i != end; i += step)
			if(comb[i] != PUZ_EMPTY){
				comb[border] = comb[i];
				return;
			}
	};

	auto begin = next(comb.begin()), end = prev(comb.end());
	iota(next(begin, m_sidelen - 2 - (m_letter_max - 'A' + 1)), end, 'A');
	do{
		f(0, 1, m_sidelen - 1, 1);
		f(m_sidelen - 1, m_sidelen - 2, 0, -1);
		m_combs.push_back(comb);
	}while(next_permutation(begin, end));
}

struct puz_state : map<int, vector<int>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool make_move(int i, int j);
	void make_move2(int i, int j);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return size();}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(int i = 1; i <= sidelen() - 2; ++i)
		(*this)[i], (*this)[sidelen() + i];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : *this){
		string area;
		for(auto& p : m_game->m_area_pos[kv.first])
			area.push_back(cell(p));

		kv.second.clear();
		for(int i = 0; i < m_game->m_combs.size(); i++)
			if(boost::equal(area, m_game->m_combs[i], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			}))
				kv.second.push_back(i);

		if(!init)
			switch(kv.second.size()){
			case 0:
				return 0;
			case 1:
				make_move2(kv.first, kv.second.front());
				return 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& area = m_game->m_area_pos[i];
	auto& comb = m_game->m_combs[j];

	for(int k = 0; k < comb.size(); ++k)
		cell(area[k]) = comb[k];

	++m_distance;
	erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
	}
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(*this, [](const pair<const int, vector<int>>& kv1, const pair<const int, vector<int>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(int n : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, n))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cell(Position(r, c)) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_abc()
{
	using namespace puzzles::abc;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\abc.xml", "Puzzles\\abc.txt", solution_format::GOAL_STATE_ONLY);
}