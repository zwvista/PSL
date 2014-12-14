#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 14/Power Grid

	Summary
	Utility Posts

	Description
	1. Your task is to identify Utility Posts of a Power Grid.
	2. There are two Posts in each Row and in each Column.
	3. The numbers on the side tell you the length of the cables between
	   the two Posts (in that Row or Column).
	4. Or in other words, the number of empty tiles between two Posts.
	5. Posts cannot touch themselves, not even diagonally.
	6. Posts don't have to form a single connected chain.

	Variant
	7. On some levels, there are exactly two Posts in each diagonal too.
*/

namespace puzzles{ namespace PowerGrid{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_POST		'P'
	
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

typedef pair<vector<Position>, int> puz_area_info;

struct puz_game
{
	string m_id;
	int m_sidelen;
	bool m_is_diagonal_type;
	vector<puz_area_info> m_area2info;
	map<int, vector<string>> m_num2perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size() - 1)
	, m_is_diagonal_type(attrs.get<string>("GameType", "") == "DIAGONAL")
	, m_area2info(m_sidelen * 2 + (m_is_diagonal_type ? 2 : 0))
{
	auto f = [&](char ch) {
		int n = ch == ' ' ? 0 : ch - '0';
		m_num2perms[n];
		return n;
	};
	for(int i = 0; i < m_sidelen; ++i){
		m_area2info[i].second = f(strs[i][m_sidelen]);
		m_area2info[i + m_sidelen].second = f(strs[m_sidelen][i]);
		for(int j = 0; j < m_sidelen; ++j){
			m_area2info[i].first.emplace_back(i, j);
			m_area2info[i + m_sidelen].first.emplace_back(j, i);
		}
		if(m_is_diagonal_type){
			m_area2info[m_sidelen * 2].first.emplace_back(i, i);
			m_area2info[m_sidelen * 2 + 1].first.emplace_back(i, m_sidelen - 1 - i);
		}
	}
	if(m_is_diagonal_type)
		m_num2perms[m_area2info[m_sidelen * 2].second =
			m_area2info[m_sidelen * 2 + 1].second = 0];
	
	string perm_empty(m_sidelen, PUZ_EMPTY), perm;
	auto g = [&](int n, vector<string>& perms){
		for(int i = 0; i < m_sidelen - n - 1; ++i){
			perm = perm_empty;
			perm[i] = perm[i + n + 1] = PUZ_POST;
			perms.push_back(perm);
		}
	};
	for(auto& kv : m_num2perms){
		int n = kv.first;
		if(n == 0)
			for(int i = 0; i < m_sidelen - n - 1; ++i)
				g(i, kv.second);
		else
			g(n, kv.second);
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
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
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
	: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int i = 1; i < g.m_area2info.size(); ++i){
		auto& perm_ids = m_matches[i];
		perm_ids.resize(g.m_num2perms.at(g.m_area2info[i].second).size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& perm_ids = kv.second;

		auto& info = m_game->m_area2info[area_id];
		string area;
		for(auto& p : info.first)
			area.push_back(cells(p));
		auto& perms = m_game->m_num2perms.at(info.second);

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(area, perms[id], [](char ch1, char ch2){
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
	auto& info = m_game->m_area2info[i];
	auto& range = info.first;
	auto& perm = m_game->m_num2perms.at(info.second)[j];

	for(int k = 0; k < perm.size(); ++k){
		char ch = perm[k];
		auto& p = range[k];
		if((cells(p) = ch) == PUZ_POST)
			for(auto& os : offset){
				auto p2 = p + os;
				if(is_valid(p2))
					cells(p2) = PUZ_EMPTY;
			}
	}

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
	auto f = [&](int n){
		auto& rng = m_game->m_area2info[n].first;
		vector<int> v;
		for(int i = 0; i < rng.size(); ++i)
			if(cells(rng[i]) == PUZ_POST)
				v.push_back(i);
		return v[1] - v[0] - 1;
	};
	for(int r = 0; r <= sidelen(); ++r){
		for(int c = 0; c <= sidelen(); ++c){
			if(r == sidelen() && c == sidelen())
				break;
			if(c == sidelen())
				out << format("%-2d") % f(r);
			else if(r == sidelen())
				out << format("%-2d") % f(c + sidelen());
			else
				out << cells({r, c}) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_PowerGrid()
{
	using namespace puzzles::PowerGrid;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\PowerGrid.xml", "Puzzles\\PowerGrid.txt", solution_format::GOAL_STATE_ONLY);
}
