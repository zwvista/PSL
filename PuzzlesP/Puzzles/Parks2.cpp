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

namespace puzzles{ namespace Parks2{

#define PUZ_TREE		'T'
#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'

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

struct puz_area_info
{
	vector<Position> m_range;
	vector<string> m_disps;
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	int m_tree_count_area;
	int m_tree_total_count;
	string m_start;
	map<Position, int> m_pos2park;
	vector<Position> m_trees;
	vector<puz_area_info> m_area_info;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_tree_count_area(attrs.get<int>("numTreesInEachArea", 1))
, m_tree_total_count(m_tree_count_area * m_sidelen)
, m_area_info(m_sidelen * 3)
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			char ch = str[c];
			if(isupper(ch)){
				ch = tolower(ch);
				m_trees.push_back(p);
				m_start.push_back(PUZ_TREE);
			}
			else
				m_start.push_back(PUZ_SPACE);
			int n = ch - 'a';
			m_pos2park[p] = n;
			m_area_info[r].m_range.push_back(p);
			m_area_info[c + m_sidelen].m_range.push_back(p);
			m_area_info[n + m_sidelen * 2].m_range.push_back(p);
		}
	}

	map<int, vector<string>> sz2disps;
	for(auto& info : m_area_info){
		int ps_cnt = info.m_range.size();
		auto& disps = sz2disps[ps_cnt];
		if(disps.empty()){
			auto disp = string(ps_cnt - m_tree_count_area, PUZ_EMPTY)
				+ string(m_tree_count_area, PUZ_TREE);
			do
				disps.push_back(disp);
			while(boost::next_permutation(disp));
		}
		for(const auto& disp : disps){
			vector<Position> ps_tree;
			for(int i = 0; i < disp.size(); ++i)
				if(disp[i] == PUZ_TREE)
					ps_tree.push_back(info.m_range[i]);

			if([&](){
				// no touching
				for(const auto& ps1 : ps_tree)
					for(const auto& ps2 : ps_tree)
						if(boost::algorithm::any_of_equal(offset, ps1 - ps2))
							return false;
				return true;
			}())
				info.m_disps.push_back(disp);
		}
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(int i, int j);
	bool make_move2(int i, int j);
	int find_matches(bool init);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return 1; }
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
	for(int i = 0; i < sidelen() * 3; ++i)
		m_matches[i];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& info = m_game->m_area_info[kv.first];
		string area;
		for(auto& p : info.m_range)
			area.push_back(cells(p));

		kv.second.clear();
		for(int i = 0; i < info.m_disps.size(); ++i)
			if(boost::equal(area, info.m_disps[i], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			}))
				kv.second.push_back(i);

		if(!init)
			switch(kv.second.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(kv.first, kv.second.front()) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(int i, int j)
{
	auto& info = m_game->m_area_info[i];
	auto& area = info.m_range;
	auto& disp = info.m_disps[j];

	for(int k = 0; k < disp.size(); ++k){
		auto& p = area[k];
		if((cells(p) = disp[k]) != PUZ_TREE) continue;

		// no touching
		for(auto& os : offset){
			auto p2 = p + os;
			if(is_valid(p2) && cells(p2) == PUZ_TREE)
				return false;
		}
	}

	++m_distance;
	m_matches.erase(i);
	return true;
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	if(!make_move2(i, j))
		return false;
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_matches, [](
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
			out << cells(Position(r, c)) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_Parks2()
{
	using namespace puzzles::Parks2;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\parks.xml", "Puzzles\\Parks2.txt", solution_format::GOAL_STATE_ONLY);
}
