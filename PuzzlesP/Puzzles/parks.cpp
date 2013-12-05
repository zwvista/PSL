#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Parks

	Summary
	Put one Tree in each Park, row and column.(two in bigger levels)

	Description
	In Parks, you have many differently coloured areas(Parks) on the board.
	The goal is to plant Trees, following these rules:
	1. A Tree can't touch another Tree, not even diagonally.
	2. Each park must have exactly ONE Tree.
	3. There must be exactly ONE Tree in each row and each column.
	4. Remember a Tree CANNOT touch another Tree diagonally,
	   but it CAN be on the same diagonal line.
	5. Larger puzzles have TWO Trees in each park, each row and each column.
*/

namespace puzzles{ namespace parks{

#define PUZ_TREE		'T'
#define PUZ_SPACE		'.'

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
	int m_tree_count_area;
	int m_tree_total_count;
	map<Position, int> m_pos2park;
	vector<Position> m_trees;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
	, m_tree_count_area(attrs.get<int>("numTreesInEachArea", 1))
	, m_tree_total_count(m_tree_count_area * m_sidelen)
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			char ch = str[c];
			if(isupper(ch)){
				ch = tolower(ch);
				m_trees.push_back(p);
			}
			int n = ch - 'a';
			m_pos2park[p] = n;
		}
	}
}

// first : all the remaining positions in the area where a tree can be planted
// second : the number of trees that need to be planted in the area
struct puz_area : pair<set<Position>, int>
{
	puz_area() {}
	puz_area(int tree_count_area)
		: pair<set<Position>, int>({}, tree_count_area)
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
	puz_group(int cell_count, int tree_count_area)
		: vector<puz_area>(cell_count, puz_area(tree_count_area)) {}
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
		: m_game(g)
		, m_parks(g->m_sidelen, g->m_tree_count_area)
		, m_rows(g->m_sidelen, g->m_tree_count_area)
		, m_cols(g->m_sidelen, g->m_tree_count_area)
	{}

	array<puz_area*, 3> get_areas(const Position& p){
		return {
			&m_parks[m_game->m_pos2park.at(p)],
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
		return m_parks.is_valid() && m_rows.is_valid() && m_cols.is_valid();
	}
	const puz_area& get_best_candidate_area() const;

	const puz_game* m_game;
	puz_group m_parks;
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
		return m_groups.m_game->m_tree_total_count - boost::count(*this, PUZ_TREE);
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
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_groups(&g)
{
	for(int r = 0; r < g.m_sidelen; ++r)
		for(int c = 0; c < g.m_sidelen; ++c)
			m_groups.add_cell(Position(r, c));

	for(const auto& p : g.m_trees)
		make_move(p);
}

const puz_area& puz_groups::get_best_candidate_area() const
{
	const puz_group* grps[] = {&m_parks, &m_rows, &m_cols};
	vector<const puz_area*> areas;
	for(const puz_group* grp : grps)
		for(const puz_area& a : *grp)
			if(a.second > 0)
				areas.push_back(&a);

	return **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2){
		return a1->first.size() < a2->first.size();
	});
}

bool puz_state::make_move(const Position& p)
{
	cell(p) = PUZ_TREE;
	m_groups.plant_tree(p);

	// no touch
	for(auto& os : offset){
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
			out << cell(Position(r, c)) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_parks()
{
	using namespace puzzles::parks;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\parks.xml", "Puzzles\\parks.txt", solution_format::GOAL_STATE_ONLY);
}
