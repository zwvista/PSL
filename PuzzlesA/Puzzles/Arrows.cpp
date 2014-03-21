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

namespace puzzles{ namespace Arrows{

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
	vector<Position> m_range;
	vector<int> m_dirs;
	vector<vector<int>> m_perms;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<int> m_start;
	map<Position, puz_arrow> m_pos2arrows;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
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
			int n = cells(p);
			auto& arrow = m_pos2arrows[p];

			for(int i = 0; i < 8; ++i){
				auto& os = offset[i];
				for(auto p2 = p + os;; p2 += os){
					int n2 = cells(p2);
					if(n2 == PUZ_BORDER){
						arrow.m_range.push_back(p2);
						arrow.m_dirs.push_back((i + 4) % 8);
						break;
					}
					if(n2 == PUZ_CORNER)
						break;
				}
			}

			int sz = arrow.m_range.size();
			vector<int> perm(sz);

			vector<int> indicators;
			indicators.insert(indicators.end(), sz - n, 0);
			indicators.insert(indicators.end(), n, 1);
			do{
				for(int i = 0; i < sz; ++i){
					int n = arrow.m_dirs[i];
					perm[i] = indicators[i] == 1 ? n : ~n;
				}
					
				arrow.m_perms.push_back(perm);
			}while(boost::next_permutation(indicators));
		}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, int n);
	void make_move2(const Position& p, int n);
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
	for(auto& kv : g.m_pos2arrows){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(kv.second.m_perms.size());
		boost::iota(perm_ids, 0);
	}

	for(int r = 1; r < sidelen() - 1; ++r){
		auto& s1 = m_arrow_dirs[{r, 0}];
		s1 = {1, 2, 3};
		if(r == 1) s1.erase(1);
		else if(r == sidelen() - 2) s1.erase(3);
		auto& s2 = m_arrow_dirs[{r, sidelen() - 1}];
		s2 = {5, 6, 7};
		if(r == 1) s2.erase(7);
		else if(r == sidelen() - 2) s2.erase(5);
	}
	for(int c = 1; c < sidelen() - 1; ++c){
		auto& s1 = m_arrow_dirs[{0, c}];
		s1 = {3, 4, 5};
		if(c == 1) s1.erase(5);
		else if(c == sidelen() - 2) s1.erase(3);
		auto& s2 = m_arrow_dirs[{sidelen() - 1, c}];
		s2 = {0, 1, 7};
		if(c == 1) s2.erase(7);
		else if(c == sidelen() - 2) s2.erase(1);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& p = kv.first;
		auto& perm_ids = kv.second;

		auto& arrow = m_game->m_pos2arrows.at(p);
		vector<set<int>> arrow_dirs;
		for(auto& p2 : arrow.m_range)
			arrow_dirs.push_back(m_arrow_dirs.at(p2));

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(arrow_dirs, arrow.m_perms[id], [](const set<int>& dirs, int n2){
				return n2 >= 0 && dirs.count(n2) != 0
					|| n2 < 0 && (dirs.size() > 1 || dirs.count(~n2) == 0);
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perm_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
	auto& arrow = m_game->m_pos2arrows.at(p);
	auto& perm = arrow.m_perms[n];

	for(int k = 0; k < perm.size(); ++k){
		const auto& p = arrow.m_range[k];
		int& n1 = cells(p);
		int n2 = perm[k];
		if(n2 >= 0)
			m_arrow_dirs[p] = {n1 = n2};
		else
			m_arrow_dirs[p].erase(~n2);
	}

	++m_distance;
	m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	make_move2(p, n);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
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
			int n = cells({r, c});
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

void solve_puz_Arrows()
{
	using namespace puzzles::Arrows;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Arrows.xml", "Puzzles\\Arrows.txt", solution_format::GOAL_STATE_ONLY);
}