#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Hidoku

	Summary
	Jump from one neighboring tile to another and fill the board

	Description
	Starting at the tile number 1, reach the last tile by jumping from tile to tile.
	1. When jumping from a tile, you can land on any tile around it, horizontally, 
	   vertically or diagonally touching.
	2. The goal is to jump on every tile, only once and reach the last tile.
*/

namespace puzzles{ namespace hidoku{

const Position offset[] = {
	{-1, 0},
	{-1, 1},
	{0, 1},
	{1, 1},
	{1, 0},
	{1, -1},
	{0, -1},
	{-1, -1},
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	vector<int> m_nums;
	map<int, Position> m_num2pos;
	Position m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen * 3; c += 3){
			Position p(r, c / 3);
			auto s = str.substr(c, 3);
			int n = s == "   " ? 0 : atoi(s.c_str());
			if(n == 1)
				m_start = p;
			m_nums.push_back(n);
			if(n != 0)
				m_num2pos[n] = p;
		}
	}
}

struct puz_state : pair<vector<int>, Position>
{
	puz_state() {}
	puz_state(const puz_game& g) 
		: pair<vector<int>, Position>(g.m_nums, g.m_start)
		, m_game(&g) {}
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	int cell(const Position& p) const { return first.at(p.first * sidelen() + p.second); }
	int& cell(const Position& p) { return first[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return first.size() - cell(second); }
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
};

bool puz_state::make_move(const Position& p, int n)
{
	int& m = cell(second = p);
	if(m == 0){
		m = n;
		const auto& kv = *boost::find_if(m_game->m_num2pos, [=](const pair<const int, Position>& kv){
			return kv.first > n;
		});
		int n2;
		Position p2;
		tie(n2, p2) = kv;
		if(max(myabs(p.first - p2.first), myabs(p.second - p2.second)) > n2 - n)
			return false;
	}
	return true;
}

void puz_state::gen_children(list<puz_state> &children) const
{
	int n = cell(second) + 1;
	auto i = m_game->m_num2pos.find(n);
	bool found = i != m_game->m_num2pos.end();
	for(auto& os : offset){
		auto p = second + os;
		if(found && p == i->second || !found && is_valid(p) && cell(p) == 0){
			children.push_back(*this);
			if(!children.back().make_move(p, n))
				children.pop_back();
			if(found) break;
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << format("%3d") % cell(Position(r, c));
		out << endl;
	}
	return out;
}

}}

void solve_puz_hidoku()
{
	using namespace puzzles::hidoku;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\hidoku.xml", "Puzzles\\hidoku.txt", solution_format::GOAL_STATE_ONLY);
}
