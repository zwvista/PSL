#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 15/ZenLandscaper

	Summary
	Variety and Balance

	Description
	1. The Zen master has been very stressed as of late, to the point that
	   yesterday he bolted for the Bahamas.
	2. The sun proved so irresistible, that he didn't even complete the
	   Japanese Gardens he worked on.
	3. Being the Zen Apprentice, you are given the task to complete all of
	   them following the master teaching of variety and continuity.
	4. The teaching says that any three contiguous tiles vertically,
	   horizontally or diagonally must NOT be:
	   all different
	   all equal
*/

namespace puzzles{ namespace ZenLandscaper{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'

const Position offset[] = {
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},	// sw
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	// 1st dimension : area id
	// 2nd dimension : three contiguous tiles
	vector<vector<Position>> m_area2range;
	// all permutations
	// 1 1 2
	// 1 1 3
	// ...
	// 3 3 2
	vector<string> m_perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
	}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	m_start = boost::accumulate(strs, string());
	for(int r = 0; r < m_sidelen; ++r)
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			for(auto& os : offset){
				auto p2 = p + os, p3 = p2 + os;
				if(is_valid(p2) && is_valid(p3))
					m_area2range.push_back({p, p2, p3});
			}
		}
	for(char ch1 = '1'; ch1 <= '3'; ++ch1)
		for(char ch2 = '1'; ch2 <= '3'; ++ch2)
			for(char ch3 = '1'; ch3 <= '3'; ++ch3)
				// NOT all equal or all different
				if(!(ch1 == ch2 && ch2 == ch3 ||
					ch1 != ch2 && ch1 != ch3 && ch2 != ch3))
					m_perms.push_back({ch1, ch2, ch3});
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const {
		return make_pair(m_cells, m_matches) < make_pair(x.m_cells, x.m_matches);
	}
	bool make_move(int i, int j);
	void make_move2(int i, int j);
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
	string m_cells;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	vector<int> perm_ids(g.m_perms.size());
	boost::iota(perm_ids, 0);

	for(int i = 0; i < g.m_area2range.size(); ++i)
		m_matches[i] = perm_ids;

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	auto& perms = m_game->m_perms;
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& p : m_game->m_area2range[kv.first])
			chars.push_back(cells(p));

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(area_id, perm_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& range = m_game->m_area2range[i];
	auto& perm = m_game->m_perms[j];

	for(int k = 0; k < perm.size(); ++k)
		cells(range[k]) = perm[k];

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<int, vector<int>>& kv1, 
		const pair<int, vector<int>>& kv2){
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
			out << cells({r, c}) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_ZenLandscaper()
{
	using namespace puzzles::ZenLandscaper;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\ZenLandscaper.xml", "Puzzles\\ZenLandscaper.txt", solution_format::GOAL_STATE_ONLY);
}