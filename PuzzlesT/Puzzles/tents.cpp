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
	{-1, 0},	// n
	{-1, 1},	// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},	// sw
	{0, -1},	// w
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
	void add_cell(const Position& p){ first.insert(p); }
	void remove_cell(const Position& p){ first.erase(p); }
	void plant_tree(const Position& p){ first.erase(p); --second; }
	bool is_valid() const {
		return second >= 0 && first.size() >= second;
	}
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

struct puz_groups
{
	puz_groups() {}
	puz_groups(const puz_game* g)
		: m_game{g}
		, m_rows(g->m_tent_counts_rows)
		, m_cols(g->m_tent_counts_cols)
	{}

	array<puz_area*, 2> get_areas(const Position& p){
		return {
			&m_rows[p.first],
			&m_cols[p.second]
		};
	}
	void add_cell(const Position& p){
		for(puz_area* a : get_areas(p))
			a->add_cell(p);
	}
	void remove_cell(const Position& p){
		for(puz_area* a : get_areas(p))
			a->remove_cell(p);
	}
	void plant_tree(const Position& p){
		for(puz_area* a : get_areas(p))
			a->plant_tree(p);
	}
	bool is_valid() const {
		return m_rows.is_valid() && m_cols.is_valid();
	}
	const puz_area& get_best_candidate_area() const;

	const puz_game* m_game;
	puz_group m_rows;
	puz_group m_cols;
};

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_groups.m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_groups.m_game->m_tent_total_count - boost::count(*this, PUZ_TENT);
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	puz_groups m_groups;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start)
, m_groups(&g)
{
	for(const auto& p : g.m_trees)
		for(int i = 0; i < 8; i += 2){
			auto p2 = p + offset[i];
			if(is_valid(p2))
				m_groups.add_cell(p2);
		}
}

const puz_area& puz_groups::get_best_candidate_area() const
{
	vector<const puz_area*> areas;
	for(const puz_group* grp : {&m_rows, &m_cols})
		for(const puz_area& a : *grp)
			if(a.second > 0)
				areas.push_back(&a);

	return **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2){
		return a1->first.size() < a2->first.size();
	});
}

bool puz_state::make_move(const Position& p)
{
	cell(p) = PUZ_TENT;
	m_groups.plant_tree(p);

	// no touch
	for(const auto& os : offset){
		const auto& p2 = p + os;
		if(is_valid(p2))
			m_groups.remove_cell(p2);
	}

	return m_groups.is_valid();
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& a = m_groups.get_best_candidate_area();
	for(const auto& p : a.first){
		children.push_back(*this);
		if(!children.back().make_move(p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cell(Position(r, c)) << " ";
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
