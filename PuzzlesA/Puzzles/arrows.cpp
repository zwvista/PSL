#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/Arrows

	Summary
	Just Arrows?

	Description
	1. The goal is to detect the arrows directions that reside outside the board.
	2. Each Arrow points to at least one number inside the board.
	3. The numbers tell you how many arrows point at them.
	4. There is one arrow for each tile outside the board.
*/

namespace puzzles{ namespace arrows{

#define PUZ_CORNER	100
#define PUZ_BORDER	99

const Position offset[] = {
	{-1, 0},	// n
	{-1, 1},	// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},	// sw
	{0, -1},	// w
	{-1, -1},	// nw
};

struct puz_arrow
{
	vector<Position> m_ps;
	vector<int> m_dirs;
	vector<vector<int>> m_combs;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<int> m_start;
	map<Position, puz_arrow> m_pos2arrows;
	int cell(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.push_back(PUZ_CORNER);
	m_start.insert(m_start.end(), m_sidelen - 2, PUZ_BORDER);
	m_start.push_back(PUZ_CORNER);
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BORDER);
		for(int c = 0; c < m_sidelen - 2; ++c)
			m_start.push_back(str[c] - '0');
		m_start.push_back(PUZ_BORDER);
	}
	m_start.push_back(PUZ_CORNER);
	m_start.insert(m_start.end(), m_sidelen - 2, PUZ_BORDER);
	m_start.push_back(PUZ_CORNER);

	for(int r = 1; r < m_sidelen - 1; ++r)
		for(int c = 1; c < m_sidelen - 1; ++c){
			Position p(r, c);
			int n = cell(p);
			auto& arrow = m_pos2arrows[p];

			for(int i = 0; i < 8; ++i){
				auto& os = offset[i];
				for(auto p2 = p + os;; p2 += os){
					int n2 = cell(p2);
					if(n2 == PUZ_BORDER){
						arrow.m_ps.push_back(p2);
						arrow.m_dirs.push_back((i + 4) % 8);
						break;
					}
					if(n2 == PUZ_CORNER)
						break;
				}
			}

			int sz = arrow.m_ps.size();
			vector<int> comb(sz);

			vector<int> indicators;
			indicators.insert(indicators.end(), sz - n, 0);
			indicators.insert(indicators.end(), n, 1);
			do{
				for(int i = 0; i < sz; ++i){
					int n = arrow.m_dirs[i];
					comb[i] = indicators[i] == 1 ? n : ~n;
				}
					
				arrow.m_combs.push_back(comb);
			}while(boost::next_permutation(indicators));
		}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cell(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	int& cell(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, int j);
	void make_move2(const Position& p, int j);
	int find_matches(bool init);

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
	vector<int> m_cells;
	map<Position, vector<int>> m_matches;
	map<Position, set<int>> m_arrow_dirs;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2arrows)
		m_matches[kv.first];

	for(int r = 1; r < sidelen() - 1; ++r){
		auto& s1 = m_arrow_dirs[Position(r, 0)];
		s1 = {1, 2, 3};
		if(r == 1) s1.erase(1);
		else if(r == sidelen() - 2) s1.erase(3);
		auto& s2 = m_arrow_dirs[Position(r, sidelen() - 1)];
		s2 = {5, 6, 7};
		if(r == 1) s2.erase(7);
		else if(r == sidelen() - 2) s2.erase(5);
	}
	for(int c = 1; c < sidelen() - 1; ++c){
		auto& s1 = m_arrow_dirs[Position(0, c)];
		s1 = {3, 4, 5};
		if(c == 1) s1.erase(5);
		else if(c == sidelen() - 2) s1.erase(3);
		auto& s2 = m_arrow_dirs[Position(sidelen() - 1, c)];
		s2 = {0, 1, 7};
		if(c == 1) s2.erase(7);
		else if(c == sidelen() - 2) s2.erase(1);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& arrow = m_game->m_pos2arrows.at(kv.first);
		vector<set<int>> arrow_dirs;
		for(auto& p : arrow.m_ps)
			arrow_dirs.push_back(m_arrow_dirs.at(p));

		kv.second.clear();
		for(int i = 0; i < arrow.m_combs.size(); ++i)
			if(boost::equal(arrow_dirs, arrow.m_combs[i], [](const set<int>& dirs, int n2){
				return n2 >= 0 && dirs.count(n2) != 0
					|| n2 < 0 && (dirs.size() > 1 || dirs.count(~n2) == 0);
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

void puz_state::make_move2(const Position& p, int j)
{
	auto& arrow = m_game->m_pos2arrows.at(p);
	auto& comb = arrow.m_combs[j];

	for(int k = 0; k < comb.size(); ++k){
		const auto& p = arrow.m_ps[k];
		int& n1 = cell(p);
		int n2 = comb[k];
		if(n2 >= 0)
			m_arrow_dirs[p] = {n1 = n2};
		else
			m_arrow_dirs[p].erase(~n2);
	}

	++m_distance;
	m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int j)
{
	m_distance = 0;
	make_move2(p, j);
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
	const auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<int>>& kv1,
		const pair<const Position, vector<int>>& kv2){
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
			if(n == PUZ_CORNER)
				out << "  ";
			else
				out << format("%-2d") % n;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_arrows()
{
	using namespace puzzles::arrows;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\arrows.xml", "Puzzles\\arrows.txt", solution_format::GOAL_STATE_ONLY);
}