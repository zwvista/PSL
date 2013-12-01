#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Nurikabe

	Summary
	Draw a continuous wall that divides gardens as big as the numbers

	Description
	1. Each number on the grid indicates a garden, occupying as many tiles
	   as the number itself.
	2. Gardens can have any form, extending horizontally and vertically, but
	   can't extend diagonally.
	3. The garden is separated by a single continuous wall. This means all
	   wall tiles on the board must be connected horizontally or vertically.
	   There can't be isolated walls.
	4. You must find and mark the wall following these rules:
	5. All the gardens in the puzzle are numbered at the start, there are no
	   hidden gardens.
	6. A wall can't go over numbered squares.
	7. The wall can't form 2*2 squares.
*/

namespace puzzles{ namespace nurikabe{

#define PUZ_SPACE		' '
#define PUZ_GARDEN		'G'
#define PUZ_WALL		'W'
#define PUZ_BOUNDARY	'B'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

using puz_area = pair<vector<Position>, int>;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2garden;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen; ++c){
			switch(char ch = str[c]){
			case PUZ_SPACE:
				m_start.push_back(PUZ_WALL);
				break;
			default:
				m_start.push_back(PUZ_GARDEN);
				m_pos2garden[Position(r + 1, c + 1)] = ch - '0';
				break;
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

using puz_key = pair<Position, bool>;

struct puz_state : map<puz_key, vector<int>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool make_move(const puz_key& kv, int j);
	void make_move2(const puz_key& kv, int j);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return size(); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	//for(const auto& kv : g.m_pos2area_cols)
	//	(*this)[make_pair(kv.first, true)];
	//for(const auto& kv : g.m_pos2area_rows)
	//	(*this)[make_pair(kv.first, false)];

	//find_matches(true);
}

int puz_state::find_matches(bool init)
{
	//for(auto& kv : *this){
	//	vector<int> nums;
	//	auto& area = (kv.first.second ? m_game->m_pos2area_cols : m_game->m_pos2area_rows).at(kv.first.first);
	//	for(auto& p : area.first)
	//		nums.push_back(m_cells.at(p));

	//	kv.second.clear();
	//	auto& combs = m_game->m_sum2combs.at(make_pair(area.second, area.first.size()));
	//	for(int i = 0; i < combs.size(); ++i)
	//		if(boost::equal(nums, combs[i], [](int n1, int n2){
	//			return n1 == 0 || n1 == n2;
	//		}))
	//			kv.second.push_back(i);

	//	if(!init)
	//		switch(kv.second.size()){
	//		case 0:
	//			return 0;
	//		case 1:
	//			make_move2(kv.first, kv.second.front());
	//			return 1;
	//		}
	//}
	return 2;
}

void puz_state::make_move2(const puz_key& kv, int j)
{
	//auto& area = (kv.second ? m_game->m_pos2area_cols : m_game->m_pos2area_rows).at(kv.first);
	//auto& comb = m_game->m_sum2combs.at(make_pair(area.second, area.first.size()))[j];

	//for(int k = 0; k < comb.size(); ++k)
	//	m_cells.at(area.first[k]) = comb[k];

	//++m_distance;
	//erase(kv);
}

bool puz_state::make_move(const puz_key& kv, int j)
{
	m_distance = 0;
	make_move2(kv, j);
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
	const auto& kv = *boost::min_element(*this, [](
		const pair<const puz_key, vector<int>>& kv1,
		const pair<const puz_key, vector<int>>& kv2){
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
			Position p(r, c);
			//out << (m_game->m_blanks.count(p) != 0 ? char(m_cells.at(p) + '0') : '.') << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_nurikabe()
{
	using namespace puzzles::nurikabe;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\nurikabe.xml", "Puzzles\\nurikabe.txt", solution_format::GOAL_STATE_ONLY);
}