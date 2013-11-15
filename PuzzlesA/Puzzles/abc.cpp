#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Abc

	Summary
	Fill the board with ABC

	Description
	The goal is to put the letter A B and C in the board.
	1. Each letter appear once in every row and column.
	2. The letters on the borders tell you what letter you see from there.
	3. Since most puzzles can contain empty spaces, the hint on the board
	   doesn't always match the tile next to it.
	4. Bigger puzzles can contain the letter 'D'. In these cases, the name
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
	vector<string> m_perms;

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
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area_pos[r].push_back(p);
			m_area_pos[m_sidelen + c].push_back(p);
		}
	}

	string perm(m_sidelen, PUZ_EMPTY);
	auto f = [&](int border, int start, int end, int step){
		for(int i = start; i != end; i += step)
			if(perm[i] != PUZ_EMPTY){
				perm[border] = perm[i];
				return;
			}
	};

	auto begin = next(perm.begin()), end = prev(perm.end());
	iota(next(begin, m_sidelen - 2 - (m_letter_max - 'A' + 1)), end, 'A');
	do{
		f(0, 1, m_sidelen - 1, 1);
		f(m_sidelen - 1, m_sidelen - 2, 0, -1);
		m_perms.push_back(perm);
	}while(next_permutation(begin, end));
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i, int j);
	void find_matches();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE) - 4;}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(int i = 1; i <= sidelen() - 2; ++i)
		m_matches[i], m_matches[sidelen() + i];

	find_matches();
}

void puz_state::find_matches()
{
	vector<int> filled;

	for(auto& kv : m_matches){
		string area;
		for(const auto& p : m_game->m_area_pos.at(kv.first))
			area.push_back(cell(p));
		if(boost::algorithm::none_of(area, [](char ch){return ch == PUZ_SPACE;})){
			filled.push_back(kv.first);
			continue;
		}

		kv.second.clear();
		for(int i = 0; i < m_game->m_perms.size(); i++)
			if(boost::equal(area, m_game->m_perms.at(i), [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2; }))
				kv.second.push_back(i);
	}

	for(int n : filled)
		m_matches.erase(n);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	const auto& area = m_game->m_area_pos.at(i);
	const auto& perm = m_game->m_perms.at(j);

	for(int k = 0; k < sidelen(); ++k){
		char& ch = cell(area[k]);
		if(ch == PUZ_SPACE)
			ch = perm[k], ++m_distance;
	}

	find_matches();
	return boost::algorithm::none_of(m_matches, [](const pair<const int, vector<int>>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(m_matches, [](const pair<const int, vector<int>>& kv1, const pair<const int, vector<int>>& kv2){
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