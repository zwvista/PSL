#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Landscaper

	Summary
	Plant Trees and Flowers with enough variety 

	Description
	1. Your goal as a landscaper is to plant some Trees and Flowers on the
	   field, in every available tile.
	2. In doing this, you must assure the scenery is varied enough:
	3. No more than two consecutive Trees or Flowers should appear horizontally
	   or vertically.
	4. Every row and column should have an equal number of Trees and Flowers.
	5. Each row disposition must be unique, i.e. the same arrangement of Trees
	   and Flowers can't appear on two rows.
	6. Each column disposition must be unique as well.

	Note on odd-size level
	7. In odd-size levels, the number of Trees and Flowers obviously won't be
	   equal on a row or column. However each row and column will have the same
	   number of Trees and Flowers.
*/

namespace puzzles{ namespace landscaper{

#define PUZ_SPACE		' '
#define PUZ_TREE		'T'
#define PUZ_FLOWER		'F'

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	vector<vector<Position>> m_area2range;
	vector<string> m_combs;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
	m_start = boost::accumulate(strs, string());

	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area2range[r].push_back(p);
			m_area2range[m_sidelen + c].push_back(p);
		}
	}

	string comb(m_sidelen, PUZ_SPACE);
	auto f = [&](int n1, int n2){
		for(int i = 0; i < n1; ++i)
			comb[i] = PUZ_FLOWER;
		for(int i = 0; i < n2; ++i)
			comb[i + n1] = PUZ_TREE;
		do{
			bool no_more_than_two = true;
			for(int i = 1, n = 1; i < m_sidelen; ++i){
				n = comb[i] == comb[i - 1] ? n + 1 : 1;
				if(n > 2){
					no_more_than_two = false;
					break;
				}
			}
			if(no_more_than_two)
				m_combs.push_back(comb);
		}while(boost::next_permutation(comb));
	};
	if(m_sidelen % 2 == 0)
		f(m_sidelen / 2, m_sidelen / 2);
	else{
		f(m_sidelen / 2, m_sidelen / 2 + 1);
		f(m_sidelen / 2 + 1, m_sidelen / 2);
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
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
	set<int> m_disp_rows, m_disp_cols;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		m_matches[i], m_matches[sidelen() + i];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		string nums;
		for(const auto& p : m_game->m_area2range[kv.first])
			nums.push_back(cell(p));

		auto& disp = kv.first < sidelen() ? m_disp_rows : m_disp_cols;
		kv.second.clear();
		for(int i = 0; i < m_game->m_combs.size(); i++)
			if(boost::equal(nums, m_game->m_combs.at(i), [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			}) && disp.count(i) == 0)
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
	auto& area = m_game->m_area2range[i];
	auto& comb = m_game->m_combs[j];

	for(int k = 0; k < comb.size(); ++k)
		cell(area[k]) = comb[k];

	auto& disp = i < sidelen() ? m_disp_rows : m_disp_cols;
	disp.insert(j);

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
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
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c){
			char ch = cell(Position(r, c));
			out << ch << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_landscaper()
{
	using namespace puzzles::landscaper;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\landscaper.xml", "Puzzles\\landscaper.txt", solution_format::GOAL_STATE_ONLY);
}