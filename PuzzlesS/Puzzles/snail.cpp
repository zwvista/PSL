#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Snail

	Summary
	Follow the winding path with numbers 1, 2 and 3

	Description
	1. The goal is to start at the entrance in the top left corner and proceed
	   towards the center, leaving a trail of numbers.
	2. The numbers must be entered in the sequence 1,2,3 1,2,3 1,2,3 etc,
	   however not every tile must be filled in and some tiles will remain
	   empty.
	3. Your task is to determine which tiles will be empty, following these
	   two rules:
	4. Trail Rule: The first number to write after entering in the top left
	   is a 1 and the last before ending in the center is a 3. In between,
	   the 1,2,3 sequence will repeat many times in this order, following the
	   snail path.
	5. Board Rule: Each row and column of the board (disregarding the snail
	   path) must have exactly one 1, one 2 and one 3.
*/

namespace puzzles{ namespace snail{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'

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
	vector<Position> m_snail_path;
	// 1st dimension : the index of the area(rows and columns)
	// 2nd dimension : all the positions that the area is composed of
	vector<vector<Position>> m_area2range;
	// all dispositions
	// space space 1 2 3
	// space space 1 3 2
	// ...
	// 3 2 1 space space
	vector<string> m_disps;

	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
	}

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
	m_start = boost::accumulate(strs, string());
	for(int r = 0; r < m_sidelen; ++r){
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area2range[r].push_back(p);
			m_area2range[m_sidelen + c].push_back(p);
		}
	}

	auto disp = string(m_sidelen - 3, PUZ_EMPTY).append("123");
	do
		m_disps.push_back(disp);
	while(boost::next_permutation(disp));

	Position p(0, 0);
	m_snail_path.push_back(p);
	for(int i = 1, n = 1; i < m_sidelen * m_sidelen; ++i){
		auto p2 = p + offset[n];
		m_snail_path.push_back(
			is_valid(p2) && boost::range::find(m_snail_path, p2) == m_snail_path.end() ?
			(p = p2) :
			(p += offset[n = (n + 1) % 4])
		);
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(int i, int j);
	void make_move2(int i, int j);
	int find_matches(bool init);
	int check_snail();

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
	string m_cells;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		m_matches[i], m_matches[sidelen() + i];

	check_snail();
	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		string area;
		for(auto& p : m_game->m_area2range[kv.first])
			area.push_back(cells(p));

		kv.second.clear();
		for(int i = 0; i < m_game->m_disps.size(); ++i)
			if(boost::equal(area, m_game->m_disps[i], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			}))
				kv.second.push_back(i);

		if(!init)
			switch(kv.second.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(kv.first, kv.second.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& range = m_game->m_area2range[i];
	auto& disp = m_game->m_disps[j];

	for(int k = 0; k < disp.size(); ++k)
		cells(range[k]) = disp[k];

	++m_distance;
	m_matches.erase(i);
}

int puz_state::check_snail()
{
	int n = 2;
	char start = '3';
	vector<Position> path;
	int sz = sidelen() * sidelen();
	for(int i = 0; i <= sz; ++i){
		char ch = i == sz ? '1' : cells(m_game->m_snail_path[i]);
		if(ch == PUZ_SPACE)
			path.push_back(m_game->m_snail_path[i]);
		else if(ch != PUZ_EMPTY)
		{
			string mid;
			for(int j = start - '1'; (j = (j + 1) % 3) != ch - '1';)
				mid.push_back(j + '1');
			int sz1 = path.size(), sz2 = mid.size();
			if(sz1 < sz2)
				return 0;
			if(sz1 == sz2 && sz1 > 0){
				for(int k = 0; k < sz1; ++k){
					const auto& p = path[k];
					cells(p) = mid[k];
				}
				n = 1;
			}
			start = ch;
			path.clear();
		}
	}
	return n;
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	for(;;){
		int n;
		while((n = find_matches(false)) == 1);
		if(n == 0)
			return false;
		switch(check_snail()){
		case 0:
			return false;
		case 2:
			return true;
		}
	}
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_matches, [](
		const pair<const int, vector<int>>& kv1,
		const pair<const int, vector<int>>& kv2){
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
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << cells(Position(r, c)) << " ";
		out << endl;
	}
	return out;
}

}}

void solve_puz_snail()
{
	using namespace puzzles::snail;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\snail.xml", "Puzzles\\snail.txt", solution_format::GOAL_STATE_ONLY);
}