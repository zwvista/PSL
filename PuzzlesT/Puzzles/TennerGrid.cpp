#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/TennerGrid

	Summary
	Counting up to 10

	Description
	1. You goal is to enter every digit, from 0 to 9, in each row of the Grid.
	2. The number on the bottom row gives you the sum for that column.
	3. Digit can repeat on the same column, however digits in contiguous tiles
	   must be different, even diagonally. Obviously digits can't repeat on
	   the same row.
*/

namespace puzzles{ namespace TennerGrid{

#define PUZ_UNKNOWN		-1
	
const Position offset[] = {
	{-1, 0},	// n
	{-1, 1},	// ne
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},	// sw
	{-1, -1},	// nw
};

struct puz_area_info
{
	vector<Position> m_ps;
	set<int> m_nums;
	vector<vector<int>> m_combs;

	puz_area_info(){
		m_nums = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	}
};

struct puz_game
{
	string m_id;
	Position m_size;
	vector<int> m_start;
	vector<puz_area_info> m_area_info;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int rows() const { return m_size.first; }
	int cols() const { return m_size.second; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_size(strs.size(), strs[0].size() / 2)
, m_area_info(rows() - 1)
{
	for(int r = 0; r < rows(); ++r){
		auto& str = strs[r];
		for(int c = 0; c < cols(); ++c){
			auto s = str.substr(c * 2, 2);
			int n = s == "  " ? PUZ_UNKNOWN : atoi(s.c_str());
			m_start.push_back(n);
			if(r == rows() - 1) continue;
			auto& info = m_area_info[r];
			if(n == PUZ_UNKNOWN)
				info.m_ps.emplace_back(r, c);
			else
				info.m_nums.erase(n);
		}
	}
	for(auto& info : m_area_info){
		vector<int> comb(info.m_nums.begin(), info.m_nums.end());
		do
			info.m_combs.push_back(comb);
		while(boost::next_permutation(comb));
	}
	boost::sort(m_area_info, [](const puz_area_info& info1, const puz_area_info& info2){
		return info1.m_combs.size() < info2.m_combs.size();
	});
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g)
		: vector<int>(g.m_start), m_game(&g) {}

	int rows() const { return m_game->rows(); }
	int cols() const { return m_game->cols(); }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
	}
	int cell(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
	int& cell(const Position& p) { return (*this)[p.first * cols() + p.second]; }
	bool make_move(int i);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return rows() - 1 - m_area_index; }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	int m_area_index = 0;
};

bool puz_state::make_move(int i)
{
	auto& info = m_game->m_area_info[m_area_index++];
	auto& area = info.m_ps;
	auto& comb = info.m_combs[i];

	for(int k = 0; k < comb.size(); ++k){
		auto& p = area[k];
		int n = cell(p) = comb[k];

		// no touching
		for(auto& os : offset){
			auto p2 = p + os;
			if(is_valid(p2) && cell(p2) == n)
				return false;
		}
	}
	for(int c = 0; c < cols(); ++c){
		int sum = 0;
		bool has_unknown = false;
		for(int r = 0; r < rows() - 1; ++r){
			int n = cell(Position(r, c));
			has_unknown = has_unknown || n == PUZ_UNKNOWN;
			sum += n == PUZ_UNKNOWN ? 0 : n;
		}
		int sum2 = cell(Position(rows() - 1, c));
		if(sum > sum2 || !has_unknown && sum < sum2)
			return false;
	}
	return true;
}

void puz_state::gen_children(list<puz_state> &children) const
{
	int sz = m_game->m_area_info[m_area_index].m_combs.size();
	for(int i = 0; i < sz; ++i){
		children.push_back(*this);
		if(!children.back().make_move(i))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < rows(); ++r){
		for(int c = 0; c < cols(); ++c)
			out << format("%3d") % cell(Position(r, c));
		out << endl;
	}
	return out;
}

}}

void solve_puz_TennerGrid()
{
	using namespace puzzles::TennerGrid;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\TennerGrid.xml", "Puzzles\\TennerGrid.txt", solution_format::GOAL_STATE_ONLY);
}