#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Tents

	Summary
	Each camper wants to put his Tent under the shade of a Tree. But he also
	wants his privacy!

	Description
	1. The board represents a camping field with many Trees. Campers want to set
	   their Tent in the shade, horizontally or vertically adjacent to a Tree(not
	   diagonally).
	2. At the same time they need their privacy, so a Tent can't have any other
	   Tents near them, not even diagonally.
	3. The numbers on the borders tell you how many Tents there are in that row
	   or column.
	4. Finally, each Tree has at least one Tent touching it, horizontally or
	   vertically.
*/

namespace puzzles{ namespace tents{

#define PUZ_SPACE		' '
#define PUZ_TREE		'T'
#define PUZ_EMPTY		'.'
#define PUZ_TENT		'E'

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	vector<Position> m_trees;
	vector<int> m_tent_counts_rows, m_tent_counts_cols;
	int m_tent_total_count;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen{strs.size() - 1}
	, m_tent_counts_rows(m_sidelen)
	, m_tent_counts_cols(m_sidelen)
{
	for(int r = 0; r < m_sidelen + 1; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen + 1; ++c)
			switch(char ch = str[c]){
			case PUZ_SPACE:
				m_start.push_back(PUZ_EMPTY);
				break;
			case PUZ_TREE:
				m_start.push_back(PUZ_TREE);
				m_trees.emplace_back(r, c);
				break;
			default:
				if(c == m_sidelen)
					m_tent_counts_rows[r] = ch - '0';
				else if(r == m_sidelen)
					m_tent_counts_cols[c] = ch - '0';
				break;
			}
	}
	m_tent_total_count = boost::accumulate(m_tent_counts_rows, 0);
}

// first : all the remaining positions in the area where a tent can be placed
// second : the number of tents that need to be placed in the area
struct puz_area : pair<set<Position>, int>
{
	puz_area() {}
	puz_area(int tent_count)
		: pair<set<Position>, int>({}, tent_count)
	{}
	void add_cells(const Position& p){ first.insert(p); }
	void remove_cells(const Position& p){ first.erase(p); }
	void plant_tree(const Position& p, bool at_least_one){
		if(first.count(p) == 0) return;
		first.erase(p);
		if(!at_least_one || at_least_one && second == 1)
			--second;
	}
	bool is_valid() const { return second >= 0 && first.size() >= second; }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
	puz_group() {}
	puz_group(const vector<int>& tent_counts){
		for(int cnt : tent_counts)
			emplace_back(cnt);
	}
	bool is_valid() const {
		return boost::algorithm::all_of(*this, [](const puz_area& a) {
			return a.is_valid();
		});
	}
};

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_game->m_tent_total_count - boost::count(*this, PUZ_TENT);
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	puz_group m_grp_trees;
	puz_group m_grp_rows;
	puz_group m_grp_cols;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_grp_trees(vector<int>(g.m_trees.size(), 1))
, m_grp_rows(g.m_tent_counts_rows)
, m_grp_cols(g.m_tent_counts_cols)
{
	for(int i = 0; i < g.m_trees.size(); ++i){
		auto& p = g.m_trees[i];
		for(int j = 0; j < 8; j += 2){
			auto p2 = p + offset[j];
			if(is_valid(p2) && cells(p2) == PUZ_EMPTY){
				m_grp_rows[p2.first].add_cells(p2);
				m_grp_cols[p2.second].add_cells(p2);
				m_grp_trees[i].add_cells(p2);
			}
		}
	}
}

bool puz_state::make_move(const Position& p)
{
	cells(p) = PUZ_TENT;

	for(auto& a : m_grp_trees)
		a.plant_tree(p, true);
	m_grp_rows[p.first].plant_tree(p, false);
	m_grp_cols[p.second].plant_tree(p, false);

	// no touch
	for(auto& os : offset){
		auto p2 = p + os;
		if(is_valid(p2)){
			for(auto& a : m_grp_trees)
				a.remove_cells(p2);
			m_grp_rows[p2.first].remove_cells(p2);
			m_grp_cols[p2.second].remove_cells(p2);
		}
	}

	return m_grp_rows.is_valid() && m_grp_cols.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
	vector<const puz_area*> areas;
	for(auto grp : {&m_grp_trees, &m_grp_rows, &m_grp_cols})
		for(auto& a : *grp)
			if(a.second > 0)
				areas.push_back(&a);

	auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2){
		return a1->first.size() < a2->first.size();
	});
	for(auto& p : a.first){
		children.push_back(*this);
		if(!children.back().make_move(p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cells({r, c}) << " ";
		out << endl;
	}
	return out;
}

}}

void solve_puz_tents()
{
	using namespace puzzles::tents;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\tents.xml", "Puzzles\\tents.txt", solution_format::GOAL_STATE_ONLY);
}
