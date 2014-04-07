#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 11/Orchards

	Summary
	Plant the trees. Very close, this time!

	Description
	1. In a reverse of 'Parks', you're now planting Trees close together in
	   neighboring country areas.
	2. These are Apple Trees, which must cross-pollinate, thus must be planted
	   in pairs - horizontally or vertically touching.
	3. A Tree must be touching just one other Tree: you can't put three or
	   more contiguous Trees.
	4. At the same time, like in Parks, every country area must have exactly
	   two Trees in it.
*/

namespace puzzles{ namespace Orchards{

#define PUZ_TREE		'T'
#define PUZ_SPACE		'.'

struct puz_tree_info
{
	Position m_trees[2];
	Position m_spaces[6];
};

const puz_tree_info tree_info[4] = {
	{{{0, 0}, {-1, 0}}, {{-2, 0}, {-1, 1}, {0, 1}, {1, 0}, {0, -1}, {0, -2}}},
	{{{0, 0}, {0, 1}}, {{-1, 0}, {-1, 1}, {0, 2}, {1, 1}, {1, 0}, {0, -1}}},
	{{{0, 0}, {1, 0}}, {{-1, 0}, {0, 1}, {1, 1}, {2, 0}, {1, -1}, {0, -1}}},
	{{{0, 0}, {0, -1}}, {{-1, 0}, {0, 1}, {1, 0}, {1, -1}, {0, -2}, {-1, -1}}},
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_area_count = 0;
	map<Position, int> m_pos2area;
	vector<Position> m_trees;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen{strs.size()}
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = str[c] - 'a';
			m_pos2area[p] = n;
			m_area_count = max(m_area_count, n + 1);
		}
	}
}

// first : all the remaining positions in the area where a tree can be planted
// second : the number of trees that need to be planted in the area
struct puz_area : pair<set<Position>, int>
{
	puz_area() : pair<set<Position>, int>({}, 2) {}
	void add_cell(const Position& p){ first.insert(p); }
	void remove_cell(const Position& p){ first.erase(p); }
	bool can_plant_tree(const Position& p){ return first.count(p) != 0; }
	void plant_tree(const Position& p){ first.erase(p); --second; }
	bool is_valid() const {
		return second >= 0 && first.size() >= second;
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
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);
	puz_area& get_area(const Position& p) { return m_areas[m_game->m_pos2area.at(p)]; }

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return 2 * m_game->m_area_count - boost::count(*this, PUZ_TREE);
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	vector<puz_area> m_areas;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g), m_areas(g.m_area_count)
{
	for(int r = 0; r < g.m_sidelen; ++r)
		for(int c = 0; c < g.m_sidelen; ++c){
			Position p(r, c);
			get_area(p).add_cell(p);
		}
}

bool puz_state::make_move(const Position& p, int n)
{
	auto& info = tree_info[n];
	for(auto& os : info.m_trees){
		auto p2 = p + os;
		if(!is_valid(p2))
			return false;
		cells(p2) = PUZ_TREE;
		auto& a = get_area(p2);
		if(!a.can_plant_tree(p2))
			return false;
		a.plant_tree(p2);
		if(!a.is_valid())
			return false;
	}
	for(auto& os : info.m_spaces){
		auto p2 = p + os;
		if(!is_valid(p2))
			continue;
		auto& a = get_area(p2);
		a.remove_cell(p2);
		if(!a.is_valid())
			return false;
	}
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	vector<const puz_area*> areas;
	for(const puz_area& a : m_areas)
		if(a.second > 0)
			areas.push_back(&a);

	const auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2){
		return a1->first.size() < a2->first.size();
	});
	for(const auto& p : a.first)
		for(int i = 0; i < 4; ++i){
			children.push_back(*this);
			if(!children.back().make_move(p, i))
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

void solve_puz_Orchards()
{
	using namespace puzzles::Orchards;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Orchards.xml", "Puzzles\\Orchards.txt", solution_format::GOAL_STATE_ONLY);
}
