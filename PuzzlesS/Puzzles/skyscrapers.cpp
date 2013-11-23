#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Skyscrapers

	Summary
	Guess skyscrapers heights, judging from the skyline

	Description
	1. The grid in the center represents a city from above. Each cell contain
	   a skyscraper, of different height.
	2. The goal is to guess the height of each Skyscraper.
	3. Each row and column can't have two Skyscrapers of the same height.
	4. The numbers on the boarders tell you how many skyscrapers you see from
	   there, keeping mind that a higher skyscraper hides a lower one. 
	   Skyscrapers are numbered from 1(lowest) to the grid size(height).
*/

namespace puzzles{ namespace skyscrapers{

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<int> m_start;
	vector<vector<Position>> m_area_pos;
	vector<vector<int>> m_perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area_pos(m_sidelen * 2)
{
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			m_start.push_back(ch == ' ' ? 0 : ch - '0');
			Position p(r, c);
			m_area_pos[r].push_back(p);
			m_area_pos[m_sidelen + c].push_back(p);
		}
	}

	vector<int> perm(m_sidelen);
	auto f = [&](int border, int start, int end, int step){
		int h = 0;
		perm[border] = 0;
		for(int i = start; i != end; i += step)
			if(perm[i] > h)
				h = perm[i], ++perm[border];
	};

	auto begin = next(perm.begin()), end = prev(perm.end());
	iota(begin, end, 1);
	do{
		f(0, 1, m_sidelen - 1, 1);
		f(m_sidelen - 1, m_sidelen - 2, 0, -1);
		m_perms.push_back(perm);
	}while(next_permutation(begin, end));
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	int& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i, int j);
	void find_matches();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
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
: vector<int>(g.m_start), m_game(&g)
{
	for(int i = 1; i <= sidelen() - 2; ++i)
		m_matches[i], m_matches[sidelen() + i];

	find_matches();
}

void puz_state::find_matches()
{
	vector<int> filled;

	for(auto& kv : m_matches){
		vector<int> area;
		for(const auto& p : m_game->m_area_pos.at(kv.first))
			area.push_back(cell(p));
		if(boost::algorithm::none_of(area, [](int n){return n == 0;})){
			filled.push_back(kv.first);
			continue;
		}

		kv.second.clear();
		for(int i = 0; i < m_game->m_perms.size(); i++)
			if(boost::equal(area, m_game->m_perms.at(i), [](int n1, int n2){
				return n1 == 0 || n1 == n2; }))
				kv.second.push_back(i);
	}

	for(int n : filled)
		m_matches.erase(n);
	m_distance = filled.size();
}

bool puz_state::make_move(int i, int j)
{
	const auto& area = m_game->m_area_pos.at(i);
	const auto& perm = m_game->m_perms.at(j);

	for(int k = 0; k < sidelen(); ++k){
		int& n = cell(area[k]);
		if(n == 0)
			n = perm[k];
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
		for(int c = 0; c < sidelen(); ++c){
			int n = cell(Position(r, c));
			out << (n == 0 ? ' ' : char(n + '0')) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_skyscrapers()
{
	using namespace puzzles::skyscrapers;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\skyscrapers.xml", "Puzzles\\skyscrapers.txt", solution_format::GOAL_STATE_ONLY);
}